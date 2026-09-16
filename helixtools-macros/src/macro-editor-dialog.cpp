/*
 * macro-editor-dialog.cpp - Qt6 visual macro editor dialog
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

#include "macro-editor-dialog.h"
#include "macro-action-widget.h"
#include "macro-engine.h"
#include "macro-storage.h"
#include "helixtools-macros.h"

#include <obs-module.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>

MacroEditorDialog::MacroEditorDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(obs_module_text("HelixToolsMacros.EditorTitle"));
	setMinimumSize(800, 500);
	resize(900, 550);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	/* ---- Main layout with splitter ---- */
	auto *mainLayout = new QVBoxLayout(this);

	auto *splitter = new QSplitter(Qt::Horizontal, this);

	/* ---- Left panel: Macro list ---- */
	auto *leftWidget = new QWidget(this);
	auto *leftLayout = new QVBoxLayout(leftWidget);
	leftLayout->setContentsMargins(0, 0, 0, 0);

	auto *macroLabel = new QLabel(
		obs_module_text("HelixToolsMacros.MacroList"), leftWidget);
	macroLabel->setStyleSheet("font-weight: bold;");
	leftLayout->addWidget(macroLabel);

	macroList = new QListWidget(leftWidget);
	macroList->setMinimumWidth(180);
	leftLayout->addWidget(macroList);

	auto *macroButtonLayout = new QHBoxLayout();
	btnAddMacro = new QPushButton(
		obs_module_text("HelixToolsMacros.Add"), leftWidget);
	btnRemoveMacro = new QPushButton(
		obs_module_text("HelixToolsMacros.Remove"), leftWidget);
	btnRenameMacro = new QPushButton(
		obs_module_text("HelixToolsMacros.Rename"), leftWidget);
	btnDuplicateMacro = new QPushButton(
		obs_module_text("HelixToolsMacros.Duplicate"), leftWidget);

	macroButtonLayout->addWidget(btnAddMacro);
	macroButtonLayout->addWidget(btnRemoveMacro);
	macroButtonLayout->addWidget(btnRenameMacro);
	macroButtonLayout->addWidget(btnDuplicateMacro);
	leftLayout->addLayout(macroButtonLayout);

	splitter->addWidget(leftWidget);

	/* ---- Right panel: Action list ---- */
	auto *rightWidget = new QWidget(this);
	auto *rightLayout = new QVBoxLayout(rightWidget);
	rightLayout->setContentsMargins(0, 0, 0, 0);

	actionListLabel = new QLabel(
		obs_module_text("HelixToolsMacros.ActionsFor"), rightWidget);
	actionListLabel->setStyleSheet("font-weight: bold;");
	rightLayout->addWidget(actionListLabel);

	actionList = new QListWidget(rightWidget);
	actionList->setDragDropMode(QAbstractItemView::InternalMove);
	rightLayout->addWidget(actionList);

	auto *actionButtonLayout = new QHBoxLayout();
	btnAddAction = new QPushButton(
		obs_module_text("HelixToolsMacros.AddAction"), rightWidget);
	btnRemoveAction = new QPushButton(
		obs_module_text("HelixToolsMacros.RemoveAction"), rightWidget);
	btnMoveUp = new QPushButton(
		obs_module_text("HelixToolsMacros.MoveUp"), rightWidget);
	btnMoveDown = new QPushButton(
		obs_module_text("HelixToolsMacros.MoveDown"), rightWidget);

	actionButtonLayout->addWidget(btnAddAction);
	actionButtonLayout->addWidget(btnRemoveAction);
	actionButtonLayout->addWidget(btnMoveUp);
	actionButtonLayout->addWidget(btnMoveDown);
	rightLayout->addLayout(actionButtonLayout);

	splitter->addWidget(rightWidget);

	/* Set splitter proportions (30/70) */
	splitter->setStretchFactor(0, 30);
	splitter->setStretchFactor(1, 70);

	mainLayout->addWidget(splitter, 1);

	/* ---- Bottom button bar ---- */
	auto *bottomLayout = new QHBoxLayout();

	btnTestRun = new QPushButton(
		obs_module_text("HelixToolsMacros.TestRun"), this);
	btnStop = new QPushButton(
		obs_module_text("HelixToolsMacros.Stop"), this);
	bottomLayout->addWidget(btnTestRun);
	bottomLayout->addWidget(btnStop);

	bottomLayout->addStretch(1);

	btnSave = new QPushButton(
		obs_module_text("HelixToolsMacros.Save"), this);
	btnClose = new QPushButton(
		obs_module_text("HelixToolsMacros.Close"), this);
	bottomLayout->addWidget(btnSave);
	bottomLayout->addWidget(btnClose);

	mainLayout->addLayout(bottomLayout);

	/* ---- Connect signals ---- */
	connect(macroList, &QListWidget::currentRowChanged, this,
		&MacroEditorDialog::OnMacroSelectionChanged);
	connect(actionList, &QListWidget::currentRowChanged, this,
		&MacroEditorDialog::UpdateButtonStates);

	connect(btnAddMacro, &QPushButton::clicked, this,
		&MacroEditorDialog::OnAddMacro);
	connect(btnRemoveMacro, &QPushButton::clicked, this,
		&MacroEditorDialog::OnRemoveMacro);
	connect(btnRenameMacro, &QPushButton::clicked, this,
		&MacroEditorDialog::OnRenameMacro);
	connect(btnDuplicateMacro, &QPushButton::clicked, this,
		&MacroEditorDialog::OnDuplicateMacro);

	connect(btnAddAction, &QPushButton::clicked, this,
		&MacroEditorDialog::OnAddAction);
	connect(btnRemoveAction, &QPushButton::clicked, this,
		&MacroEditorDialog::OnRemoveAction);
	connect(btnMoveUp, &QPushButton::clicked, this,
		&MacroEditorDialog::OnMoveActionUp);
	connect(btnMoveDown, &QPushButton::clicked, this,
		&MacroEditorDialog::OnMoveActionDown);

	connect(btnTestRun, &QPushButton::clicked, this,
		&MacroEditorDialog::OnTestRun);
	connect(btnStop, &QPushButton::clicked, this,
		&MacroEditorDialog::OnStop);
	connect(btnSave, &QPushButton::clicked, this,
		&MacroEditorDialog::OnSave);
	connect(btnClose, &QPushButton::clicked, this, &QDialog::close);

	/* Populate */
	PopulateMacroList();
	UpdateButtonStates();
}

MacroEditorDialog::~MacroEditorDialog() {}

void MacroEditorDialog::ToggleShowHide()
{
	if (isVisible()) {
		hide();
	} else {
		PopulateMacroList();
		show();
		raise();
		activateWindow();
	}
}

/* ---- Macro List ---- */

void MacroEditorDialog::PopulateMacroList()
{
	macroList->blockSignals(true);
	macroList->clear();
	for (const auto *m : helixtools_macros_get_all())
		macroList->addItem(QString::fromStdString(m->name));
	macroList->blockSignals(false);

	if (macroList->count() > 0)
		macroList->setCurrentRow(0);
	else
		ClearActionList();

	UpdateButtonStates();
}

HelixToolsMacro *MacroEditorDialog::GetSelectedMacro()
{
	int row = macroList->currentRow();
	if (row < 0)
		return nullptr;

	auto &macros = helixtools_macros_get_all();
	if (row >= (int)macros.size())
		return nullptr;
	return macros[row];
}

/* ---- Action List ---- */

void MacroEditorDialog::PopulateActionList(HelixToolsMacro *macro)
{
	ClearActionList();

	if (!macro) {
		actionListLabel->setText(
			obs_module_text("HelixToolsMacros.ActionsFor"));
		UpdateButtonStates();
		return;
	}

	QString label = QString("%1 \"%2\"")
				.arg(obs_module_text("HelixToolsMacros.ActionsFor"))
				.arg(QString::fromStdString(macro->name));
	actionListLabel->setText(label);

	for (const auto &action : macro->actions) {
		auto *widget = new MacroActionWidget(action);

		auto *item = new QListWidgetItem(actionList);
		item->setSizeHint(QSize(0, 36));
		actionList->setItemWidget(item, widget);
	}

	UpdateButtonStates();
}

void MacroEditorDialog::ClearActionList()
{
	actionList->clear();
}

void MacroEditorDialog::SyncActionsFromWidgets(HelixToolsMacro *macro)
{
	if (!macro)
		return;

	macro->actions.clear();
	for (int i = 0; i < actionList->count(); i++) {
		auto *item = actionList->item(i);
		auto *widget = qobject_cast<MacroActionWidget *>(
			actionList->itemWidget(item));
		if (widget)
			macro->actions.push_back(widget->GetAction());
	}
}

void MacroEditorDialog::UpdateButtonStates()
{
	bool hasMacro = GetSelectedMacro() != nullptr;
	bool hasActions = actionList->count() > 0;
	bool hasSelectedAction = actionList->currentRow() >= 0;

	btnRemoveMacro->setEnabled(hasMacro);
	btnRenameMacro->setEnabled(hasMacro);
	btnDuplicateMacro->setEnabled(hasMacro);

	btnAddAction->setEnabled(hasMacro);
	btnRemoveAction->setEnabled(hasSelectedAction);
	btnMoveUp->setEnabled(hasSelectedAction &&
			      actionList->currentRow() > 0);
	btnMoveDown->setEnabled(hasSelectedAction &&
				actionList->currentRow() <
					actionList->count() - 1);

	btnTestRun->setEnabled(hasMacro && hasActions);
	btnStop->setEnabled(hasMacro);
}

/* ---- Slots: Macro Management ---- */

void MacroEditorDialog::OnMacroSelectionChanged()
{
	PopulateActionList(GetSelectedMacro());
}

void MacroEditorDialog::OnAddMacro()
{
	bool ok;
	QString name = QInputDialog::getText(
		this, obs_module_text("HelixToolsMacros.AddMacroTitle"),
		obs_module_text("HelixToolsMacros.MacroName"), QLineEdit::Normal,
		obs_module_text("HelixToolsMacros.NewMacroDefault"), &ok);

	if (!ok || name.trimmed().isEmpty())
		return;

	HelixToolsMacro *macro =
		helixtools_macros_create(name.trimmed().toUtf8().constData());
	helixtools_macros_register_hotkey(macro);

	PopulateMacroList();
	macroList->setCurrentRow(macroList->count() - 1);
}

void MacroEditorDialog::OnRemoveMacro()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	auto result = QMessageBox::question(
		this, obs_module_text("HelixToolsMacros.RemoveMacroTitle"),
		QString(obs_module_text("HelixToolsMacros.RemoveConfirm"))
			.arg(QString::fromStdString(macro->name)));

	if (result != QMessageBox::Yes)
		return;

	helixtools_macros_cancel(macro);
	helixtools_macros_unregister_hotkey(macro);
	helixtools_macros_remove(macro);

	PopulateMacroList();
}

void MacroEditorDialog::OnRenameMacro()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	bool ok;
	QString name = QInputDialog::getText(
		this, obs_module_text("HelixToolsMacros.RenameMacroTitle"),
		obs_module_text("HelixToolsMacros.MacroName"), QLineEdit::Normal,
		QString::fromStdString(macro->name), &ok);

	if (!ok || name.trimmed().isEmpty())
		return;

	macro->name = name.trimmed().toUtf8().constData();

	int row = macroList->currentRow();
	PopulateMacroList();
	macroList->setCurrentRow(row);
}

void MacroEditorDialog::OnDuplicateMacro()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	std::string newName = macro->name + " (Copy)";
	HelixToolsMacro *copy = helixtools_macros_create(newName.c_str());
	copy->actions = macro->actions;
	helixtools_macros_register_hotkey(copy);

	PopulateMacroList();
	macroList->setCurrentRow(macroList->count() - 1);
}

/* ---- Slots: Action Management ---- */

void MacroEditorDialog::OnAddAction()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	/* Sync current widget state first */
	SyncActionsFromWidgets(macro);

	/* Add default action (Delay) */
	HelixToolsAction action;
	action.type = HelixToolsActionType::Delay;
	action.delay_seconds = 1.0f;
	macro->actions.push_back(action);

	/* Rebuild action list to show new action */
	PopulateActionList(macro);

	/* Select the newly added action */
	actionList->setCurrentRow(actionList->count() - 1);
}

void MacroEditorDialog::OnRemoveAction()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	int row = actionList->currentRow();
	if (row < 0)
		return;

	/* Sync, remove, rebuild */
	SyncActionsFromWidgets(macro);
	if (row < (int)macro->actions.size())
		macro->actions.erase(macro->actions.begin() + row);

	PopulateActionList(macro);

	if (row > 0 && row >= actionList->count())
		actionList->setCurrentRow(actionList->count() - 1);
	else if (actionList->count() > 0)
		actionList->setCurrentRow(row < actionList->count() ? row : 0);
}

void MacroEditorDialog::OnMoveActionUp()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	int row = actionList->currentRow();
	if (row <= 0)
		return;

	SyncActionsFromWidgets(macro);
	std::swap(macro->actions[row], macro->actions[row - 1]);

	PopulateActionList(macro);
	actionList->setCurrentRow(row - 1);
}

void MacroEditorDialog::OnMoveActionDown()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	int row = actionList->currentRow();
	if (row < 0 || row >= actionList->count() - 1)
		return;

	SyncActionsFromWidgets(macro);
	std::swap(macro->actions[row], macro->actions[row + 1]);

	PopulateActionList(macro);
	actionList->setCurrentRow(row + 1);
}

/* ---- Slots: Run / Save ---- */

void MacroEditorDialog::OnTestRun()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (!macro)
		return;

	/* Sync latest edits before running */
	SyncActionsFromWidgets(macro);
	helixtools_macros_execute(macro);
}

void MacroEditorDialog::OnStop()
{
	HelixToolsMacro *macro = GetSelectedMacro();
	if (macro)
		helixtools_macros_cancel(macro);
}

void MacroEditorDialog::OnSave()
{
	/* Sync actions from the currently selected macro's widgets */
	HelixToolsMacro *macro = GetSelectedMacro();
	if (macro)
		SyncActionsFromWidgets(macro);

	helixtools_macros_save();

	/* Brief visual feedback */
	btnSave->setText(obs_module_text("HelixToolsMacros.Saved"));
	QTimer::singleShot(1500, [this] {
		btnSave->setText(obs_module_text("HelixToolsMacros.Save"));
	});
}
