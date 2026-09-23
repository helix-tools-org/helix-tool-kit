/*
 * helixtools-dashboard.h - HelixTools Dashboard dock (standalone)
 *
 * A single browser dock with a sidebar (logo + name) to pick one
 * platform (Rumble Studio, Twitch, YouTube, Kick, Blaze, VPZone) and
 * show that platform's stream dashboard — the choice persists across
 * restarts. See dashboard-sites.h for the URL/icon table and
 * dashboard-dock.cpp for the sidebar/persistence mechanics.
 *
 * Deliberately not five separate plugins/docks: confirmed 2026-09-22
 * that the goal is letting a user pick their one main platform, not
 * showing every platform's dashboard at once.
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

#ifndef HELIXTOOLS_DASHBOARD_VERSION
#define HELIXTOOLS_DASHBOARD_VERSION "0.1.0"
#endif

#ifdef __cplusplus
extern "C" {
#endif
void helixtools_dashboard_init_dock(void);
void helixtools_dashboard_shutdown_dock(void);
#ifdef __cplusplus
}
#endif
