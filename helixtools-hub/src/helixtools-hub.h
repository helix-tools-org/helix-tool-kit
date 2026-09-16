/*
 * helixtools-hub.h - HelixTools Hub dock plugin (standalone)
 *
 * Hosts the Hub website (helixtools.org/hub) in an OBS dock and adds
 * a "Push to Studio" button next to every overlay/game URL on the
 * page, so a user can add an overlay source to their current scene
 * without leaving the browser.
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

#ifndef HELIXTOOLS_HUB_VERSION
#define HELIXTOOLS_HUB_VERSION "0.1.0"
#endif

#define HUB_DOCK_URL     "https://helixtools.org/hub"
#define HUB_OVERLAY_BASE "helixtools.org/overlay"

/* hub-dock.cpp */
void helixtools_hub_init_dock(void);
void helixtools_hub_shutdown_dock(void);
