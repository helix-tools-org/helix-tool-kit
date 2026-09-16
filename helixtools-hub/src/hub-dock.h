/*
 * hub-dock.h - HelixTools Hub browser dock
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
#include <QVBoxLayout>

struct QCef;
struct QCefCookieManager;
class QCefWidget;

class HubDock : public QWidget {
	Q_OBJECT

public:
	explicit HubDock(QCef *cef, QWidget *parent = nullptr);
	~HubDock() override;

private slots:
	void onTitleChanged(const QString &title);

private:
	QCef *cef;
	QCefCookieManager *cookies = nullptr;
	QCefWidget *browser = nullptr;

	void handlePushCommand(const QString &json);
	void handleAuthCommand(const QString &json);
};
