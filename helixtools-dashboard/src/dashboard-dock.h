/*
 * dashboard-dock.h - Sidebar (logo + platform name) + browser widget
 * for the Dashboard dock
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

#include <QWidget>
#include <QVector>

struct QCef;
struct QCefCookieManager;
class QCefWidget;
class QListWidget;
class QStackedWidget;

class DashboardDock : public QWidget {
	Q_OBJECT

public:
	explicit DashboardDock(QCef *cef, QWidget *parent = nullptr);
	~DashboardDock() override;

private slots:
	void onPlatformSelected();

private:
	void ensureBrowserCreated(int index);

	QCef *cef;
	QCefCookieManager *cookies = nullptr;
	QListWidget *sidebar = nullptr;
	QStackedWidget *stack = nullptr;

	/* One real, persistent QCefWidget per platform, created lazily the
	 * first time its sidebar entry is selected and kept alive for the
	 * rest of the dock's lifetime (nullptr until then). Switching
	 * platforms is then just showing/hiding an already-loaded widget via
	 * the stack, exactly like real browser tabs — never a fresh
	 * navigation — so each platform's own in-app navigation (e.g. Twitch
	 * -> Stream Manager) is preserved automatically for free. This
	 * replaces an earlier single-shared-widget design that stored each
	 * platform's last-visited URL and reloaded it on switch: cold-loading
	 * a saved deep link that way turned out not to work reliably, since
	 * these dashboards are SPAs whose routing doesn't always support
	 * being loaded directly at an internal sub-page rather than reached
	 * by clicking within the already-loaded app (confirmed reproducing on
	 * both Twitch and Rumble Studio — see SESSION_NOTES.md 2026-09-23).
	 * The tradeoff, accepted deliberately: up to NUM_DASHBOARD_PLATFORMS
	 * live CEF renderer processes at once instead of one. */
	QVector<QCefWidget *> browsers;
};
