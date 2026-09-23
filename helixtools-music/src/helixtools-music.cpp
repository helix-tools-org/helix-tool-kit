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
#include "music-manager.h"
#include "vendor/browser-panel.hpp"

#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-music", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "HelixTools Music dock";
}

static MusicDock *g_dock = nullptr;
static MusicManager *g_musicManager = nullptr;

static void onFrontendEvent(enum obs_frontend_event event, void *)
{
	/* Creating a public source any earlier than this — obs_module_post_load
	 * included — races OBS's own scene-collection load, which expects a
	 * clean global source table at that point. FINISHED_LOADING is the
	 * documented safe point for a plugin to add its own sources. */
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		g_musicManager->createAudioSource();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING) {
		/* The shared obs_source_t isn't owned by any one scene, so
		 * nothing tears it down automatically on a collection switch
		 * the way a normal scene item's source would be. Release it
		 * before the switch (its per-scene items go with the
		 * collection being torn down either way). */
		g_musicManager->teardownAudioSource();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED) {
		g_musicManager->createAudioSource();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED) {
		/* Catches a scene created after this collection already
		 * loaded — syncScenes() only touches scenes it hasn't
		 * injected into yet, so this never fights a deliberate
		 * removal from an already-handled scene. */
		g_musicManager->syncScenes();
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		g_musicManager->teardownAudioSource();
	}
}

bool obs_module_load(void)
{
	g_musicManager = new MusicManager();
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
	delete g_musicManager;
	g_musicManager = nullptr;
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
