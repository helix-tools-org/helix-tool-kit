/*
 * helixtools-service.h - HelixTools Rumble Service Plugin (standalone)
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

#pragma once

#include <obs-module.h>

#ifndef HELIXTOOLS_SERVICE_VERSION
#define HELIXTOOLS_SERVICE_VERSION "0.1.0"
#endif

/* Matches obs-studio/plugins/rtmp-services/data/services.json's
 * "Rumble Studio" / "Rumble Live" entries — two distinct real
 * endpoints, not one. */
#define RUMBLE_STUDIO_RTMP_URL    "rtmp://studio-rtmp.rumble.com/live"
#define RUMBLE_STUDIO_KEY_LINK    "https://studio.rumble.com/home"
#define RUMBLE_LIVE_RTMP_URL      "rtmp://rtmp.rumble.com/live"
#define RUMBLE_LIVE_KEY_LINK      "https://rumble.com/account/static-stream-key"

#define RUMBLE_DEFAULT_RTMP_URL   RUMBLE_LIVE_RTMP_URL
#define RUMBLE_KEYFRAME_INTERVAL  2
#define RUMBLE_AUDIO_SAMPLERATE   44100
#define RUMBLE_SUPPORTED_CODEC    "h264"

/* rumble-service.c */
extern struct obs_service_info helixtools_rumble_service_info;

/* service-menu.cpp - Tools menu action to set Rumble as the active
 * streaming service. OBS's Stream settings page only lists presets
 * from the built-in rtmp_common service's services.json; a custom
 * obs_service_info like ours is never shown there, so this is the
 * only user-reachable way to actually select it. */
#ifdef __cplusplus
extern "C" {
#endif
void helixtools_service_init_menu(void);
#ifdef __cplusplus
}
#endif
