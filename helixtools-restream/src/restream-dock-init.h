/*
 * restream-dock-init.h - OBS dock registration for the destinations editor
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

#include "helixtools-restream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the destinations dock and registers it with OBS as
 * "Restream Destinations" under View -> Docks. Must be called after the
 * main window exists. */
void helixtools_restream_init_dock(struct restream_context *ctx);

/* Removes the dock created by helixtools_restream_init_dock(). */
void helixtools_restream_shutdown_dock(void);

#ifdef __cplusplus
}
#endif
