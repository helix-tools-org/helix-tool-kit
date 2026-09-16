/*
 * restream-storage.c - Disk persistence for restream destinations
 *
 * Wraps the existing obs_data_t-based (de)serialization in
 * restream-output.c with real file I/O, using the same
 * obs_module_config_path() convention the rest of the plugin suite
 * uses for local per-plugin settings.
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

#include "restream-storage.h"

#include <util/platform.h>

void helixtools_restream_load(struct restream_context *ctx)
{
	char *path = obs_module_config_path("destinations.json");
	obs_data_t *settings = obs_data_create_from_json_file(path);
	bfree(path);

	if (!settings) {
		ctx->destination_count = 0;
		blog(LOG_INFO,
		     "[helixtools-restream] No destinations.json found, starting fresh");
		return;
	}

	helixtools_restream_load_config(ctx, settings);
	obs_data_release(settings);

	blog(LOG_INFO, "[helixtools-restream] Loaded %d destination(s) from disk",
	     ctx->destination_count);
}

void helixtools_restream_save(struct restream_context *ctx)
{
	char *dir = obs_module_config_path(NULL);
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}

	char *path = obs_module_config_path("destinations.json");

	obs_data_t *settings = obs_data_create();
	helixtools_restream_save_config(ctx, settings);

	if (!obs_data_save_json(settings, path)) {
		blog(LOG_WARNING,
		     "[helixtools-restream] Failed to save destinations.json");
	} else {
		blog(LOG_INFO, "[helixtools-restream] Saved %d destination(s) to disk",
		     ctx->destination_count);
	}

	obs_data_release(settings);
	bfree(path);
}
