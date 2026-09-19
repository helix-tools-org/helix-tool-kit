/*
 * helixtools-music.cpp - Plugin entry point for HelixTools Music
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

#include "helixtools-music.h"
#include "music-dock.h"
#include "vendor/browser-panel.hpp"

#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-music", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "HelixTools Music dock + mixer channel";
}

static MusicDock *g_dock = nullptr;
static obs_source_t *g_musicAudioSource = nullptr;
static bool g_audioSourceCreated = false;

static void createMixerAudioSource(void)
{
	if (g_audioSourceCreated)
		return;
	g_audioSourceCreated = true;

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "url", MUSIC_PLAYER_URL);
	obs_data_set_int(settings, "width", 1);
	obs_data_set_int(settings, "height", 1);
	obs_data_set_bool(settings, "shutdown", false);
	obs_data_set_int(settings, "fps", 5);

	/* A real (public) source, not private — obs_set_output_source
	 * below requires that to make it show up in the Audio Mixer, the
	 * same way Desktop Audio/Mic do despite not being scene items
	 * either.
	 *
	 * IMPORTANT: assigning a source to ANY global output channel
	 * (obs_set_output_source) does not just wire up audio — libobs's
	 * main render loop (obs_render_main_texture) walks every channel
	 * 0-63 each frame and calls obs_source_video_render() on whatever
	 * occupies it, the exact mechanism that draws the active program
	 * scene on channel 0. A video-capable source on channel 7 was
	 * therefore being painted directly onto the main canvas every
	 * frame with no scene item backing it — unselectable, undeletable,
	 * fixed in place regardless of scene (this was the floating panel
	 * bug; it was never a CEF popup window). A real Custom Browser
	 * Dock never hits this because it's a plain Qt/CEF widget that's
	 * never assigned to an output channel at all.
	 *
	 * 1x1 keeps the CEF view (and therefore the visible render) too
	 * small to see anything meaningful — audio capture doesn't depend
	 * on the video target's size. */
	g_musicAudioSource = obs_source_create("browser_source",
					       MUSIC_AUDIO_SOURCE_NAME, settings,
					       nullptr);
	obs_data_release(settings);

	if (!g_musicAudioSource) {
		blog(LOG_ERROR,
		     "[helixtools-music] Failed to create audio source (obs-browser missing?)");
		return;
	}

	obs_set_output_source(MUSIC_MIXER_CHANNEL, g_musicAudioSource);
	blog(LOG_INFO, "[helixtools-music] Mixer channel %d ready",
	     MUSIC_MIXER_CHANNEL);
}

static void teardownMixerAudioSource(void)
{
	if (!g_musicAudioSource)
		return;

	obs_set_output_source(MUSIC_MIXER_CHANNEL, nullptr);
	obs_source_release(g_musicAudioSource);
	g_musicAudioSource = nullptr;
	g_audioSourceCreated = false;
}

static void onFrontendEvent(enum obs_frontend_event event, void *)
{
	/* Creating a public source (needed for the Mixer channel) any
	 * earlier than this — obs_module_post_load included — races OBS's
	 * own scene-collection load, which expects a clean global source
	 * table at that point. FINISHED_LOADING is the documented safe
	 * point for a plugin to add its own global/channel sources. */
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		createMixerAudioSource();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING) {
		/* Channel 7 isn't one of the channels OBS itself resets
		 * around a collection switch (only its own mic/desktop
		 * audio + scene/transition channels are). Left alone, this
		 * source survives libobs's scene-data-clear pass as an
		 * untracked leftover ("Not all sources were cleared when
		 * clearing scene data: - HelixTools Music" in the log),
		 * which libobs's teardown path isn't designed to hit. Tear
		 * it down before the switch so the clear pass finds nothing
		 * of ours. */
		teardownMixerAudioSource();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED) {
		createMixerAudioSource();
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		/* Tear down here, not in obs_module_unload(): EXIT fires
		 * while Qt/the GPU thread are still fully alive, whereas
		 * obs_module_unload() runs much later during final module
		 * teardown, where releasing a browser_source has hung
		 * before (see NOTE.md history). */
		teardownMixerAudioSource();
	}
}

bool obs_module_load(void)
{
	obs_frontend_add_event_callback(onFrontendEvent, nullptr);
	blog(LOG_INFO, "[helixtools-music] Plugin loaded (v%s)",
	     HELIXTOOLS_MUSIC_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	/* Dock creation, same proven-safe timing as helixtools-hub/-social:
	 * obs-browser may not have finished its own obs_module_load() yet
	 * if this module happens to load first alphabetically, so this
	 * must wait for obs_module_post_load (runs after every module's
	 * own obs_module_load has completed). */
	helixtools_music_init_dock();
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
	helixtools_music_shutdown_dock();
	/* Safety net in case OBS_FRONTEND_EVENT_EXIT was somehow missed. */
	teardownMixerAudioSource();
	blog(LOG_INFO, "[helixtools-music] Plugin unloaded");
}

void helixtools_music_init_dock(void)
{
	QCef *cef = obs_browser_init_panel();
	if (!cef) {
		blog(LOG_ERROR,
		     "[helixtools-music] obs-browser not available; Music dock not created");
		return;
	}

	QWidget *mainWindow =
		static_cast<QWidget *>(obs_frontend_get_main_window());
	g_dock = new MusicDock(cef, mainWindow);
	obs_frontend_add_dock_by_id("HelixToolsMusicDock", "HelixTools Music",
				    g_dock);
}

void helixtools_music_shutdown_dock(void)
{
	if (g_dock) {
		obs_frontend_remove_dock("HelixToolsMusicDock");
		g_dock = nullptr;
	}
}
