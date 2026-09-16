/*
 * restream-dock-init.cpp - OBS dock registration for the destinations editor
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

#include "restream-dock-init.h"
#include "restream-dock.h"

#include <obs-frontend-api.h>

static RestreamDock *g_dock = nullptr;

void helixtools_restream_init_dock(struct restream_context *ctx)
{
	QWidget *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
	g_dock = new RestreamDock(ctx, mainWindow);
	obs_frontend_add_dock_by_id("HelixToolsRestreamDock", "Helix Restream - Destinations", g_dock);
}

void helixtools_restream_shutdown_dock(void)
{
	if (g_dock) {
		obs_frontend_remove_dock("HelixToolsRestreamDock");
		g_dock = nullptr;
	}
}
