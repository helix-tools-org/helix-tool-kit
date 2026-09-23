/*
 * helixtools-dashboard.cpp - Plugin entry point for the Dashboard dock
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

#include "helixtools-dashboard.h"
#include "dashboard-dock.h"
#include "vendor/browser-panel.hpp"

#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-dashboard", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "HelixTools Dashboard dock";
}

static DashboardDock *g_dock = nullptr;

bool obs_module_load(void)
{
	blog(LOG_INFO, "[helixtools-dashboard] Plugin loaded (v%s)",
	     HELIXTOOLS_DASHBOARD_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	/* obs-browser may not have finished its own obs_module_load() yet
	 * if this module happens to load first alphabetically — same
	 * load-order issue helixtools-hub/-social/-music/-rumble-studio
	 * all hit. */
	helixtools_dashboard_init_dock();
}

void obs_module_unload(void)
{
	helixtools_dashboard_shutdown_dock();
	blog(LOG_INFO, "[helixtools-dashboard] Plugin unloaded");
}

void helixtools_dashboard_init_dock(void)
{
	QCef *cef = obs_browser_init_panel();
	if (!cef) {
		blog(LOG_ERROR,
		     "[helixtools-dashboard] obs-browser not available; Dashboard dock not created");
		return;
	}

	QWidget *mainWindow =
		static_cast<QWidget *>(obs_frontend_get_main_window());
	g_dock = new DashboardDock(cef, mainWindow);
	obs_frontend_add_dock_by_id("HelixToolsDashboardDock",
				    "Helix - Streaming Dashboards", g_dock);
}

void helixtools_dashboard_shutdown_dock(void)
{
	if (g_dock) {
		obs_frontend_remove_dock("HelixToolsDashboardDock");
		g_dock = nullptr;
	}
}
