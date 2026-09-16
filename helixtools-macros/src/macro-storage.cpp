/*
 * macro-storage.cpp - JSON persistence for macros
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

#include "macro-storage.h"
#include "macro-engine.h"
#include "helixtools-macros.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QUuid>

#include <obs-module.h>
#include <util/platform.h>

#include <mutex>

static std::vector<HelixToolsMacro *> g_macros;
static std::mutex g_macros_mutex;

/* ---- Global macro list accessors ---- */

void helixtools_macros_lock(void)
{
	g_macros_mutex.lock();
}

void helixtools_macros_unlock(void)
{
	g_macros_mutex.unlock();
}

std::vector<HelixToolsMacro *> &helixtools_macros_get_all(void)
{
	/* Caller MUST hold g_macros_mutex (via helixtools_macros_lock) */
	return g_macros;
}

HelixToolsMacro *helixtools_macros_find_by_name(const char *name)
{
	if (!name)
		return nullptr;
	std::lock_guard<std::mutex> lock(g_macros_mutex);
	for (auto *m : g_macros) {
		if (m->name == name)
			return m;
	}
	return nullptr;
}

HelixToolsMacro *helixtools_macros_find_by_uuid(const char *uuid)
{
	if (!uuid)
		return nullptr;
	std::lock_guard<std::mutex> lock(g_macros_mutex);
	for (auto *m : g_macros) {
		if (m->uuid == uuid)
			return m;
	}
	return nullptr;
}

/* ---- UUID generation ---- */

std::string helixtools_macros_generate_uuid(void)
{
	return QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
}

/* ---- Action summary text ---- */

std::string HelixToolsAction::summary() const
{
	switch (type) {
	case HelixToolsActionType::SwitchScene:
		return std::string("Switch Scene: ") + scene_name;
	case HelixToolsActionType::ShowSource:
		return std::string("Show: ") + source_name + " in " +
		       scene_name;
	case HelixToolsActionType::HideSource:
		return std::string("Hide: ") + source_name + " in " +
		       scene_name;
	case HelixToolsActionType::ToggleSource:
		return std::string("Toggle: ") + source_name + " in " +
		       scene_name;
	case HelixToolsActionType::MuteSource:
		return std::string("Mute: ") + source_name;
	case HelixToolsActionType::UnmuteSource:
		return std::string("Unmute: ") + source_name;
	case HelixToolsActionType::ToggleAudio:
		return std::string("Toggle Audio: ") + source_name;
	case HelixToolsActionType::SetVolume: {
		char buf[64];
		snprintf(buf, sizeof(buf), "Set Volume: %s to %.0f%%",
			 source_name.c_str(), volume * 100.0f);
		return buf;
	}
	case HelixToolsActionType::Delay: {
		char buf[64];
		snprintf(buf, sizeof(buf), "Delay: %.1f seconds",
			 delay_seconds);
		return buf;
	}
	case HelixToolsActionType::StartStreaming:
		return "Start Streaming";
	case HelixToolsActionType::StopStreaming:
		return "Stop Streaming";
	case HelixToolsActionType::StartRecording:
		return "Start Recording";
	case HelixToolsActionType::StopRecording:
		return "Stop Recording";
	case HelixToolsActionType::SetTransition: {
		char buf[128];
		snprintf(buf, sizeof(buf), "Set Transition: %s (%dms)",
			 transition_name.c_str(), transition_duration_ms);
		return buf;
	}
	}
	return "Unknown Action";
}

/* ---- JSON Serialization ---- */

static QJsonObject action_to_json(const HelixToolsAction &action)
{
	QJsonObject obj;
	obj["type"] = QString::fromUtf8(helixtools_action_type_key(action.type));

	switch (action.type) {
	case HelixToolsActionType::SwitchScene:
		obj["scene_name"] = QString::fromStdString(action.scene_name);
		break;
	case HelixToolsActionType::ShowSource:
	case HelixToolsActionType::HideSource:
	case HelixToolsActionType::ToggleSource:
		obj["scene_name"] = QString::fromStdString(action.scene_name);
		obj["source_name"] =
			QString::fromStdString(action.source_name);
		break;
	case HelixToolsActionType::MuteSource:
	case HelixToolsActionType::UnmuteSource:
	case HelixToolsActionType::ToggleAudio:
		obj["source_name"] =
			QString::fromStdString(action.source_name);
		break;
	case HelixToolsActionType::SetVolume:
		obj["source_name"] =
			QString::fromStdString(action.source_name);
		obj["volume"] = (double)action.volume;
		break;
	case HelixToolsActionType::Delay:
		obj["delay_seconds"] = (double)action.delay_seconds;
		break;
	case HelixToolsActionType::SetTransition:
		obj["transition_name"] =
			QString::fromStdString(action.transition_name);
		obj["transition_duration_ms"] = action.transition_duration_ms;
		break;
	case HelixToolsActionType::StartStreaming:
	case HelixToolsActionType::StopStreaming:
	case HelixToolsActionType::StartRecording:
	case HelixToolsActionType::StopRecording:
		/* No additional parameters */
		break;
	}
	return obj;
}

static HelixToolsAction action_from_json(const QJsonObject &obj)
{
	HelixToolsAction action;
	action.type = helixtools_action_type_from_key(
		obj["type"].toString().toUtf8().constData());

	if (obj.contains("scene_name"))
		action.scene_name =
			obj["scene_name"].toString().toStdString();
	if (obj.contains("source_name"))
		action.source_name =
			obj["source_name"].toString().toStdString();
	if (obj.contains("transition_name"))
		action.transition_name =
			obj["transition_name"].toString().toStdString();
	if (obj.contains("delay_seconds"))
		action.delay_seconds = (float)obj["delay_seconds"].toDouble();
	if (obj.contains("volume"))
		action.volume = (float)obj["volume"].toDouble();
	if (obj.contains("transition_duration_ms"))
		action.transition_duration_ms =
			obj["transition_duration_ms"].toInt();

	return action;
}

static QJsonObject macro_to_json(const HelixToolsMacro *macro)
{
	QJsonObject obj;
	obj["uuid"] = QString::fromStdString(macro->uuid);
	obj["name"] = QString::fromStdString(macro->name);

	QJsonArray actionsArr;
	for (const auto &action : macro->actions)
		actionsArr.append(action_to_json(action));

	obj["actions"] = actionsArr;
	return obj;
}

static HelixToolsMacro *macro_from_json(const QJsonObject &obj)
{
	auto *macro = new HelixToolsMacro();
	macro->uuid = obj["uuid"].toString().toStdString();
	macro->name = obj["name"].toString().toStdString();

	if (macro->uuid.empty())
		macro->uuid = helixtools_macros_generate_uuid();

	QJsonArray actionsArr = obj["actions"].toArray();
	for (const auto &val : actionsArr)
		macro->actions.push_back(action_from_json(val.toObject()));

	return macro;
}

/* ---- File I/O ---- */

static QString get_config_path(void)
{
	char *path = obs_module_config_path("macros.json");
	QString result = QString::fromUtf8(path);
	bfree(path);
	return result;
}

void helixtools_macros_load(void)
{
	std::lock_guard<std::mutex> lock(g_macros_mutex);

	/* Clear existing macros */
	for (auto *m : g_macros)
		delete m;
	g_macros.clear();

	QString path = get_config_path();
	QFile file(path);
	if (!file.exists()) {
		blog(LOG_INFO,
		     "[helixtools-macros] No macros.json found, starting fresh");
		return;
	}

	if (!file.open(QIODevice::ReadOnly)) {
		blog(LOG_WARNING,
		     "[helixtools-macros] Failed to open macros.json for reading");
		return;
	}

	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	file.close();

	if (doc.isNull()) {
		blog(LOG_WARNING,
		     "[helixtools-macros] Failed to parse macros.json: %s",
		     error.errorString().toUtf8().constData());
		return;
	}

	QJsonObject root = doc.object();
	QJsonArray macrosArr = root["macros"].toArray();

	for (const auto &val : macrosArr) {
		HelixToolsMacro *macro = macro_from_json(val.toObject());
		g_macros.push_back(macro);
	}

	blog(LOG_INFO, "[helixtools-macros] Loaded %d macro(s)",
	     (int)g_macros.size());
}

void helixtools_macros_save(void)
{
	std::lock_guard<std::mutex> lock(g_macros_mutex);

	QString path = get_config_path();

	/* Ensure directory exists */
	QDir dir = QFileInfo(path).dir();
	if (!dir.exists())
		dir.mkpath(".");

	QJsonArray macrosArr;
	for (const auto *m : g_macros)
		macrosArr.append(macro_to_json(m));

	QJsonObject root;
	root["version"] = 1;
	root["macros"] = macrosArr;

	QJsonDocument doc(root);

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		blog(LOG_WARNING,
		     "[helixtools-macros] Failed to open macros.json for writing");
		return;
	}

	file.write(doc.toJson(QJsonDocument::Indented));
	file.close();

	blog(LOG_INFO, "[helixtools-macros] Saved %d macro(s)",
	     (int)g_macros.size());
}

/* ---- Macro CRUD ---- */

HelixToolsMacro *helixtools_macros_create(const char *name)
{
	auto *macro = new HelixToolsMacro();
	macro->name = name ? name : "New Macro";
	macro->uuid = helixtools_macros_generate_uuid();
	{
		std::lock_guard<std::mutex> lock(g_macros_mutex);
		g_macros.push_back(macro);
	}
	return macro;
}

void helixtools_macros_remove(HelixToolsMacro *macro)
{
	if (!macro)
		return;

	/* Cancel if running and wait briefly for thread to exit */
	if (macro->run_state.load() == MacroRunState::Running) {
		helixtools_macros_cancel(macro);
		/* Wait up to 500ms for the executor thread to notice */
		for (int i = 0; i < 50; i++) {
			if (macro->run_state.load() == MacroRunState::Idle)
				break;
			os_sleep_ms(10);
		}
	}

	{
		std::lock_guard<std::mutex> lock(g_macros_mutex);
		auto &macros = g_macros;
		for (auto it = macros.begin(); it != macros.end(); ++it) {
			if (*it == macro) {
				macros.erase(it);
				break;
			}
		}
	}
	delete macro;
}
