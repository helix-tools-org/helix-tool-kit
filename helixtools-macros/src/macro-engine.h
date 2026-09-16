/*
 * macro-engine.h - Threaded macro execution engine
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

/* Execute a macro in a background thread. No-op if already running. */
void helixtools_macros_execute(HelixToolsMacro *macro);

/* Request cancellation of a running macro */
void helixtools_macros_cancel(HelixToolsMacro *macro);

/* Cancel all running macros */
void helixtools_macros_cancel_all(void);

/* Shutdown engine: cancel all macros and wait for threads to finish */
void helixtools_macros_engine_shutdown(void);

/* Register/unregister hotkeys for all macros */
void helixtools_macros_register_hotkeys(void);
void helixtools_macros_unregister_hotkeys(void);

/* Register a single macro's hotkey (called when creating new macros) */
void helixtools_macros_register_hotkey(HelixToolsMacro *macro);

/* Unregister a single macro's hotkey (called before deleting) */
void helixtools_macros_unregister_hotkey(HelixToolsMacro *macro);
