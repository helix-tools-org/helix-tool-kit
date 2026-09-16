/*
 * helixtools-macros.h - Native macro system for HelixTools
 *
 * Provides hotkey-triggered macro sequences (scene switching, source
 * toggling, audio control, delays) with a visual editor and
 * obs-websocket vendor API for Stream Deck integration.
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
#include <obs-frontend-api.h>

#ifndef HELIXTOOLS_MACROS_VERSION
#define HELIXTOOLS_MACROS_VERSION "0.1.0"
#endif

/* macro-storage.cpp */
void helixtools_macros_load(void);
void helixtools_macros_save(void);

/* macro-engine.cpp */
void helixtools_macros_register_hotkeys(void);
void helixtools_macros_unregister_hotkeys(void);
void helixtools_macros_cancel_all(void);
void helixtools_macros_engine_shutdown(void);

/* macro-vendor.cpp */
void helixtools_macros_vendor_init(void);
void helixtools_macros_vendor_shutdown(void);

/* macro-hub-triggers.cpp */
void helixtools_macros_hub_triggers_init(void);
void helixtools_macros_hub_triggers_shutdown(void);
