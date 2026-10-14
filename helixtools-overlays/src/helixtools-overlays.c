/*
 * helixtools-overlays.c - Plugin entry point for Hub overlay sources
 *
 * Registers individual OBS sources for each Hub overlay and game type.
 * Each source's type_data points to its helixtools_overlay_def entry.
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

#include "helixtools-overlays.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-overlays", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "HelixTools Hub overlay and game sources";
}

bool obs_module_load(void)
{
	for (size_t i = 0; i < NUM_OVERLAY_DEFS; i++) {
		struct obs_source_info info = {0};
		info.id = overlay_defs[i].source_id;
		info.type = OBS_SOURCE_TYPE_INPUT;
		/* Deliberately NOT OBS_SOURCE_CAP_DISABLED: that flag hides the
		 * type from the native "Add Source" (+) menu, which also means a
		 * source pushed from the Hub could never be found again to re-add
		 * or add as "existing" later. Visible types are grouped in that
		 * menu by the "HelixTools:" name prefix (see overlay_get_name()). */
		info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO |
				    OBS_SOURCE_CUSTOM_DRAW |
				    OBS_SOURCE_INTERACTION;
		info.get_name = overlay_get_name;
		info.create = overlay_create;
		info.destroy = overlay_destroy;
		info.update = overlay_update;
		info.get_defaults2 = overlay_defaults;
		info.get_properties2 = overlay_properties;
		info.get_width = overlay_get_width;
		info.get_height = overlay_get_height;
		info.enum_active_sources = overlay_enum_active_sources;
		info.video_render = overlay_video_render;
		info.mouse_click = overlay_mouse_click;
		info.mouse_move = overlay_mouse_move;
		info.mouse_wheel = overlay_mouse_wheel;
		info.key_click = overlay_key_click;
		info.focus = overlay_focus;
		/* These wrap a browser source, so use OBS's browser (globe)
		 * icon; OBS_ICON_TYPE_CUSTOM with no icon renders as a blank
		 * document in the Sources list. */
		info.icon_type = OBS_ICON_TYPE_BROWSER;
		info.type_data = (void *)&overlay_defs[i];

		obs_register_source(&info);
	}

	blog(LOG_INFO,
	     "[helixtools-overlays] Registered %d sources (v%s)",
	     (int)NUM_OVERLAY_DEFS, HELIXTOOLS_OVERLAYS_VERSION);
	return true;
}

void obs_module_unload(void)
{
	overlay_free_cred_cache();
	blog(LOG_INFO, "[helixtools-overlays] Plugin unloaded");
}
