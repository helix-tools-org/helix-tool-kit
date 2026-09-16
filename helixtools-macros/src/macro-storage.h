/*
 * macro-storage.h - JSON persistence for macros
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

/* Load macros from plugin config directory */
void helixtools_macros_load(void);

/* Save all macros to plugin config directory */
void helixtools_macros_save(void);

/* Lock/unlock the global macro list for thread-safe iteration.
 * Use these around helixtools_macros_get_all() when iterating. */
void helixtools_macros_lock(void);
void helixtools_macros_unlock(void);

/* Generate a new UUID for a macro */
std::string helixtools_macros_generate_uuid(void);

/* Create a new empty macro and add it to the list */
HelixToolsMacro *helixtools_macros_create(const char *name);

/* Remove a macro by pointer (frees memory) */
void helixtools_macros_remove(HelixToolsMacro *macro);
