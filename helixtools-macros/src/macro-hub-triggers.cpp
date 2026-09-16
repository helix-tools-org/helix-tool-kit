/*
 * macro-hub-triggers.cpp - Hub event trigger stubs
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

#include "macro-hub-triggers.h"

#include <obs-module.h>

void helixtools_macros_hub_triggers_init(void)
{
	/*
	 * TODO (Phase 5): Connect to Hub WebSocket events
	 * and allow macros to trigger from:
	 * - Rant received (with amount threshold)
	 * - New follower
	 * - New subscriber
	 * - Chat command
	 *
	 * This requires helixtools-hub to expose an event callback API.
	 */
	blog(LOG_INFO,
	     "[helixtools-macros] Hub event triggers initialized (stub)");
}

void helixtools_macros_hub_triggers_shutdown(void)
{
	blog(LOG_INFO, "[helixtools-macros] Hub event triggers shut down");
}
