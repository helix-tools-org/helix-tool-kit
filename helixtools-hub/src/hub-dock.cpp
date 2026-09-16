/*
 * hub-dock.cpp - HelixTools Hub browser dock
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

#include "hub-dock.h"
#include "helixtools-hub.h"
#include "overlay-type-map.h"
#include "push-to-studio-script.h"
#include "vendor/browser-panel.hpp"

#include <obs.h>
#include <obs-frontend-api.h>

#include <cstring>
#include <string>

#include <windows.h>
#include <wincred.h>

static HubDock *g_dock = nullptr;

/*
 * Extract the overlay type's URL path segment(s) from a full Hub
 * overlay URL, e.g.
 *   "https://helixtools.org/overlay/rumble/chat/<uuid>" -> "rumble/chat"
 *   "https://helixtools.org/overlay/multichat/<uuid>"   -> "multichat"
 * The user ID is a UUID (36 chars, hyphens at positions 8 and 13);
 * everything before the first UUID-shaped path segment is the type.
 */
static bool isUuidSegment(const std::string &s)
{
	return s.length() == 36 && s[8] == '-' && s[13] == '-';
}

static std::string extractOverlayType(const std::string &url)
{
	const char *marker = "/overlay/";
	size_t pos = url.find(marker);
	if (pos == std::string::npos)
		return "";

	size_t start = pos + strlen(marker);
	size_t queryPos = url.find('?', start);
	size_t end = (queryPos == std::string::npos) ? url.length() : queryPos;
	std::string rest = url.substr(start, end - start);

	std::string typePath;
	size_t segStart = 0;
	while (segStart < rest.length()) {
		size_t segEnd = rest.find('/', segStart);
		if (segEnd == std::string::npos)
			segEnd = rest.length();
		std::string segment = rest.substr(segStart, segEnd - segStart);

		if (isUuidSegment(segment))
			return typePath;

		if (!typePath.empty())
			typePath += "/";
		typePath += segment;

		segStart = segEnd + 1;
	}

	/* No UUID segment found (e.g. a bare type with no user ID yet) */
	return typePath;
}

static const overlay_type_entry *findOverlayType(const std::string &typePath)
{
	for (size_t i = 0; i < NUM_OVERLAY_TYPE_MAP; i++) {
		if (typePath == overlay_type_map[i].url_path)
			return &overlay_type_map[i];
	}
	return nullptr;
}

/* Windows Credential Manager target names shared with helixtools-overlays
 * and helixtools-clip (see their HUB_USERID_CREDENTIAL / HUB_CREDENTIAL_*
 * defines) — must stay in sync. */
static const wchar_t *CRED_USERID = L"HelixTools/HubUserId";
static const wchar_t *CRED_TOKEN = L"HelixTools/HubToken";
static const wchar_t *CRED_REFRESH = L"HelixTools/HubRefresh";

static bool writeCredential(const wchar_t *target, const std::string &value)
{
	CREDENTIALW cred = {};
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = const_cast<LPWSTR>(target);
	cred.CredentialBlobSize = (DWORD)value.size();
	cred.CredentialBlob = (LPBYTE)value.data();
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	return CredWriteW(&cred, 0) == TRUE;
}

HubDock::HubDock(QCef *cef_, QWidget *parent) : QWidget(parent), cef(cef_)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	BPtr<char> cookiePath = obs_module_config_path("cookies");
	cookies = cef->create_cookie_manager(cookiePath ? cookiePath.Get() : "",
					     true);

	browser = cef->create_widget(this, HUB_DOCK_URL, cookies);
	browser->setStartupScript(HELIXTOOLS_PUSH_TO_STUDIO_SCRIPT);
	layout->addWidget(browser);

	/* QCefWidget's real implementation lives inside obs-browser.dll,
	 * loaded dynamically at runtime — its meta-object symbols aren't
	 * linkable at build time, so the new-style &QCefWidget::titleChanged
	 * connect syntax fails to link. The old string-based SIGNAL/SLOT
	 * form resolves via the object's actual (runtime) meta-object
	 * instead, which works fine here. */
	connect(browser, SIGNAL(titleChanged(const QString &)), this,
		SLOT(onTitleChanged(const QString &)));
}

HubDock::~HubDock()
{
	/* browser is owned by the layout/widget tree and destroyed with it */
	if (cookies)
		delete cookies;
}

void HubDock::onTitleChanged(const QString &title)
{
	static const QString pushPrefix = "HELIXTOOLS_PUSH:";
	static const QString authPrefix = "HELIXTOOLS_AUTH:";

	if (title.startsWith(pushPrefix)) {
		handlePushCommand(title.mid(pushPrefix.length()));
	} else if (title.startsWith(authPrefix)) {
		handleAuthCommand(title.mid(authPrefix.length()));
	}
}

void HubDock::handlePushCommand(const QString &json)
{
	obs_data_t *cmd = obs_data_create_from_json(json.toUtf8().constData());
	if (!cmd) {
		blog(LOG_WARNING, "[helixtools-hub] Push to Studio: bad JSON");
		return;
	}

	const char *url = obs_data_get_string(cmd, "url");
	const char *name = obs_data_get_string(cmd, "name");

	if (!url || !*url) {
		blog(LOG_WARNING, "[helixtools-hub] Push to Studio: missing url");
		obs_data_release(cmd);
		return;
	}

	std::string typePath = extractOverlayType(url);
	const overlay_type_entry *entry = findOverlayType(typePath);

	if (!entry) {
		blog(LOG_WARNING,
		     "[helixtools-hub] Push to Studio: unknown overlay type '%s' (url: %s)",
		     typePath.c_str(), url);
		obs_data_release(cmd);
		return;
	}

	obs_source_t *sceneSource = obs_frontend_get_current_scene();
	if (!sceneSource) {
		blog(LOG_WARNING,
		     "[helixtools-hub] Push to Studio: no current scene");
		obs_data_release(cmd);
		return;
	}

	obs_scene_t *scene = obs_scene_from_source(sceneSource);
	const char *sourceName = (name && *name) ? name : entry->display_name;

	/* Pass the exact URL captured from the page as "custom_url" (which
	 * overlay-source.c's overlay_build_url() always prefers over its
	 * own auto-generated one). Several overlay types — Scroller, Media
	 * Looper, Slot, the black-background Multichat variant — bake
	 * extra config into query params that only exist in this captured
	 * URL, not in the plugin's bare per-userId default. */
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "custom_url", url);

	obs_source_t *source =
		obs_source_create(entry->source_id, sourceName, settings, nullptr);
	obs_data_release(settings);
	if (source) {
		obs_scene_add(scene, source);
		obs_source_release(source);
		blog(LOG_INFO,
		     "[helixtools-hub] Push to Studio: added '%s' (%s)",
		     sourceName, entry->source_id);
	} else {
		blog(LOG_ERROR,
		     "[helixtools-hub] Push to Studio: failed to create source '%s'",
		     entry->source_id);
	}

	obs_source_release(sceneSource);
	obs_data_release(cmd);
}

void HubDock::handleAuthCommand(const QString &json)
{
	obs_data_t *cmd = obs_data_create_from_json(json.toUtf8().constData());
	if (!cmd) {
		blog(LOG_WARNING, "[helixtools-hub] Auth capture: bad JSON");
		return;
	}

	const char *userId = obs_data_get_string(cmd, "userId");
	const char *accessToken = obs_data_get_string(cmd, "accessToken");
	const char *refreshToken = obs_data_get_string(cmd, "refreshToken");

	if (!userId || !*userId || !accessToken || !*accessToken) {
		blog(LOG_WARNING, "[helixtools-hub] Auth capture: missing fields");
		obs_data_release(cmd);
		return;
	}

	bool ok = writeCredential(CRED_USERID, userId);
	ok = writeCredential(CRED_TOKEN, accessToken) && ok;
	if (refreshToken && *refreshToken)
		writeCredential(CRED_REFRESH, refreshToken);

	blog(LOG_INFO,
	     "[helixtools-hub] Auth capture: %s (user=%s)",
	     ok ? "cached Hub login to Credential Manager" : "failed to write credentials",
	     userId);

	obs_data_release(cmd);
}

void helixtools_hub_init_dock(void)
{
	QCef *cef = obs_browser_init_panel();
	if (!cef) {
		blog(LOG_ERROR,
		     "[helixtools-hub] obs-browser not available; Hub dock not created");
		return;
	}

	QWidget *mainWindow =
		static_cast<QWidget *>(obs_frontend_get_main_window());
	g_dock = new HubDock(cef, mainWindow);
	obs_frontend_add_dock_by_id("HelixToolsHubDock", "HelixTools Hub", g_dock);
}

void helixtools_hub_shutdown_dock(void)
{
	if (g_dock) {
		obs_frontend_remove_dock("HelixToolsHubDock");
		g_dock = nullptr;
	}
}
