/*
 * macro-vendor.h - obs-websocket vendor API for Stream Deck integration
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

#include "macro-types.h"

/* Initialize vendor registration (call from obs_module_post_load) */
void helixtools_macros_vendor_init(void);

/* Cleanup vendor (call from obs_module_unload) */
void helixtools_macros_vendor_shutdown(void);

/* Emit a MacroStateChanged vendor event */
void helixtools_macros_vendor_emit_state(HelixToolsMacro *macro, const char *state);
