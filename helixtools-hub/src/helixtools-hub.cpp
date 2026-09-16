/*
 * helixtools-hub.cpp - Plugin entry point for the HelixTools Hub dock
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

#include "helixtools-hub.h"

#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-hub", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "HelixTools Hub dock (Push to Studio)";
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[helixtools-hub] Plugin loaded (v%s)",
	     HELIXTOOLS_HUB_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	/* obs-browser may not have finished its own obs_module_load() yet
	 * if this module happens to load first (OBS loads plugins roughly
	 * alphabetically, and "helixtools-hub" sorts before "obs-browser").
	 * obs_module_post_load runs only after every module's obs_module_load
	 * has completed, so obs-browser is guaranteed ready here. */
	helixtools_hub_init_dock();
}

void obs_module_unload(void)
{
	helixtools_hub_shutdown_dock();
	blog(LOG_INFO, "[helixtools-hub] Plugin unloaded");
}
