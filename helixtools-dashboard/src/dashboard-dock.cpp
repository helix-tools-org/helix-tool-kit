/*
 * dashboard-dock.cpp - Sidebar (logo + platform name) + one persistent
 * browser widget per platform for the Dashboard dock
 *
 * One cookie jar shared across all platforms (cookies are
 * domain-scoped, so this works fine for all of them — same reasoning
 * helixtools-social's sidebar dock already relies on), but each
 * platform gets its own real QCefWidget, created lazily on first
 * selection and kept alive afterward in a QStackedWidget. Selecting a
 * sidebar entry just switches which already-loaded widget is shown —
 * never a fresh navigation — and persists the choice to the profile
 * config so the dock reopens on the same platform next time.
 *
 * This replaces an earlier design (see SESSION_NOTES.md 2026-09-23)
 * that shared a single QCefWidget across all platforms and tracked
 * each one's last-visited URL, reloading it with setURL() on switch.
 * That turned out not to reliably preserve where you were: these
 * dashboards are SPAs, and cold-loading a saved deep link (e.g.
 * Twitch's Stream Manager) via a fresh top-level navigation isn't the
 * same as reaching it by clicking within the already-loaded app —
 * confirmed bouncing back to the platform's home page on both Twitch
 * and Rumble Studio. Keeping each platform's browser genuinely alive
 * sidesteps that entirely, at the deliberately accepted cost of up to
 * NUM_DASHBOARD_PLATFORMS live CEF renderer processes at once instead
 * of one.
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

#include "dashboard-dock.h"
#include "dashboard-sites.h"
#include "vendor/browser-panel.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QIcon>
#include <QSize>

#include <cstring>

static const char *CONFIG_SECTION = "Dashboard";
static const char *CONFIG_KEY_PLATFORM = "Platform";

static int findPlatformIndex(const char *label)
{
	if (!label)
		return -1;
	for (size_t i = 0; i < NUM_DASHBOARD_PLATFORMS; i++) {
		if (strcmp(dashboardPlatforms[i].label, label) == 0)
			return (int)i;
	}
	return -1;
}

DashboardDock::DashboardDock(QCef *cef_, QWidget *parent) : QWidget(parent), cef(cef_)
{
	auto *mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	sidebar = new QListWidget(this);
	sidebar->setFixedWidth(150);
	sidebar->setIconSize(QSize(24, 24));
	sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	sidebar->setStyleSheet(
		"QListWidget { background: #1A1E24; border: none; "
		"  border-right: 1px solid #2A2E34; }"
		"QListWidget::item { padding: 8px 10px; color: #ccc; }"
		"QListWidget::item:selected { background: #7C3AED; "
		"  color: white; }"
		"QListWidget::item:hover:!selected { background: #252930; }");

	for (size_t i = 0; i < NUM_DASHBOARD_PLATFORMS; i++) {
		auto *item = new QListWidgetItem(sidebar);
		item->setText(dashboardPlatforms[i].label);

		BPtr<char> iconPath = obs_module_file(dashboardPlatforms[i].iconFile);
		if (iconPath)
			item->setIcon(QIcon(QString::fromUtf8(iconPath.Get())));
	}

	/* Default to the first entry (Rumble Studio) unless the profile
	 * config already has a saved choice from a previous session. */
	int startIndex = 0;
	config_t *config = obs_frontend_get_profile_config();
	if (config) {
		const char *saved =
			config_get_string(config, CONFIG_SECTION, CONFIG_KEY_PLATFORM);
		int savedIndex = findPlatformIndex(saved);
		if (savedIndex >= 0)
			startIndex = savedIndex;
	}
	sidebar->setCurrentRow(startIndex);

	mainLayout->addWidget(sidebar);

	BPtr<char> cookiePath = obs_module_config_path("cookies");
	cookies = cef->create_cookie_manager(cookiePath ? cookiePath.Get() : "",
					     true);

	browsers.resize((int)NUM_DASHBOARD_PLATFORMS);

	stack = new QStackedWidget(this);
	mainLayout->addWidget(stack, 1);

	ensureBrowserCreated(startIndex);
	stack->setCurrentWidget(browsers[startIndex]);

	connect(sidebar, &QListWidget::currentItemChanged, this,
		&DashboardDock::onPlatformSelected);
}

DashboardDock::~DashboardDock()
{
	if (cookies)
		delete cookies;
}

void DashboardDock::ensureBrowserCreated(int index)
{
	if (browsers[index])
		return;

	QCefWidget *widget = cef->create_widget(this, dashboardPlatforms[index].url, cookies);
	browsers[index] = widget;
	stack->addWidget(widget);
}

void DashboardDock::onPlatformSelected()
{
	int index = sidebar->currentRow();
	if (index < 0 || (size_t)index >= NUM_DASHBOARD_PLATFORMS)
		return;

	ensureBrowserCreated(index);
	stack->setCurrentWidget(browsers[index]);

	config_t *config = obs_frontend_get_profile_config();
	if (!config)
		return;
	config_set_string(config, CONFIG_SECTION, CONFIG_KEY_PLATFORM,
			   dashboardPlatforms[index].label);
	config_save(config);
}
