/*
 * macro-types.h - Data model for HelixTools macros
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

#include <obs.h>
#include <string>
#include <vector>
#include <atomic>

/* ---- Action Types ---- */

enum class HelixToolsActionType {
	SwitchScene,
	ShowSource,
	HideSource,
	ToggleSource,
	MuteSource,
	UnmuteSource,
	ToggleAudio,
	SetVolume,
	Delay,
	StartStreaming,
	StopStreaming,
	StartRecording,
	StopRecording,
	SetTransition,
};

/* Human-readable names for the editor UI */
inline const char *helixtools_action_type_name(HelixToolsActionType type)
{
	switch (type) {
	case HelixToolsActionType::SwitchScene:
		return "Switch Scene";
	case HelixToolsActionType::ShowSource:
		return "Show Source";
	case HelixToolsActionType::HideSource:
		return "Hide Source";
	case HelixToolsActionType::ToggleSource:
		return "Toggle Source";
	case HelixToolsActionType::MuteSource:
		return "Mute Source";
	case HelixToolsActionType::UnmuteSource:
		return "Unmute Source";
	case HelixToolsActionType::ToggleAudio:
		return "Toggle Audio";
	case HelixToolsActionType::SetVolume:
		return "Set Volume";
	case HelixToolsActionType::Delay:
		return "Delay";
	case HelixToolsActionType::StartStreaming:
		return "Start Streaming";
	case HelixToolsActionType::StopStreaming:
		return "Stop Streaming";
	case HelixToolsActionType::StartRecording:
		return "Start Recording";
	case HelixToolsActionType::StopRecording:
		return "Stop Recording";
	case HelixToolsActionType::SetTransition:
		return "Set Transition";
	}
	return "Unknown";
}

/* JSON keys for serialization */
inline const char *helixtools_action_type_key(HelixToolsActionType type)
{
	switch (type) {
	case HelixToolsActionType::SwitchScene:
		return "SwitchScene";
	case HelixToolsActionType::ShowSource:
		return "ShowSource";
	case HelixToolsActionType::HideSource:
		return "HideSource";
	case HelixToolsActionType::ToggleSource:
		return "ToggleSource";
	case HelixToolsActionType::MuteSource:
		return "MuteSource";
	case HelixToolsActionType::UnmuteSource:
		return "UnmuteSource";
	case HelixToolsActionType::ToggleAudio:
		return "ToggleAudio";
	case HelixToolsActionType::SetVolume:
		return "SetVolume";
	case HelixToolsActionType::Delay:
		return "Delay";
	case HelixToolsActionType::StartStreaming:
		return "StartStreaming";
	case HelixToolsActionType::StopStreaming:
		return "StopStreaming";
	case HelixToolsActionType::StartRecording:
		return "StartRecording";
	case HelixToolsActionType::StopRecording:
		return "StopRecording";
	case HelixToolsActionType::SetTransition:
		return "SetTransition";
	}
	return "Unknown";
}

inline HelixToolsActionType helixtools_action_type_from_key(const char *key)
{
	if (!key)
		return HelixToolsActionType::Delay;
	std::string s(key);
	if (s == "SwitchScene")
		return HelixToolsActionType::SwitchScene;
	if (s == "ShowSource")
		return HelixToolsActionType::ShowSource;
	if (s == "HideSource")
		return HelixToolsActionType::HideSource;
	if (s == "ToggleSource")
		return HelixToolsActionType::ToggleSource;
	if (s == "MuteSource")
		return HelixToolsActionType::MuteSource;
	if (s == "UnmuteSource")
		return HelixToolsActionType::UnmuteSource;
	if (s == "ToggleAudio")
		return HelixToolsActionType::ToggleAudio;
	if (s == "SetVolume")
		return HelixToolsActionType::SetVolume;
	if (s == "Delay")
		return HelixToolsActionType::Delay;
	if (s == "StartStreaming")
		return HelixToolsActionType::StartStreaming;
	if (s == "StopStreaming")
		return HelixToolsActionType::StopStreaming;
	if (s == "StartRecording")
		return HelixToolsActionType::StartRecording;
	if (s == "StopRecording")
		return HelixToolsActionType::StopRecording;
	if (s == "SetTransition")
		return HelixToolsActionType::SetTransition;
	return HelixToolsActionType::Delay;
}

/* ---- Action ---- */

struct HelixToolsAction {
	HelixToolsActionType type = HelixToolsActionType::Delay;
	std::string scene_name;
	std::string source_name;
	std::string transition_name;
	float delay_seconds = 1.0f;
	float volume = 1.0f;         /* 0.0 - 2.0 (OBS range) */
	int transition_duration_ms = 300;

	/* Convenience: produce a one-line summary for the editor list */
	std::string summary() const;
};

/* ---- Macro Run State ---- */

enum class MacroRunState { Idle, Running, Cancelling };

/* ---- Macro ---- */

struct HelixToolsMacro {
	std::string name;
	std::string uuid;
	std::vector<HelixToolsAction> actions;

	obs_hotkey_id hotkey_id = OBS_INVALID_HOTKEY_ID;

	/* Transient runtime state (not persisted) */
	std::atomic<MacroRunState> run_state{MacroRunState::Idle};
	std::atomic<int> current_action_index{-1};
};

/* ---- Global Macro List (owned by macro-storage.cpp) ---- */

std::vector<HelixToolsMacro *> &helixtools_macros_get_all(void);
HelixToolsMacro *helixtools_macros_find_by_name(const char *name);
HelixToolsMacro *helixtools_macros_find_by_uuid(const char *uuid);
