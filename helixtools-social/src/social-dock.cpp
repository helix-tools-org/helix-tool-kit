/*
 * social-dock.cpp - Sidebar + browser widget for the Social Media dock
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

#include "social-dock.h"
#include "social-sites.h"
#include "vendor/browser-panel.hpp"

#include <obs-module.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QFont>
#include <QColor>

SocialDock::SocialDock(QCef *cef_, QWidget *parent) : QWidget(parent), cef(cef_)
{
	auto *mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	auto *leftPanel = new QWidget(this);
	auto *leftLayout = new QVBoxLayout(leftPanel);
	leftLayout->setContentsMargins(0, 0, 0, 0);
	leftLayout->setSpacing(0);
	leftPanel->setFixedWidth(140);

	sidebar = new QListWidget(leftPanel);
	sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	sidebar->setStyleSheet(
		"QListWidget { background: #1A1E24; border: none; "
		"  border-right: 1px solid #2A2E34; }"
		"QListWidget::item { padding: 8px 12px; color: #ccc; }"
		"QListWidget::item:selected { background: #7C3AED; "
		"  color: white; }"
		"QListWidget::item:hover:!selected { background: #252930; }");

	int firstClickableRow = -1;
	for (size_t i = 0; i < NUM_SOCIAL_MEDIA_SITES; i++) {
		const auto &site = socialMediaSites[i];
		auto *item = new QListWidgetItem(site.label, sidebar);
		if (site.isHeader) {
			item->setFlags(Qt::NoItemFlags);
			QFont font = item->font();
			font.setBold(true);
			font.setPointSize(font.pointSize() + 1);
			item->setFont(font);
			item->setForeground(QColor("#888"));
		} else {
			item->setData(Qt::UserRole, QString(site.url));
			if (firstClickableRow < 0)
				firstClickableRow = (int)i;
		}
	}

	urlEdit = new QLineEdit(leftPanel);
	urlEdit->setPlaceholderText("Custom URL...");
	urlEdit->setStyleSheet(
		"QLineEdit { background: #1A1E24; color: #ccc; "
		"  border: none; border-top: 1px solid #2A2E34; "
		"  border-right: 1px solid #2A2E34; "
		"  padding: 6px 8px; }");

	leftLayout->addWidget(sidebar, 1);
	leftLayout->addWidget(urlEdit);

	QString initialUrl = "about:blank";
	if (firstClickableRow >= 0) {
		sidebar->setCurrentRow(firstClickableRow);
		initialUrl = sidebar->item(firstClickableRow)
				     ->data(Qt::UserRole)
				     .toString();
	}

	mainLayout->addWidget(leftPanel);

	BPtr<char> cookiePath = obs_module_config_path("cookies");
	cookies = cef->create_cookie_manager(cookiePath ? cookiePath.Get() : "",
					     true);
	browser = cef->create_widget(this, initialUrl.toUtf8().constData(),
				     cookies);
	mainLayout->addWidget(browser, 1);

	connect(sidebar, &QListWidget::currentItemChanged, this,
		&SocialDock::onSidebarSelectionChanged);
	connect(urlEdit, &QLineEdit::returnPressed, this,
		&SocialDock::onUrlEntered);
}

SocialDock::~SocialDock()
{
	if (cookies)
		delete cookies;
}

void SocialDock::onSidebarSelectionChanged()
{
	QListWidgetItem *current = sidebar->currentItem();
	if (!current)
		return;
	QString url = current->data(Qt::UserRole).toString();
	if (url.isEmpty())
		return;
	browser->setURL(url.toUtf8().constData());
	urlEdit->clear();
}

void SocialDock::onUrlEntered()
{
	QString url = urlEdit->text().trimmed();
	if (url.isEmpty())
		return;
	if (!url.startsWith("http://") && !url.startsWith("https://"))
		url = "https://" + url;
	sidebar->clearSelection();
	sidebar->setCurrentItem(nullptr);
	browser->setURL(url.toUtf8().constData());
}
