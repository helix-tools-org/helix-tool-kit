/*
 * restream-dock.h - Qt6 destinations editor dock
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

#include "helixtools-restream.h"

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

class RestreamDock : public QWidget {
	Q_OBJECT

public:
	explicit RestreamDock(struct restream_context *ctx, QWidget *parent = nullptr);
	~RestreamDock() override;

private slots:
	void OnSelectionChanged();
	void OnAdd();
	void OnEdit();
	void OnRemove();
	void OnRenameMainStream();
	void RefreshLiveStatus();

private:
	void PopulateList();
	void UpdateButtonStates();

	struct restream_context *ctx;

	QLabel *mainStreamLabel;
	QPushButton *btnRenameMainStream;
	QLabel *streamingNotice;
	QListWidget *destList;
	QPushButton *btnAdd;
	QPushButton *btnEdit;
	QPushButton *btnRemove;
	QTimer *refreshTimer;
};
