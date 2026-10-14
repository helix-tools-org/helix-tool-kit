/*
 * update-checker.cpp - Notify-only update check for the Helix ToolKit suite
 *
 * Asks GitHub's public API for the latest release of the toolkit repo and
 * compares its tag with the version the installer recorded in the registry.
 * If a newer release exists, the user gets a prompt with a button that opens
 * the release page in their browser. Nothing is ever downloaded or run from
 * here — OBS locks plugin DLLs while it's running and the installer needs
 * admin rights, so installing stays a deliberate, user-driven step.
 *
 * Uses WinHTTP directly rather than Qt Network: this plugin is always
 * installed, but Qt's TLS backend plugin is only bundled with the clip
 * component, so Qt HTTPS could silently fail here.
 *
 * Privacy: one anonymous HTTPS GET to api.github.com (no identifiers
 * sent beyond a generic User-Agent). Can be turned off from the Tools menu
 * dialog.
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

#include "helixtools-service.h"

#include <obs-frontend-api.h>
#include <util/platform.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMetaObject>
#include <QObject>
#include <QPushButton>
#include <QUrl>
#include <QWidget>

#include <atomic>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <windows.h>
#include <winhttp.h>

namespace {

#define UPDATE_REPO_OWNER "helix-tools-org"
#define UPDATE_REPO_NAME "helix-tool-kit"
#define UPDATE_API_HOST L"api.github.com"
#define UPDATE_API_PATH L"/repos/helix-tools-org/helix-tool-kit/releases/latest"
#define UPDATE_RELEASE_URL_PREFIX "https://github.com/" UPDATE_REPO_OWNER "/" UPDATE_REPO_NAME "/"
#define UPDATE_CHECK_INTERVAL_SEC (20 * 60 * 60)

struct update_result {
	bool ok = false;        /* request + parse succeeded */
	std::string tag;        /* e.g. "v0.7.11" */
	std::string url;        /* release page */
};

struct update_settings {
	bool auto_check = true;
	std::string skipped_version;
	int64_t last_check = 0;
};

QObject *g_ctx = nullptr; /* lives on the UI thread; target for queued callbacks */
std::thread g_thread;
std::atomic<bool> g_busy{false};
std::atomic<bool> g_shutdown{false};
std::mutex g_session_mutex;
HINTERNET g_session = nullptr;

/* ---- settings (global, not per-profile) ------------------------------ */

std::string settings_path()
{
	char *p = obs_module_config_path("updater.json");
	std::string out = p ? p : "";
	bfree(p);
	return out;
}

update_settings load_settings()
{
	update_settings s;
	std::string path = settings_path();
	if (path.empty())
		return s;
	obs_data_t *d = obs_data_create_from_json_file(path.c_str());
	if (!d)
		return s;
	obs_data_set_default_bool(d, "auto_check", true);
	s.auto_check = obs_data_get_bool(d, "auto_check");
	s.skipped_version = obs_data_get_string(d, "skipped_version");
	s.last_check = obs_data_get_int(d, "last_check");
	obs_data_release(d);
	return s;
}

void save_settings(const update_settings &s)
{
	std::string path = settings_path();
	if (path.empty())
		return;
	char *dir = obs_module_config_path("");
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}
	obs_data_t *d = obs_data_create();
	obs_data_set_bool(d, "auto_check", s.auto_check);
	obs_data_set_string(d, "skipped_version", s.skipped_version.c_str());
	obs_data_set_int(d, "last_check", s.last_check);
	obs_data_save_json_safe(d, path.c_str(), "tmp", "bak");
	obs_data_release(d);
}

/* ---- versions ----------------------------------------------------------- */

/* Version the installer recorded (see [Registry] in the .iss). Empty for
 * dev builds / manual copies, in which case we simply don't check. */
std::string installed_version()
{
	HKEY key;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\HelixTools\\PluginSuite", 0,
			  KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
		return "";
	wchar_t buf[64] = {0};
	DWORD size = sizeof(buf) - sizeof(wchar_t);
	DWORD type = 0;
	LONG rc = RegQueryValueExW(key, L"Version", nullptr, &type, (LPBYTE)buf, &size);
	RegCloseKey(key);
	if (rc != ERROR_SUCCESS || type != REG_SZ)
		return "";
	char out[128] = {0};
	WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, sizeof(out) - 1, nullptr, nullptr);
	return out;
}

std::vector<int> parse_version(const std::string &v)
{
	std::vector<int> parts;
	size_t i = (!v.empty() && (v[0] == 'v' || v[0] == 'V')) ? 1 : 0;
	int cur = 0;
	bool have = false;
	for (; i < v.size(); i++) {
		char c = v[i];
		if (c >= '0' && c <= '9') {
			cur = cur * 10 + (c - '0');
			have = true;
		} else if (c == '.') {
			parts.push_back(cur);
			cur = 0;
			have = false;
		} else {
			break; /* "-beta", "+build" etc. */
		}
	}
	if (have)
		parts.push_back(cur);
	return parts;
}

/* true if `latest` is strictly newer than `current` */
bool is_newer(const std::string &latest, const std::string &current)
{
	std::vector<int> a = parse_version(latest), b = parse_version(current);
	if (a.empty() || b.empty())
		return false;
	size_t n = a.size() > b.size() ? a.size() : b.size();
	for (size_t i = 0; i < n; i++) {
		int x = i < a.size() ? a[i] : 0;
		int y = i < b.size() ? b[i] : 0;
		if (x != y)
			return x > y;
	}
	return false;
}

/* ---- network ------------------------------------------------------------ */

update_result fetch_latest_release()
{
	update_result res;

	HINTERNET session = WinHttpOpen(L"HelixToolKit-UpdateCheck/" L"1.0",
					WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
					WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session)
		return res;
	{
		std::lock_guard<std::mutex> lock(g_session_mutex);
		g_session = session;
	}
	WinHttpSetTimeouts(session, 5000, 5000, 5000, 8000);

	HINTERNET conn = WinHttpConnect(session, UPDATE_API_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
	HINTERNET req = conn ? WinHttpOpenRequest(conn, L"GET", UPDATE_API_PATH, nullptr,
						  WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
						  WINHTTP_FLAG_SECURE)
			     : nullptr;

	std::string body;
	bool got_ok = false;
	if (req) {
		static const wchar_t *headers = L"Accept: application/vnd.github+json\r\n";
		if (WinHttpSendRequest(req, headers, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
		    WinHttpReceiveResponse(req, nullptr)) {
			DWORD status = 0, sz = sizeof(status);
			WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					    WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz,
					    WINHTTP_NO_HEADER_INDEX);
			if (status == 200) {
				got_ok = true;
				DWORD avail = 0;
				while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
					std::string chunk(avail, '\0');
					DWORD read = 0;
					if (!WinHttpReadData(req, &chunk[0], avail, &read) || !read)
						break;
					body.append(chunk, 0, read);
					if (body.size() > 1024 * 1024) /* sanity cap */
						break;
				}
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(g_session_mutex);
		g_session = nullptr;
	}
	if (req)
		WinHttpCloseHandle(req);
	if (conn)
		WinHttpCloseHandle(conn);
	WinHttpCloseHandle(session);

	if (!got_ok || body.empty())
		return res;

	obs_data_t *json = obs_data_create_from_json(body.c_str());
	if (!json)
		return res;
	res.tag = obs_data_get_string(json, "tag_name");
	res.url = obs_data_get_string(json, "html_url");
	bool draft = obs_data_get_bool(json, "draft");
	bool prerelease = obs_data_get_bool(json, "prerelease");
	obs_data_release(json);

	/* Only ever open a link on the toolkit's own repo. */
	if (res.tag.empty() || draft || prerelease ||
	    res.url.compare(0, sizeof(UPDATE_RELEASE_URL_PREFIX) - 1, UPDATE_RELEASE_URL_PREFIX) != 0)
		return res;

	res.ok = true;
	return res;
}

/* ---- UI (always on the UI thread) --------------------------------------- */

QWidget *main_window()
{
	return static_cast<QWidget *>(obs_frontend_get_main_window());
}

void show_result(const update_result &res, const std::string &installed, bool manual)
{
	update_settings s = load_settings();
	s.last_check = (int64_t)time(nullptr);

	QWidget *parent = main_window();
	QString title = "Helix ToolKit Updates";

	bool newer = res.ok && is_newer(res.tag, installed);
	bool skipped = newer && res.tag == s.skipped_version;

	if (!manual && (!newer || skipped)) {
		save_settings(s);
		return;
	}

	QMessageBox box(parent);
	box.setWindowTitle(title);
	QCheckBox *autoBox = new QCheckBox("Check for updates automatically");
	autoBox->setChecked(s.auto_check);
	box.setCheckBox(autoBox);

	QPushButton *download = nullptr;
	QPushButton *skip = nullptr;

	if (!res.ok) {
		box.setIcon(QMessageBox::Warning);
		box.setText("Couldn't check for updates right now.\n\n"
			    "Check your internet connection and try again later.");
		box.addButton(QMessageBox::Close);
	} else if (newer) {
		box.setIcon(QMessageBox::Information);
		box.setText(QString("A new version of Helix ToolKit is available: %1\n"
				    "You have %2 installed.\n\n"
				    "Download opens the release page in your browser. Close "
				    "OBS before running the new installer.")
				    .arg(QString::fromStdString(res.tag),
					 QString::fromStdString(installed)));
		download = box.addButton("Download", QMessageBox::AcceptRole);
		skip = box.addButton("Skip This Version", QMessageBox::DestructiveRole);
		box.addButton("Remind Me Later", QMessageBox::RejectRole);
		box.setDefaultButton(download);
	} else {
		box.setIcon(QMessageBox::Information);
		box.setText(QString("Helix ToolKit is up to date (%1).")
				    .arg(QString::fromStdString(installed)));
		box.addButton(QMessageBox::Ok);
	}

	box.exec();

	s.auto_check = autoBox->isChecked();
	if (box.clickedButton() == download) {
		QDesktopServices::openUrl(QUrl(QString::fromStdString(res.url)));
	} else if (skip && box.clickedButton() == skip) {
		s.skipped_version = res.tag;
	}
	save_settings(s);
}

/* ---- orchestration ------------------------------------------------------ */

void start_check(bool manual)
{
	if (g_shutdown || !g_ctx)
		return;

	std::string installed = installed_version();
	if (installed.empty()) {
		/* Dev build or files copied by hand: nothing meaningful to compare. */
		if (manual)
			QMessageBox::information(
				main_window(), "Helix ToolKit Updates",
				"This copy of Helix ToolKit wasn't installed with the official "
				"installer, so its version can't be checked.");
		return;
	}

	if (g_busy.exchange(true))
		return; /* a check is already running */

	if (g_thread.joinable())
		g_thread.join();

	g_thread = std::thread([installed, manual]() {
		update_result res = fetch_latest_release();
		if (g_shutdown) {
			g_busy = false;
			return;
		}
		QMetaObject::invokeMethod(
			g_ctx,
			[res, installed, manual]() {
				g_busy = false;
				if (!g_shutdown)
					show_result(res, installed, manual);
			},
			Qt::QueuedConnection);
	});
}

void on_frontend_event(enum obs_frontend_event event, void *)
{
	if (event != OBS_FRONTEND_EVENT_FINISHED_LOADING)
		return;
	update_settings s = load_settings();
	if (!s.auto_check)
		return;
	if ((int64_t)time(nullptr) - s.last_check < UPDATE_CHECK_INTERVAL_SEC)
		return;
	start_check(false);
}

void on_menu_check(void *)
{
	start_check(true);
}

} // namespace

void helixtools_update_init(void)
{
	g_ctx = new QObject();
	obs_frontend_add_event_callback(on_frontend_event, nullptr);
	obs_frontend_add_tools_menu_item("Check for Helix ToolKit Updates...", on_menu_check,
					 nullptr);
}

void helixtools_update_shutdown(void)
{
	g_shutdown = true;
	obs_frontend_remove_event_callback(on_frontend_event, nullptr);

	/* Abort any in-flight request so the join below returns promptly. */
	{
		std::lock_guard<std::mutex> lock(g_session_mutex);
		if (g_session) {
			WinHttpCloseHandle(g_session);
			g_session = nullptr;
		}
	}
	if (g_thread.joinable())
		g_thread.join();

	delete g_ctx;
	g_ctx = nullptr;
}
