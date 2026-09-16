/*
 * helixtools-music.h - HelixTools Music dock + mixer channel (standalone)
 *
 * Two separate things, both pointed at the same page
 * (helixtools.org/music/home):
 *   - A plain browser dock (QCefWidget, via the public browser-panel
 *     API — the same proven-safe pattern helixtools-hub/-social use)
 *     for browsing, logging in, and controlling playback.
 *   - A private "browser_source" instance, pinned to an OBS audio
 *     mixer channel via obs_set_output_source, so its audio shows up
 *     as a persistent Mixer control exactly like Desktop Audio/Mic —
 *     no scene involvement needed.
 *
 * These are two independent browser instances, not one shared browser
 * rendered twice — an earlier attempt at sharing a single instance
 * (rendering an obs_source directly into a custom GPU-backed dock
 * widget) had a confirmed rendering bug where its surface bled onto
 * the Scene Preview canvas. See NOTE.md for that history. This design
 * trades perfect dock/stream playback-state sync for something that
 * can't touch OBS's rendering pipeline at all.
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
