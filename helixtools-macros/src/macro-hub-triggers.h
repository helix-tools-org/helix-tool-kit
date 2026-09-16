/*
 * macro-hub-triggers.h - Hub event trigger stubs
 *
 * Future: allows macros to trigger from Hub events (rants, follows, subs).
 * Currently a stub - will be implemented when Hub WebSocket events are ready.
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

/* Initialize Hub event trigger system */
void helixtools_macros_hub_triggers_init(void);

/* Shutdown Hub event trigger system */
void helixtools_macros_hub_triggers_shutdown(void);
