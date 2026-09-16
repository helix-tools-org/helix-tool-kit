/*
 * helixtools-overlays.h - HelixTools Hub overlay sources (standalone)
 *
 * Registers individual OBS sources for each Hub overlay and game type.
 * Source IDs are prefixed helixtools_overlay_<platform>_<type> for
 * per-platform overlays, helixtools_common_<type> for platform-agnostic
 * minigames/utilities, and helixtools_combined_<type> for multi-platform
 * interactive overlays.
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

#pragma once

#include <obs-module.h>

#ifndef HELIXTOOLS_OVERLAYS_VERSION
#define HELIXTOOLS_OVERLAYS_VERSION "0.1.0"
#endif

#define HUB_OVERLAY_BASE "https://helixtools.org/overlay/"

/* Windows Credential Manager targets, written by the HelixTools login
 * helper (runs once on first use of any HelixTools plugin). */
#define HUB_USERID_CREDENTIAL  L"HelixTools/HubUserId"
#define HUB_TOKEN_CREDENTIAL   L"HelixTools/HubToken"
#define HUB_REFRESH_CREDENTIAL L"HelixTools/HubRefresh"

/* ------------------------------------------------------------------ */
/* Overlay type definition (one per concrete overlay/game type)        */
/* ------------------------------------------------------------------ */

struct helixtools_overlay_def {
	const char *source_id;        /* OBS source ID */
	const char *url_path;         /* appended to HUB_OVERLAY_BASE, e.g. "rumble/chat" or "multichat" */
	const char *display_name;     /* shown in the add-source list */
	bool needs_extra_id;          /* true if this type needs an item ID path segment */
	const char *extra_id_label;   /* UI label for item ID (NULL if N/A) */
	uint32_t default_width;
	uint32_t default_height;
};

/* Defined in overlay-source.c */
extern const struct helixtools_overlay_def overlay_defs[];
extern const size_t NUM_OVERLAY_DEFS;

/* Shared callbacks - type_data points to a specific helixtools_overlay_def */
extern const char *overlay_get_name(void *type_data);
extern void *overlay_create(obs_data_t *settings, obs_source_t *source);
extern void overlay_destroy(void *data);
extern void overlay_update(void *data, obs_data_t *settings);
extern void overlay_defaults(void *type_data, obs_data_t *settings);
extern obs_properties_t *overlay_properties(void *data, void *type_data);
extern uint32_t overlay_get_width(void *data);
extern uint32_t overlay_get_height(void *data);
extern void overlay_enum_active_sources(void *data,
					obs_source_enum_proc_t cb,
					void *param);
extern void overlay_video_render(void *data, gs_effect_t *effect);
extern void overlay_mouse_click(void *data, const struct obs_mouse_event *event,
				int32_t type, bool mouse_up, uint32_t click_count);
extern void overlay_mouse_move(void *data, const struct obs_mouse_event *event,
			       bool mouse_leave);
extern void overlay_mouse_wheel(void *data, const struct obs_mouse_event *event,
				int x_delta, int y_delta);
extern void overlay_key_click(void *data, const struct obs_key_event *event,
			      bool key_up);
extern void overlay_focus(void *data, bool focus);

/* Called from obs_module_unload to free static credential cache */
extern void overlay_free_cred_cache(void);
