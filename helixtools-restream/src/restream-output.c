/*
 * restream-output.c - Multi-RTMP output management
 *
 * Creates and manages parallel RTMP outputs for restreaming to
 * YouTube, Twitch, and custom RTMP destinations. By default shares
 * the primary encoder for zero additional CPU cost.
 *
 * Copyright (C) 2026 HelixTools
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "helixtools-restream.h"

#include <util/dstr.h>
#include <util/threading.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/*
 * Safely acquire encoder references from the main streaming output.
 * Both encoders are returned with a held reference that the caller
 * must release via obs_encoder_release() when done.
 * The main_output reference is held across the entire call to avoid
 * the encoder being destroyed between lookup and use.
 */
static bool get_main_encoders(obs_encoder_t **out_venc,
			      obs_encoder_t **out_aenc)
{
	obs_output_t *main_output = obs_frontend_get_streaming_output();
	if (!main_output)
		return false;

	*out_venc = obs_output_get_video_encoder(main_output);
	*out_aenc = obs_output_get_audio_encoder(main_output, 0);

	/* Hold references so encoders survive after we release the output */
	*out_venc = *out_venc ? obs_encoder_get_ref(*out_venc) : NULL;
	*out_aenc = *out_aenc ? obs_encoder_get_ref(*out_aenc) : NULL;

	obs_output_release(main_output);

	if (!*out_venc || !*out_aenc) {
		if (*out_venc) obs_encoder_release(*out_venc);
		if (*out_aenc) obs_encoder_release(*out_aenc);
		*out_venc = NULL;
		*out_aenc = NULL;
		return false;
	}
	return true;
}

/* Signal handler: detect when a restream output stops unexpectedly */
static void on_output_stop(void *data, calldata_t *cd)
{
	struct restream_destination *dest = data;
	int code = (int)calldata_int(cd, "code");

	if (dest->active) {
		dest->active = false;
		blog(LOG_WARNING,
		     "[helixtools-restream] Destination '%s' stopped (code=%d)",
		     dest->name ? dest->name : "unnamed", code);
	}
}

static void start_destination(struct restream_destination *dest, int index,
			      obs_encoder_t *shared_venc,
			      obs_encoder_t *shared_aenc)
{
	if (!dest->enabled || dest->active)
		return;

	if (!dest->rtmp_url || !dest->stream_key) {
		blog(LOG_WARNING,
		     "[helixtools-restream] Destination %d (%s) missing URL or key",
		     index, dest->name ? dest->name : "unnamed");
		return;
	}

	/* Create service for this destination */
	obs_data_t *svc_settings = obs_data_create();
	obs_data_set_string(svc_settings, "server", dest->rtmp_url);
	obs_data_set_string(svc_settings, "key", dest->stream_key);

	struct dstr svc_name = {0};
	dstr_printf(&svc_name, "helixtools_restream_svc_%d", index);

	dest->service = obs_service_create("rtmp_custom", svc_name.array,
					   svc_settings, NULL);
	obs_data_release(svc_settings);
	dstr_free(&svc_name);

	if (!dest->service) {
		blog(LOG_ERROR,
		     "[helixtools-restream] Failed to create service for dest %d",
		     index);
		return;
	}

	/* Create output */
	struct dstr out_name = {0};
	dstr_printf(&out_name, "helixtools_restream_out_%d", index);

	obs_data_t *out_settings = obs_data_create();
	dest->output = obs_output_create("rtmp_output", out_name.array,
					 out_settings, NULL);
	obs_data_release(out_settings);
	dstr_free(&out_name);

	if (!dest->output) {
		blog(LOG_ERROR,
		     "[helixtools-restream] Failed to create output for dest %d",
		     index);
		obs_service_release(dest->service);
		dest->service = NULL;
		return;
	}

	/* Connect stop signal to detect unexpected disconnections */
	signal_handler_t *sh = obs_output_get_signal_handler(dest->output);
	if (sh)
		signal_handler_connect(sh, "stop", on_output_stop, dest);

	obs_output_set_service(dest->output, dest->service);

	/* Share main encoder (zero CPU cost) or use dedicated one */
	obs_encoder_t *venc = dest->video_encoder
				      ? dest->video_encoder
				      : shared_venc;
	obs_encoder_t *aenc = dest->audio_encoder
				      ? dest->audio_encoder
				      : shared_aenc;

	if (!venc || !aenc) {
		blog(LOG_ERROR,
		     "[helixtools-restream] No encoder available for dest %d",
		     index);
		obs_output_release(dest->output);
		obs_service_release(dest->service);
		dest->output = NULL;
		dest->service = NULL;
		return;
	}

	obs_output_set_video_encoder(dest->output, venc);
	obs_output_set_audio_encoder(dest->output, aenc, 0);

	if (obs_output_start(dest->output)) {
		dest->active = true;
		blog(LOG_INFO,
		     "[helixtools-restream] Started dest %d (%s) -> %s",
		     index, dest->name ? dest->name : "unnamed",
		     dest->rtmp_url);
	} else {
		const char *err = obs_output_get_last_error(dest->output);
		blog(LOG_ERROR,
		     "[helixtools-restream] Failed to start dest %d: %s",
		     index, err ? err : "unknown");
		obs_output_release(dest->output);
		obs_service_release(dest->service);
		dest->output = NULL;
		dest->service = NULL;
	}
}

static void stop_destination(struct restream_destination *dest, int index)
{
	if (!dest->active && !dest->output)
		return;

	if (dest->output) {
		/* Disconnect signal handler before stopping */
		signal_handler_t *sh =
			obs_output_get_signal_handler(dest->output);
		if (sh)
			signal_handler_disconnect(sh, "stop",
						  on_output_stop, dest);

		/* Force-stop to avoid blocking while flushing */
		obs_output_force_stop(dest->output);
		obs_output_release(dest->output);
		dest->output = NULL;
	}

	if (dest->service) {
		obs_service_release(dest->service);
		dest->service = NULL;
	}

	dest->active = false;
	blog(LOG_INFO, "[helixtools-restream] Stopped dest %d (%s)",
	     index, dest->name ? dest->name : "unnamed");
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

/* ---- Background thread wrappers ---- */

struct restream_thread_data {
	struct restream_context *ctx;
};

static void *start_all_thread_func(void *data)
{
	struct restream_thread_data *td = data;
	struct restream_context *ctx = td->ctx;

	/* Acquire shared encoders once for all destinations */
	obs_encoder_t *venc = NULL;
	obs_encoder_t *aenc = NULL;
	if (!get_main_encoders(&venc, &aenc)) {
		blog(LOG_ERROR,
		     "[helixtools-restream] Could not acquire main encoders");
		bfree(td);
		return NULL;
	}

	for (int i = 0; i < ctx->destination_count; i++)
		start_destination(&ctx->destinations[i], i, venc, aenc);

	/* Release the encoder references we held during startup */
	obs_encoder_release(venc);
	obs_encoder_release(aenc);

	bfree(td);
	return NULL;
}

static void *stop_all_thread_func(void *data)
{
	struct restream_thread_data *td = data;
	struct restream_context *ctx = td->ctx;

	for (int i = 0; i < ctx->destination_count; i++)
		stop_destination(&ctx->destinations[i], i);

	ctx->streaming = false;
	bfree(td);
	return NULL;
}

void helixtools_restream_start_all(struct restream_context *ctx)
{
	if (ctx->streaming)
		return;

	ctx->streaming = true;

	/* Start destinations on a background thread to avoid
	 * blocking the UI thread during RTMP handshakes */
	struct restream_thread_data *td =
		bzalloc(sizeof(struct restream_thread_data));
	td->ctx = ctx;

	pthread_t thread;
	if (pthread_create(&thread, NULL, start_all_thread_func, td) == 0) {
		pthread_detach(thread);
	} else {
		blog(LOG_ERROR,
		     "[helixtools-restream] Failed to create start thread");
		bfree(td);
	}
}

void helixtools_restream_stop_all(struct restream_context *ctx)
{
	if (!ctx->streaming)
		return;

	/* Stop destinations on a background thread to avoid
	 * blocking the UI thread during output flush */
	struct restream_thread_data *td =
		bzalloc(sizeof(struct restream_thread_data));
	td->ctx = ctx;

	pthread_t thread;
	if (pthread_create(&thread, NULL, stop_all_thread_func, td) == 0) {
		pthread_detach(thread);
	} else {
		blog(LOG_ERROR,
		     "[helixtools-restream] Failed to create stop thread");
		/* Fallback: stop synchronously */
		for (int i = 0; i < ctx->destination_count; i++)
			stop_destination(&ctx->destinations[i], i);
		ctx->streaming = false;
		bfree(td);
	}
}

void helixtools_restream_load_config(struct restream_context *ctx,
				     obs_data_t *settings)
{
	bfree(ctx->main_stream_label);
	const char *label = obs_data_get_string(settings, "main_stream_label");
	ctx->main_stream_label = (label && *label) ? bstrdup(label) : NULL;

	obs_data_array_t *dests = obs_data_get_array(settings, "destinations");
	if (!dests) {
		ctx->destination_count = 0;
		return;
	}

	size_t count = obs_data_array_count(dests);
	if (count > HELIXTOOLS_RESTREAM_MAX_DESTINATIONS)
		count = HELIXTOOLS_RESTREAM_MAX_DESTINATIONS;

	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(dests, i);
		struct restream_destination *d = &ctx->destinations[i];

		d->name = bstrdup(obs_data_get_string(item, "name"));
		d->rtmp_url = bstrdup(obs_data_get_string(item, "url"));
		d->stream_key = bstrdup(obs_data_get_string(item, "key"));
		d->enabled = obs_data_get_bool(item, "enabled");
		d->active = false;
		d->service = NULL;
		d->output = NULL;
		d->video_encoder = NULL;
		d->audio_encoder = NULL;

		obs_data_release(item);
	}

	ctx->destination_count = (int)count;
	obs_data_array_release(dests);
}

void helixtools_restream_save_config(struct restream_context *ctx,
				     obs_data_t *settings)
{
	obs_data_set_string(settings, "main_stream_label",
			    ctx->main_stream_label ? ctx->main_stream_label : "");

	obs_data_array_t *dests = obs_data_array_create();

	for (int i = 0; i < ctx->destination_count; i++) {
		struct restream_destination *d = &ctx->destinations[i];
		obs_data_t *item = obs_data_create();

		obs_data_set_string(item, "name", d->name ? d->name : "");
		obs_data_set_string(item, "url",
				    d->rtmp_url ? d->rtmp_url : "");
		obs_data_set_string(item, "key",
				    d->stream_key ? d->stream_key : "");
		obs_data_set_bool(item, "enabled", d->enabled);

		obs_data_array_push_back(dests, item);
		obs_data_release(item);
	}

	obs_data_set_array(settings, "destinations", dests);
	obs_data_array_release(dests);
}

void helixtools_restream_free(struct restream_context *ctx)
{
	bfree(ctx->main_stream_label);
	ctx->main_stream_label = NULL;

	for (int i = 0; i < ctx->destination_count; i++) {
		struct restream_destination *d = &ctx->destinations[i];
		/* Release OBS objects if still active (safety net) */
		if (d->output) {
			obs_output_release(d->output);
			d->output = NULL;
		}
		if (d->service) {
			obs_service_release(d->service);
			d->service = NULL;
		}
		bfree(d->name);
		bfree(d->rtmp_url);
		bfree(d->stream_key);
	}
	ctx->destination_count = 0;
}
