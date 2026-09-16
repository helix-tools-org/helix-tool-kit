/*
 * macro-action-widget.cpp - Custom widget for a single macro action step
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

#include "macro-action-widget.h"

#include <obs.h>
#include <obs-frontend-api.h>

#include <QHBoxLayout>
#include <QAbstractItemView>

/* Set a combo box's minimum width based on its widest item text,
 * and force its dropdown popup to be at least that wide via stylesheet. */
static void SetComboMinWidth(QComboBox *combo, int extraPadding = 50)
{
	int maxWidth = 0;
	QFontMetrics fm(combo->font());
	for (int i = 0; i < combo->count(); i++) {
		int w = fm.horizontalAdvance(combo->itemText(i));
		if (w > maxWidth)
			maxWidth = w;
	}
	maxWidth += extraPadding;
	combo->setMinimumWidth(maxWidth);
	combo->setStyleSheet(
		QString("QComboBox QAbstractItemView { min-width: %1px; }")
			.arg(maxWidth));
}

/* ---- OBS Scene/Source Helpers ---- */

static QStringList get_scene_names()
{
	QStringList names;
	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	for (size_t i = 0; i < scenes.sources.num; i++) {
		const char *name =
			obs_source_get_name(scenes.sources.array[i]);
		if (name)
			names.append(QString::fromUtf8(name));
	}
	obs_frontend_source_list_free(&scenes);
	return names;
}

static QStringList get_source_names_in_scene(const QString &sceneName)
{
	QStringList names;
	obs_source_t *scene_src =
		obs_get_source_by_name(sceneName.toUtf8().constData());
	if (!scene_src)
		return names;

	obs_scene_t *scene = obs_scene_from_source(scene_src);
	if (scene) {
		obs_scene_enum_items(
			scene,
			[](obs_scene_t *, obs_sceneitem_t *item,
			   void *param) -> bool {
				auto *list =
					static_cast<QStringList *>(param);
				obs_source_t *src =
					obs_sceneitem_get_source(item);
				const char *n = obs_source_get_name(src);
				if (n)
					list->append(QString::fromUtf8(n));
				return true;
			},
			&names);
	}

	obs_source_release(scene_src);
	return names;
}

static QStringList get_audio_source_names()
{
	QStringList names;
	auto cb = [](void *param, obs_source_t *source) -> bool {
		auto *list = static_cast<QStringList *>(param);
		uint32_t flags = obs_source_get_output_flags(source);
		if (flags & OBS_SOURCE_AUDIO) {
			const char *n = obs_source_get_name(source);
			if (n)
				list->append(QString::fromUtf8(n));
		}
		return true;
	};
	obs_enum_sources(cb, &names);
	return names;
}

static QStringList get_transition_names()
{
	QStringList names;
	struct obs_frontend_source_list transitions = {};
	obs_frontend_get_transitions(&transitions);
	for (size_t i = 0; i < transitions.sources.num; i++) {
		const char *name =
			obs_source_get_name(transitions.sources.array[i]);
		if (name)
			names.append(QString::fromUtf8(name));
	}
	obs_frontend_source_list_free(&transitions);
	return names;
}

/* ---- Action Type requires scene param ---- */

static bool type_needs_scene(HelixToolsActionType t)
{
	return t == HelixToolsActionType::SwitchScene ||
	       t == HelixToolsActionType::ShowSource ||
	       t == HelixToolsActionType::HideSource ||
	       t == HelixToolsActionType::ToggleSource;
}

static bool type_needs_source(HelixToolsActionType t)
{
	return t == HelixToolsActionType::ShowSource ||
	       t == HelixToolsActionType::HideSource ||
	       t == HelixToolsActionType::ToggleSource;
}

static bool type_needs_audio_source(HelixToolsActionType t)
{
	return t == HelixToolsActionType::MuteSource ||
	       t == HelixToolsActionType::UnmuteSource ||
	       t == HelixToolsActionType::ToggleAudio ||
	       t == HelixToolsActionType::SetVolume;
}

/* ---- Constructor ---- */

MacroActionWidget::MacroActionWidget(const HelixToolsAction &action,
				     QWidget *parent)
	: QWidget(parent),
	  currentType(action.type),
	  typeCombo(nullptr),
	  sceneCombo(nullptr),
	  sourceCombo(nullptr),
	  transitionCombo(nullptr),
	  delaySpinBox(nullptr),
	  volumeSpinBox(nullptr),
	  transitionDurationSpinBox(nullptr),
	  paramLabel(nullptr),
	  mainLayout(nullptr)
{
	BuildUI();
	SetAction(action);
}

MacroActionWidget::~MacroActionWidget() {}

/* ---- Build UI ---- */

void MacroActionWidget::BuildUI()
{
	mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(4, 2, 4, 2);

	/* Action type dropdown */
	typeCombo = new QComboBox(this);

	/* Add all action types */
	for (int i = 0; i <= (int)HelixToolsActionType::SetTransition; i++) {
		auto t = static_cast<HelixToolsActionType>(i);
		typeCombo->addItem(QString::fromUtf8(helixtools_action_type_name(t)),
				   i);
	}
	SetComboMinWidth(typeCombo);
	connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &MacroActionWidget::OnActionTypeChanged);

	mainLayout->addWidget(typeCombo);

	/* Scene combo */
	sceneCombo = new QComboBox(this);
	sceneCombo->setVisible(false);
	mainLayout->addWidget(sceneCombo);

	connect(sceneCombo,
		QOverload<int>::of(&QComboBox::currentIndexChanged),
		[this](int) {
			if (type_needs_source(currentType))
				PopulateSourceCombo(sceneCombo->currentText());
		});

	/* Source combo (for scene items) */
	sourceCombo = new QComboBox(this);
	sourceCombo->setVisible(false);
	mainLayout->addWidget(sourceCombo);

	/* Transition combo */
	transitionCombo = new QComboBox(this);
	transitionCombo->setVisible(false);
	mainLayout->addWidget(transitionCombo);

	/* Delay spin box */
	delaySpinBox = new QDoubleSpinBox(this);
	delaySpinBox->setRange(0.1, 3600.0);
	delaySpinBox->setSingleStep(0.5);
	delaySpinBox->setDecimals(1);
	delaySpinBox->setSuffix(" sec");
	delaySpinBox->setVisible(false);
	mainLayout->addWidget(delaySpinBox);

	/* Volume spin box */
	volumeSpinBox = new QDoubleSpinBox(this);
	volumeSpinBox->setRange(0.0, 200.0);
	volumeSpinBox->setSingleStep(5.0);
	volumeSpinBox->setDecimals(0);
	volumeSpinBox->setSuffix("%");
	volumeSpinBox->setVisible(false);
	mainLayout->addWidget(volumeSpinBox);

	/* Transition duration spin box */
	transitionDurationSpinBox = new QSpinBox(this);
	transitionDurationSpinBox->setRange(50, 10000);
	transitionDurationSpinBox->setSingleStep(50);
	transitionDurationSpinBox->setSuffix(" ms");
	transitionDurationSpinBox->setVisible(false);
	mainLayout->addWidget(transitionDurationSpinBox);

	/* Info label (for parameterless actions) */
	paramLabel = new QLabel(this);
	paramLabel->setVisible(false);
	mainLayout->addWidget(paramLabel);

	mainLayout->addStretch(1);
}

/* ---- Populate Helpers ---- */

void MacroActionWidget::PopulateSceneCombo()
{
	QString current = sceneCombo->currentText();
	sceneCombo->blockSignals(true);
	sceneCombo->clear();
	sceneCombo->addItems(get_scene_names());
	if (!current.isEmpty()) {
		int idx = sceneCombo->findText(current);
		if (idx >= 0)
			sceneCombo->setCurrentIndex(idx);
	}
	sceneCombo->blockSignals(false);
	SetComboMinWidth(sceneCombo);
}

void MacroActionWidget::PopulateSourceCombo(const QString &sceneName)
{
	QString current = sourceCombo->currentText();
	sourceCombo->blockSignals(true);
	sourceCombo->clear();

	if (type_needs_audio_source(currentType)) {
		sourceCombo->addItems(get_audio_source_names());
	} else {
		sourceCombo->addItems(
			get_source_names_in_scene(sceneName));
	}

	if (!current.isEmpty()) {
		int idx = sourceCombo->findText(current);
		if (idx >= 0)
			sourceCombo->setCurrentIndex(idx);
	}
	sourceCombo->blockSignals(false);
	SetComboMinWidth(sourceCombo);
}

void MacroActionWidget::PopulateTransitionCombo()
{
	QString current = transitionCombo->currentText();
	transitionCombo->blockSignals(true);
	transitionCombo->clear();
	transitionCombo->addItems(get_transition_names());
	if (!current.isEmpty()) {
		int idx = transitionCombo->findText(current);
		if (idx >= 0)
			transitionCombo->setCurrentIndex(idx);
	}
	transitionCombo->blockSignals(false);
	SetComboMinWidth(transitionCombo);
}

/* ---- Rebuild Parameter Widgets ---- */

void MacroActionWidget::RebuildParameterWidgets()
{
	/* Hide all parameter widgets */
	sceneCombo->setVisible(false);
	sourceCombo->setVisible(false);
	transitionCombo->setVisible(false);
	delaySpinBox->setVisible(false);
	volumeSpinBox->setVisible(false);
	transitionDurationSpinBox->setVisible(false);
	paramLabel->setVisible(false);

	switch (currentType) {
	case HelixToolsActionType::SwitchScene:
		PopulateSceneCombo();
		sceneCombo->setVisible(true);
		break;

	case HelixToolsActionType::ShowSource:
	case HelixToolsActionType::HideSource:
	case HelixToolsActionType::ToggleSource:
		PopulateSceneCombo();
		sceneCombo->setVisible(true);
		PopulateSourceCombo(sceneCombo->currentText());
		sourceCombo->setVisible(true);
		break;

	case HelixToolsActionType::MuteSource:
	case HelixToolsActionType::UnmuteSource:
	case HelixToolsActionType::ToggleAudio:
		PopulateSourceCombo("");
		sourceCombo->setVisible(true);
		break;

	case HelixToolsActionType::SetVolume:
		PopulateSourceCombo("");
		sourceCombo->setVisible(true);
		volumeSpinBox->setVisible(true);
		break;

	case HelixToolsActionType::Delay:
		delaySpinBox->setVisible(true);
		break;

	case HelixToolsActionType::SetTransition:
		PopulateTransitionCombo();
		transitionCombo->setVisible(true);
		transitionDurationSpinBox->setVisible(true);
		break;

	case HelixToolsActionType::StartStreaming:
	case HelixToolsActionType::StopStreaming:
	case HelixToolsActionType::StartRecording:
	case HelixToolsActionType::StopRecording:
		paramLabel->setText("(no parameters)");
		paramLabel->setVisible(true);
		break;
	}
}

/* ---- Slots ---- */

void MacroActionWidget::OnActionTypeChanged(int index)
{
	currentType = static_cast<HelixToolsActionType>(
		typeCombo->itemData(index).toInt());
	RebuildParameterWidgets();
}

/* ---- Get/Set Action ---- */

HelixToolsAction MacroActionWidget::GetAction() const
{
	HelixToolsAction action;
	action.type = currentType;

	switch (currentType) {
	case HelixToolsActionType::SwitchScene:
		action.scene_name =
			sceneCombo->currentText().toStdString();
		break;

	case HelixToolsActionType::ShowSource:
	case HelixToolsActionType::HideSource:
	case HelixToolsActionType::ToggleSource:
		action.scene_name =
			sceneCombo->currentText().toStdString();
		action.source_name =
			sourceCombo->currentText().toStdString();
		break;

	case HelixToolsActionType::MuteSource:
	case HelixToolsActionType::UnmuteSource:
	case HelixToolsActionType::ToggleAudio:
		action.source_name =
			sourceCombo->currentText().toStdString();
		break;

	case HelixToolsActionType::SetVolume:
		action.source_name =
			sourceCombo->currentText().toStdString();
		action.volume = (float)(volumeSpinBox->value() / 100.0);
		break;

	case HelixToolsActionType::Delay:
		action.delay_seconds = (float)delaySpinBox->value();
		break;

	case HelixToolsActionType::SetTransition:
		action.transition_name =
			transitionCombo->currentText().toStdString();
		action.transition_duration_ms =
			transitionDurationSpinBox->value();
		break;

	default:
		break;
	}

	return action;
}

void MacroActionWidget::SetAction(const HelixToolsAction &action)
{
	currentType = action.type;

	/* Set type combo */
	typeCombo->blockSignals(true);
	typeCombo->setCurrentIndex((int)action.type);
	typeCombo->blockSignals(false);

	/* Rebuild parameter widgets for this type */
	RebuildParameterWidgets();

	/* Set parameter values */
	switch (action.type) {
	case HelixToolsActionType::SwitchScene: {
		int idx = sceneCombo->findText(
			QString::fromStdString(action.scene_name));
		if (idx >= 0)
			sceneCombo->setCurrentIndex(idx);
		else if (!action.scene_name.empty())
			sceneCombo->setCurrentText(
				QString::fromStdString(action.scene_name));
		break;
	}

	case HelixToolsActionType::ShowSource:
	case HelixToolsActionType::HideSource:
	case HelixToolsActionType::ToggleSource: {
		int idx = sceneCombo->findText(
			QString::fromStdString(action.scene_name));
		if (idx >= 0) {
			sceneCombo->setCurrentIndex(idx);
			PopulateSourceCombo(sceneCombo->currentText());
		} else if (!action.scene_name.empty()) {
			sceneCombo->setCurrentText(
				QString::fromStdString(action.scene_name));
		}
		int sidx = sourceCombo->findText(
			QString::fromStdString(action.source_name));
		if (sidx >= 0)
			sourceCombo->setCurrentIndex(sidx);
		else if (!action.source_name.empty())
			sourceCombo->setCurrentText(
				QString::fromStdString(action.source_name));
		break;
	}

	case HelixToolsActionType::MuteSource:
	case HelixToolsActionType::UnmuteSource:
	case HelixToolsActionType::ToggleAudio: {
		int idx = sourceCombo->findText(
			QString::fromStdString(action.source_name));
		if (idx >= 0)
			sourceCombo->setCurrentIndex(idx);
		else if (!action.source_name.empty())
			sourceCombo->setCurrentText(
				QString::fromStdString(action.source_name));
		break;
	}

	case HelixToolsActionType::SetVolume: {
		int idx = sourceCombo->findText(
			QString::fromStdString(action.source_name));
		if (idx >= 0)
			sourceCombo->setCurrentIndex(idx);
		else if (!action.source_name.empty())
			sourceCombo->setCurrentText(
				QString::fromStdString(action.source_name));
		volumeSpinBox->setValue((double)(action.volume * 100.0f));
		break;
	}

	case HelixToolsActionType::Delay:
		delaySpinBox->setValue((double)action.delay_seconds);
		break;

	case HelixToolsActionType::SetTransition: {
		int idx = transitionCombo->findText(
			QString::fromStdString(action.transition_name));
		if (idx >= 0)
			transitionCombo->setCurrentIndex(idx);
		else if (!action.transition_name.empty())
			transitionCombo->setCurrentText(
				QString::fromStdString(
					action.transition_name));
		transitionDurationSpinBox->setValue(
			action.transition_duration_ms);
		break;
	}

	default:
		break;
	}
}
