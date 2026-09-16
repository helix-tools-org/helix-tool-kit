/*
 * macro-editor-dialog.h - Qt6 visual macro editor dialog
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

#include "macro-types.h"

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

class MacroActionWidget;

class MacroEditorDialog : public QDialog {
	Q_OBJECT

public:
	explicit MacroEditorDialog(QWidget *parent = nullptr);
	~MacroEditorDialog() override;

	void ToggleShowHide();

private slots:
	void OnMacroSelectionChanged();
	void OnAddMacro();
	void OnRemoveMacro();
	void OnRenameMacro();
	void OnDuplicateMacro();

	void OnAddAction();
	void OnRemoveAction();
	void OnMoveActionUp();
	void OnMoveActionDown();

	void OnTestRun();
	void OnStop();
	void OnSave();

private:
	void PopulateMacroList();
	void PopulateActionList(HelixToolsMacro *macro);
	void ClearActionList();
	HelixToolsMacro *GetSelectedMacro();
	void SyncActionsFromWidgets(HelixToolsMacro *macro);
	void UpdateButtonStates();

	/* Left panel - macro list */
	QListWidget *macroList;
	QPushButton *btnAddMacro;
	QPushButton *btnRemoveMacro;
	QPushButton *btnRenameMacro;
	QPushButton *btnDuplicateMacro;

	/* Right panel - action list */
	QListWidget *actionList;
	QLabel *actionListLabel;
	QPushButton *btnAddAction;
	QPushButton *btnRemoveAction;
	QPushButton *btnMoveUp;
	QPushButton *btnMoveDown;

	/* Bottom buttons */
	QPushButton *btnTestRun;
	QPushButton *btnStop;
	QPushButton *btnSave;
	QPushButton *btnClose;
};
