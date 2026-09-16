/*
 * helixtools-restream.c - Plugin entry point for multi-RTMP restreaming
 *
 * Hooks into OBS streaming lifecycle to start/stop additional RTMP
 * outputs in parallel with the primary stream. Destinations are
 * configured via the "Restream Destinations" dock (View -> Docks;
 * restream-dock-init.cpp / restream-dock.cpp) and persisted to disk via
 * restream-storage.c.
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
#include "restream-storage.h"
#include "restream-dock-init.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-restream", "en-US")

static struct restream_context *restream_ctx = NULL;

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Multi-RTMP restreaming (HelixTools)";
}

/* ------------------------------------------------------------------ */
/* Frontend event handler                                              */
/* ------------------------------------------------------------------ */

static void on_frontend_event(enum obs_frontend_event event, void *data)
{
	struct restream_context *ctx = data;

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		blog(LOG_INFO, "[helixtools-restream] Primary stream started, "
			       "launching restream outputs");
		helixtools_restream_start_all(ctx);
		break;

	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		blog(LOG_INFO, "[helixtools-restream] Primary stream stopping, "
			       "shutting down restream outputs");
		helixtools_restream_stop_all(ctx);
		break;

	case OBS_FRONTEND_EVENT_EXIT:
		helixtools_restream_stop_all(ctx);
		break;

	default:
		break;
	}
}

/* ------------------------------------------------------------------ */
/* Module lifecycle                                                    */
/* ------------------------------------------------------------------ */

bool obs_module_load(void)
{
	restream_ctx = bzalloc(sizeof(struct restream_context));

	helixtools_restream_load(restream_ctx);

	obs_frontend_add_event_callback(on_frontend_event, restream_ctx);

	helixtools_restream_init_dock(restream_ctx);

	blog(LOG_INFO, "[helixtools-restream] Plugin loaded (v%s) - %d destinations",
	     HELIXTOOLS_RESTREAM_VERSION, restream_ctx->destination_count);
	return true;
}

void obs_module_unload(void)
{
	helixtools_restream_shutdown_dock();

	if (restream_ctx) {
		obs_frontend_remove_event_callback(on_frontend_event,
						   restream_ctx);
		helixtools_restream_stop_all(restream_ctx);
		helixtools_restream_free(restream_ctx);
		bfree(restream_ctx);
		restream_ctx = NULL;
	}
	blog(LOG_INFO, "[helixtools-restream] Plugin unloaded");
}
