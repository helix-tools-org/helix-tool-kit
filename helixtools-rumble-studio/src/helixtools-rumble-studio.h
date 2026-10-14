/*
 * helixtools-rumble-studio.h - HelixTools Rumble Studio dock (standalone)
 *
 * Plain browser dock that loads Rumble Studio so users can manage their
 * Rumble channel settings, go live, check analytics, and adjust stream
 * configuration without leaving OBS. (Superseded by helixtools-dashboard,
 * which includes Rumble Studio; no longer packaged in the installer.)
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

#ifndef HELIXTOOLS_RUMBLE_STUDIO_VERSION
#define HELIXTOOLS_RUMBLE_STUDIO_VERSION "0.1.0"
#endif

#define RUMBLE_STUDIO_URL "https://studio.rumble.com"

#ifdef __cplusplus
extern "C" {
#endif
void helixtools_rumble_studio_init_dock(void);
void helixtools_rumble_studio_shutdown_dock(void);
#ifdef __cplusplus
}
#endif
