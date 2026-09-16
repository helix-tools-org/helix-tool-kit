/*
 * macro-engine.cpp - Threaded macro execution engine
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

#include "macro-engine.h"
#include "macro-storage.h"
#include "macro-vendor.h"
#include "helixtools-macros.h"

#include <obs.h>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>

/* ---- Shutdown synchronization ---- */

static std::atomic<bool> engine_shutting_down{false};
static std::atomic<int> active_thread_count{0};
static std::mutex shutdown_mutex;
static std::condition_variable shutdown_cv;

/* ---- Helper: execute OBS API call on UI thread ---- */

struct UiTaskData {
	HelixToolsAction action;
	bool done = false;
};

static void execute_action_on_ui(void *param)
{
	auto *data = static_cast<UiTaskData *>(param);
	const auto &action = data->action;

	switch (action.type) {
	case HelixToolsActionType::SwitchScene: {
		obs_source_t *scene =
			obs_get_source_by_name(action.scene_name.c_str());
		if (scene) {
			obs_frontend_set_current_scene(scene);
			obs_source_release(scene);
		} else {
			blog(LOG_WARNING,
			     "[helixtools-macros] Scene not found: %s",
			     action.scene_name.c_str());
		}
		break;
	}

	case HelixToolsActionType::ShowSource:
	case HelixToolsActionType::HideSource:
	case HelixToolsActionType::ToggleSource: {
		obs_source_t *scene_src =
			obs_get_source_by_name(action.scene_name.c_str());
		if (!scene_src) {
			blog(LOG_WARNING,
			     "[helixtools-macros] Scene not found: %s",
			     action.scene_name.c_str());
			break;
		}
		obs_scene_t *scene = obs_scene_from_source(scene_src);
		if (!scene) {
			obs_source_release(scene_src);
			break;
		}
		obs_sceneitem_t *item = obs_scene_find_source_recursive(
			scene, action.source_name.c_str());
		if (item) {
			bool visible;
			if (action.type == HelixToolsActionType::ShowSource)
				visible = true;
			else if (action.type == HelixToolsActionType::HideSource)
				visible = false;
			else /* Toggle */
				visible = !obs_sceneitem_visible(item);
			obs_sceneitem_set_visible(item, visible);
		} else {
			blog(LOG_WARNING,
			     "[helixtools-macros] Source '%s' not found in scene '%s'",
			     action.source_name.c_str(),
			     action.scene_name.c_str());
		}
		obs_source_release(scene_src);
		break;
	}

	case HelixToolsActionType::MuteSource:
	case HelixToolsActionType::UnmuteSource:
	case HelixToolsActionType::ToggleAudio: {
		obs_source_t *src =
			obs_get_source_by_name(action.source_name.c_str());
		if (src) {
			if (action.type == HelixToolsActionType::MuteSource)
				obs_source_set_muted(src, true);
			else if (action.type == HelixToolsActionType::UnmuteSource)
				obs_source_set_muted(src, false);
			else /* Toggle */
				obs_source_set_muted(src,
						     !obs_source_muted(src));
			obs_source_release(src);
		} else {
			blog(LOG_WARNING,
			     "[helixtools-macros] Audio source not found: %s",
			     action.source_name.c_str());
		}
		break;
	}

	case HelixToolsActionType::SetVolume: {
		obs_source_t *src =
			obs_get_source_by_name(action.source_name.c_str());
		if (src) {
			obs_source_set_volume(src, action.volume);
			obs_source_release(src);
		} else {
			blog(LOG_WARNING,
			     "[helixtools-macros] Source not found for volume: %s",
			     action.source_name.c_str());
		}
		break;
	}

	case HelixToolsActionType::StartStreaming:
		obs_frontend_streaming_start();
		break;

	case HelixToolsActionType::StopStreaming:
		obs_frontend_streaming_stop();
		break;

	case HelixToolsActionType::StartRecording:
		obs_frontend_recording_start();
		break;

	case HelixToolsActionType::StopRecording:
		obs_frontend_recording_stop();
		break;

	case HelixToolsActionType::SetTransition: {
		/* Find transition by name from the list of available transitions */
		struct obs_frontend_source_list transitions = {};
		obs_frontend_get_transitions(&transitions);
		for (size_t i = 0; i < transitions.sources.num; i++) {
			obs_source_t *tr = transitions.sources.array[i];
			const char *name = obs_source_get_name(tr);
			if (name &&
			    action.transition_name == name) {
				obs_frontend_set_current_transition(tr);
				obs_frontend_set_transition_duration(
					action.transition_duration_ms);
				break;
			}
		}
		obs_frontend_source_list_free(&transitions);
		break;
	}

	case HelixToolsActionType::Delay:
		/* Delay is handled in the executor thread, not here */
		break;
	}

	data->done = true;
}

/* ---- Macro Executor Thread ---- */

static void macro_executor_thread(HelixToolsMacro *macro)
{
	active_thread_count.fetch_add(1);

	blog(LOG_INFO, "[helixtools-macros] Executing macro: %s (%d actions)",
	     macro->name.c_str(), (int)macro->actions.size());

	/* Emit vendor event: started */
	if (!engine_shutting_down.load())
		helixtools_macros_vendor_emit_state(macro, "started");

	for (size_t i = 0; i < macro->actions.size(); i++) {
		if (macro->run_state.load() == MacroRunState::Cancelling ||
		    engine_shutting_down.load())
			break;

		macro->current_action_index.store((int)i);
		const auto &action = macro->actions[i];

		if (action.type == HelixToolsActionType::Delay) {
			/* Sleep in 100ms chunks for responsive cancellation */
			int total_ms = (int)(action.delay_seconds * 1000.0f);
			int elapsed = 0;
			while (elapsed < total_ms &&
			       macro->run_state.load() !=
				       MacroRunState::Cancelling &&
			       !engine_shutting_down.load()) {
				int chunk = (total_ms - elapsed < 100)
						    ? (total_ms - elapsed)
						    : 100;
				os_sleep_ms(chunk);
				elapsed += chunk;
			}
		} else if (!engine_shutting_down.load()) {
			/* Execute action on UI thread synchronously */
			UiTaskData task_data;
			task_data.action = action;
			obs_queue_task(OBS_TASK_UI, execute_action_on_ui,
				       &task_data, true);
		}
	}

	bool cancelled =
		macro->run_state.load() == MacroRunState::Cancelling;
	macro->run_state.store(MacroRunState::Idle);
	macro->current_action_index.store(-1);

	/* Emit vendor event */
	if (!engine_shutting_down.load())
		helixtools_macros_vendor_emit_state(
			macro, cancelled ? "cancelled" : "completed");

	blog(LOG_INFO, "[helixtools-macros] Macro '%s' %s", macro->name.c_str(),
	     cancelled ? "cancelled" : "completed");

	if (active_thread_count.fetch_sub(1) == 1) {
		std::lock_guard<std::mutex> lock(shutdown_mutex);
		shutdown_cv.notify_all();
	}
}

/* ---- Public API ---- */

void helixtools_macros_execute(HelixToolsMacro *macro)
{
	if (!macro || macro->actions.empty() || engine_shutting_down.load())
		return;

	MacroRunState expected = MacroRunState::Idle;
	if (!macro->run_state.compare_exchange_strong(
		    expected, MacroRunState::Running)) {
		blog(LOG_INFO,
		     "[helixtools-macros] Macro '%s' is already running",
		     macro->name.c_str());
		return;
	}

	std::thread(macro_executor_thread, macro).detach();
}

void helixtools_macros_cancel(HelixToolsMacro *macro)
{
	if (!macro)
		return;

	MacroRunState expected = MacroRunState::Running;
	macro->run_state.compare_exchange_strong(expected,
						 MacroRunState::Cancelling);
}

void helixtools_macros_cancel_all(void)
{
	helixtools_macros_lock();
	for (auto *m : helixtools_macros_get_all())
		helixtools_macros_cancel(m);
	helixtools_macros_unlock();
}

void helixtools_macros_engine_shutdown(void)
{
	engine_shutting_down.store(true);
	helixtools_macros_cancel_all();

	/* Wait up to 10 seconds for active threads to finish.
	 * Macro delays sleep in 100ms chunks and check the shutdown
	 * flag, so even long delays should exit within a few hundred ms. */
	std::unique_lock<std::mutex> lock(shutdown_mutex);
	shutdown_cv.wait_for(lock, std::chrono::seconds(10), []() {
		return active_thread_count.load() == 0;
	});

	if (active_thread_count.load() > 0)
		blog(LOG_WARNING,
		     "[helixtools-macros] %d thread(s) still active at shutdown",
		     active_thread_count.load());
}

/* ---- Hotkey System ---- */

static void macro_hotkey_callback(void *data, obs_hotkey_id, obs_hotkey_t *,
				  bool pressed)
{
	if (!pressed)
		return;

	auto *macro = static_cast<HelixToolsMacro *>(data);
	if (!macro)
		return;

	/* Toggle: if running, cancel; if idle, execute */
	if (macro->run_state.load() == MacroRunState::Running) {
		helixtools_macros_cancel(macro);
	} else {
		helixtools_macros_execute(macro);
	}
}

void helixtools_macros_register_hotkey(HelixToolsMacro *macro)
{
	if (!macro || macro->hotkey_id != OBS_INVALID_HOTKEY_ID)
		return;

	std::string hotkey_name = "helixtools_macro_" + macro->uuid;
	std::string hotkey_desc = "Run Macro: " + macro->name;

	macro->hotkey_id = obs_hotkey_register_frontend(
		hotkey_name.c_str(), hotkey_desc.c_str(),
		macro_hotkey_callback, macro);
}

void helixtools_macros_unregister_hotkey(HelixToolsMacro *macro)
{
	if (!macro || macro->hotkey_id == OBS_INVALID_HOTKEY_ID)
		return;

	obs_hotkey_unregister(macro->hotkey_id);
	macro->hotkey_id = OBS_INVALID_HOTKEY_ID;
}

void helixtools_macros_register_hotkeys(void)
{
	helixtools_macros_lock();
	for (auto *m : helixtools_macros_get_all())
		helixtools_macros_register_hotkey(m);
	helixtools_macros_unlock();
}

void helixtools_macros_unregister_hotkeys(void)
{
	helixtools_macros_lock();
	for (auto *m : helixtools_macros_get_all())
		helixtools_macros_unregister_hotkey(m);
	helixtools_macros_unlock();
}
