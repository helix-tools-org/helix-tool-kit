/*
 * macro-vendor.cpp - obs-websocket vendor API for Stream Deck integration
 *
 * Registers vendor requests: RunMacro, ListMacros, GetMacroState, CancelMacro
 * Emits vendor event: MacroStateChanged
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

#include "macro-vendor.h"
#include "macro-engine.h"
#include "macro-storage.h"
#include "helixtools-macros.h"

#include <obs-websocket-api.h>

static obs_websocket_vendor vendor = nullptr;

/* ---- Vendor Request: RunMacro ---- */

static void cb_run_macro(obs_data_t *request_data, obs_data_t *response_data,
			 void *)
{
	const char *name = obs_data_get_string(request_data, "macroName");
	const char *uuid = obs_data_get_string(request_data, "macroUuid");

	HelixToolsMacro *macro = nullptr;
	if (uuid && strlen(uuid) > 0)
		macro = helixtools_macros_find_by_uuid(uuid);
	if (!macro && name && strlen(name) > 0)
		macro = helixtools_macros_find_by_name(name);

	if (!macro) {
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "error",
				    "Macro not found");
		return;
	}

	if (macro->run_state.load() == MacroRunState::Running) {
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "error",
				    "Macro is already running");
		return;
	}

	helixtools_macros_execute(macro);
	obs_data_set_bool(response_data, "success", true);
	obs_data_set_string(response_data, "macroName",
			    macro->name.c_str());
}

/* ---- Vendor Request: ListMacros ---- */

static void cb_list_macros(obs_data_t *, obs_data_t *response_data, void *)
{
	obs_data_array_t *arr = obs_data_array_create();

	helixtools_macros_lock();
	for (const auto *m : helixtools_macros_get_all()) {
		obs_data_t *obj = obs_data_create();
		obs_data_set_string(obj, "name", m->name.c_str());
		obs_data_set_string(obj, "uuid", m->uuid.c_str());
		obs_data_set_int(obj, "actionCount",
				 (long long)m->actions.size());
		obs_data_set_bool(
			obj, "running",
			m->run_state.load() == MacroRunState::Running);
		obs_data_array_push_back(arr, obj);
		obs_data_release(obj);
	}
	helixtools_macros_unlock();

	obs_data_set_array(response_data, "macros", arr);
	obs_data_array_release(arr);
}

/* ---- Vendor Request: GetMacroState ---- */

static void cb_get_macro_state(obs_data_t *request_data,
			       obs_data_t *response_data, void *)
{
	const char *name = obs_data_get_string(request_data, "macroName");
	const char *uuid = obs_data_get_string(request_data, "macroUuid");

	HelixToolsMacro *macro = nullptr;
	if (uuid && strlen(uuid) > 0)
		macro = helixtools_macros_find_by_uuid(uuid);
	if (!macro && name && strlen(name) > 0)
		macro = helixtools_macros_find_by_name(name);

	if (!macro) {
		obs_data_set_bool(response_data, "found", false);
		return;
	}

	obs_data_set_bool(response_data, "found", true);

	MacroRunState state = macro->run_state.load();
	const char *state_str = "idle";
	if (state == MacroRunState::Running)
		state_str = "running";
	else if (state == MacroRunState::Cancelling)
		state_str = "cancelling";

	obs_data_set_string(response_data, "state", state_str);
	obs_data_set_int(response_data, "currentAction",
			 macro->current_action_index.load());
	obs_data_set_int(response_data, "totalActions",
			 (long long)macro->actions.size());
}

/* ---- Vendor Request: CancelMacro ---- */

static void cb_cancel_macro(obs_data_t *request_data,
			    obs_data_t *response_data, void *)
{
	const char *name = obs_data_get_string(request_data, "macroName");
	const char *uuid = obs_data_get_string(request_data, "macroUuid");

	HelixToolsMacro *macro = nullptr;
	if (uuid && strlen(uuid) > 0)
		macro = helixtools_macros_find_by_uuid(uuid);
	if (!macro && name && strlen(name) > 0)
		macro = helixtools_macros_find_by_name(name);

	if (!macro) {
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "error",
				    "Macro not found");
		return;
	}

	helixtools_macros_cancel(macro);
	obs_data_set_bool(response_data, "success", true);
}

/* ---- Vendor Event: MacroStateChanged ---- */

void helixtools_macros_vendor_emit_state(HelixToolsMacro *macro, const char *state)
{
	if (!vendor || !macro)
		return;

	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, "macroName", macro->name.c_str());
	obs_data_set_string(data, "macroUuid", macro->uuid.c_str());
	obs_data_set_string(data, "state", state);

	obs_websocket_vendor_emit_event(vendor, "MacroStateChanged", data);
	obs_data_release(data);
}

/* ---- Init / Shutdown ---- */

void helixtools_macros_vendor_init(void)
{
	vendor = obs_websocket_register_vendor("helixtools-macros");
	if (!vendor) {
		blog(LOG_WARNING,
		     "[helixtools-macros] Vendor registration failed "
		     "(obs-websocket may not be loaded)");
		return;
	}

	obs_websocket_vendor_register_request(vendor, "RunMacro",
					      cb_run_macro, nullptr);
	obs_websocket_vendor_register_request(vendor, "ListMacros",
					      cb_list_macros, nullptr);
	obs_websocket_vendor_register_request(vendor, "GetMacroState",
					      cb_get_macro_state, nullptr);
	obs_websocket_vendor_register_request(vendor, "CancelMacro",
					      cb_cancel_macro, nullptr);

	blog(LOG_INFO,
	     "[helixtools-macros] Registered vendor requests with obs-websocket");
}

void helixtools_macros_vendor_shutdown(void)
{
	/* obs-websocket handles cleanup on module unload */
	vendor = nullptr;
}
