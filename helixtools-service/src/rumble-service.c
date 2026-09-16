/*
 * rumble-service.c - Rumble RTMP service implementation
 *
 * Registers a native Rumble streaming service so users don't need to
 * manually configure "Custom RTMP". Supports static stream keys and
 * enforces Rumble's encoder constraints (H.264 only, CBR, 2s keyframe).
 *
 * Copyright (C) 2025 HelixTools
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

#include "helixtools-service.h"

#include <obs-module.h>
#include <util/dstr.h>

/* ------------------------------------------------------------------ */
/* Service data                                                        */
/* ------------------------------------------------------------------ */

struct rumble_service {
	char *server;
	char *key;
};

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

static const char *rumble_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Rumble";
}

static void *rumble_create(obs_data_t *settings, obs_service_t *service)
{
	UNUSED_PARAMETER(service);

	struct rumble_service *rs = bzalloc(sizeof(struct rumble_service));

	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");

	rs->server = bstrdup(server && *server ? server
					       : RUMBLE_DEFAULT_RTMP_URL);
	rs->key = bstrdup(key ? key : "");

	blog(LOG_INFO, "[helixtools-service] Created Rumble service (server: %s)",
	     rs->server);
	return rs;
}

static void rumble_destroy(void *data)
{
	struct rumble_service *rs = data;
	bfree(rs->server);
	bfree(rs->key);
	bfree(rs);
}

static void rumble_update(void *data, obs_data_t *settings)
{
	struct rumble_service *rs = data;

	bfree(rs->server);
	bfree(rs->key);

	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");

	rs->server = bstrdup(server && *server ? server
					       : RUMBLE_DEFAULT_RTMP_URL);
	rs->key = bstrdup(key ? key : "");
}

/* ------------------------------------------------------------------ */
/* Settings defaults                                                   */
/* ------------------------------------------------------------------ */

static void rumble_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "server",
				    RUMBLE_DEFAULT_RTMP_URL);
	obs_data_set_default_string(settings, "key", "");
}

/* ------------------------------------------------------------------ */
/* Properties UI                                                       */
/* ------------------------------------------------------------------ */

static obs_properties_t *rumble_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "server", "RTMP Server URL",
				OBS_TEXT_DEFAULT);

	obs_properties_add_text(props, "key", "Stream Key",
				OBS_TEXT_PASSWORD);

	obs_properties_add_text(
		props, "info",
		"Get your static stream key from:\n"
		"rumble.com/account/static-stream-key\n\n"
		"Rumble recommended settings: H.264, CBR,\n"
		"2s keyframe interval, 1080p, 6000-10000 Kbps.",
		OBS_TEXT_INFO);

	return props;
}

/* ------------------------------------------------------------------ */
/* Service interface                                                   */
/* ------------------------------------------------------------------ */

static const char *rumble_get_url(void *data)
{
	struct rumble_service *rs = data;
	return rs->server;
}

static const char *rumble_get_key(void *data)
{
	struct rumble_service *rs = data;
	return rs->key;
}

static const char **rumble_get_supported_video_codecs(void *data)
{
	UNUSED_PARAMETER(data);

	/* Rumble supports H.264 only — no HEVC, no AV1 */
	static const char *codecs[] = {"h264", NULL};
	return codecs;
}

static const char *rumble_get_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "RTMP";
}

static const char *rumble_get_output_type(void *data)
{
	UNUSED_PARAMETER(data);
	return "rtmp_output";
}

/* ------------------------------------------------------------------ */
/* Registration struct                                                 */
/* ------------------------------------------------------------------ */

struct obs_service_info helixtools_rumble_service_info = {
	.id                      = "helixtools_rumble_service",
	.get_name                = rumble_get_name,
	.create                  = rumble_create,
	.destroy                 = rumble_destroy,
	.update                  = rumble_update,
	.get_defaults            = rumble_defaults,
	.get_properties          = rumble_get_properties,
	.get_url                 = rumble_get_url,
	.get_key                 = rumble_get_key,
	.get_supported_video_codecs = rumble_get_supported_video_codecs,
	.get_protocol            = rumble_get_protocol,
	.get_output_type         = rumble_get_output_type,
};
