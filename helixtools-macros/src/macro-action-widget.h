/*
 * macro-action-widget.h - Custom widget for a single macro action step
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

#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>

class MacroActionWidget : public QWidget {
	Q_OBJECT

public:
	explicit MacroActionWidget(const HelixToolsAction &action,
				   QWidget *parent = nullptr);
	~MacroActionWidget() override;

	/* Read the current action state from the widget controls */
	HelixToolsAction GetAction() const;

	/* Update the widget to reflect a new action */
	void SetAction(const HelixToolsAction &action);

private slots:
	void OnActionTypeChanged(int index);

private:
	void BuildUI();
	void RebuildParameterWidgets();
	void PopulateSceneCombo();
	void PopulateSourceCombo(const QString &sceneName);
	void PopulateTransitionCombo();

	HelixToolsActionType currentType;

	/* Action type selector */
	QComboBox *typeCombo;

	/* Parameter widgets (shown/hidden based on type) */
	QComboBox *sceneCombo;
	QComboBox *sourceCombo;
	QComboBox *transitionCombo;
	QDoubleSpinBox *delaySpinBox;
	QDoubleSpinBox *volumeSpinBox;
	QSpinBox *transitionDurationSpinBox;
	QLabel *paramLabel;

	QHBoxLayout *mainLayout;
};
