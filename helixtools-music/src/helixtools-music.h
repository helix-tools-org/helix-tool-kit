/*
 * helixtools-music.h - HelixTools Music dock (standalone)
 *
 * A plain browser dock (QCefWidget, via the public browser-panel API —
 * the same proven-safe pattern helixtools-hub/-social use) for
 * browsing, logging in, and controlling playback at
 * helixtools.org/music/home.
 *
 * This previously pinned a second, independent "browser_source"
 * instance to a global OBS mixer channel so its audio would show up as
 * a persistent Mixer control. Removed: it was a separate,
 * unauthenticated CEF instance with no playback-state sync to the
 * dock, so its fader controlled silent/logged-out audio and served no
 * real function. It was also the source of the
 * OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING/_CHANGED freeze/crash
 * fixed 2026-09-18.
 *
 * Replaced by MusicManager (music-manager.cpp): a native ffmpeg_source
 * driven by polling the overlay_data Supabase table the dock already
 * writes to for the website's own now-playing overlay. A second
 * attempt at a global mixer channel (this time with ffmpeg_source
 * instead of browser_source) hit the exact same "renders on the
 * canvas" bug for a different reason — see music-manager.cpp's header
 * comment — so this instead adds the source as a real, 0x0-scaled
 * scene item to every scene. MusicManager still handles the
 * scene-collection-switch teardown/recreation of the shared
 * obs_source_t itself (its scene *items* are torn down by OBS's own
 * lifecycle since they're normal scene content now).
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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <obs-module.h>

#ifndef HELIXTOOLS_MUSIC_VERSION
#define HELIXTOOLS_MUSIC_VERSION "0.1.0"
#endif

#define MUSIC_PLAYER_URL "https://helixtools.org/music/home"
#define MUSIC_AUDIO_SOURCE_NAME "HelixTools Music"

/* Channel 7 — outside the 1-6 range OBS's own Settings > Audio >
 * Advanced UI exposes for Desktop Audio (1-2) / Mic-Aux (3-6), so this
 * won't collide with anything a user configures there. */
#define MUSIC_MIXER_CHANNEL 7

#ifdef __cplusplus
extern "C" {
#endif
void helixtools_music_init_dock(void);
void helixtools_music_shutdown_dock(void);
#ifdef __cplusplus
}
#endif
