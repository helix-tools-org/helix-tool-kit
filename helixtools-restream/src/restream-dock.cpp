/*
 * restream-dock.cpp - Qt6 destinations editor dock
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

#include "restream-dock.h"
#include "restream-storage.h"

#include <obs-frontend-api.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>

#include <cstring>

/* ---- Main stream label ---- */

/* Reads OBS's currently configured stream service (Settings -> Stream) so
 * the dock can show what the "main" stream is pointed at, next to the
 * restream destinations. This is read-only and never touches the service
 * OBS itself owns (obs_frontend_get_streaming_service() returns a borrowed
 * pointer — OBSBasic::GetService() does not add a ref — so no release
 * here, unlike obs_service_get_settings()'s return value below).
 *
 * Prefers ctx->main_stream_label if the user set one; otherwise
 * auto-detects. OBS itself always names any service created through its
 * own Settings dialog "default_service" (see OBSBasic_Service.cpp),
 * regardless of the actual platform configured, so that placeholder is
 * ignored in favor of the platform name (for built-in presets) or the
 * server URL (for a custom RTMP/WHIP entry, which is what Rumble uses). */
static QString GetMainStreamValue(struct restream_context *ctx)
{
	if (ctx->main_stream_label && *ctx->main_stream_label)
		return QString::fromUtf8(ctx->main_stream_label);

	obs_service_t *service = obs_frontend_get_streaming_service();
	if (!service)
		return "(not configured)";

	const char *name = obs_service_get_name(service);
	if (name && *name && strcmp(name, "default_service") != 0)
		return QString::fromUtf8(name);

	obs_data_t *settings = obs_service_get_settings(service);
	const char *id = obs_service_get_id(service);
	QString result;
	if (id && strcmp(id, "rtmp_common") == 0) {
		const char *svc = obs_data_get_string(settings, "service");
		result = (svc && *svc) ? QString::fromUtf8(svc) : "(unnamed)";
	} else {
		const char *server = obs_data_get_string(settings, "server");
		result = (server && *server) ? QString::fromUtf8(server) : "(unnamed)";
	}
	obs_data_release(settings);
	return result;
}

/* ---- Add/Edit sub-form ---- */

static bool PromptDestinationForm(QWidget *parent, const QString &title, QString &name,
				  QString &url, QString &key, bool &enabled)
{
	QDialog dlg(parent);
	dlg.setWindowTitle(title);
	dlg.setMinimumWidth(420);

	auto *layout = new QVBoxLayout(&dlg);
	auto *form = new QFormLayout();

	auto *nameEdit = new QLineEdit(name, &dlg);
	auto *urlEdit = new QLineEdit(url, &dlg);
	urlEdit->setPlaceholderText("rtmp://example.com/live");
	auto *keyEdit = new QLineEdit(key, &dlg);
	keyEdit->setEchoMode(QLineEdit::Password);
	auto *enabledBox = new QCheckBox(&dlg);
	enabledBox->setChecked(enabled);

	form->addRow("Name:", nameEdit);
	form->addRow("Server URL:", urlEdit);
	form->addRow("Stream key:", keyEdit);
	form->addRow("Enabled:", enabledBox);

	layout->addLayout(form);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
	layout->addWidget(buttons);

	QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

	if (dlg.exec() != QDialog::Accepted)
		return false;

	name = nameEdit->text().trimmed();
	url = urlEdit->text().trimmed();
	key = keyEdit->text().trimmed();
	enabled = enabledBox->isChecked();
	return true;
}

/* ---- RestreamDock ---- */

RestreamDock::RestreamDock(struct restream_context *ctx, QWidget *parent)
	: QWidget(parent), ctx(ctx)
{
	auto *mainLayout = new QVBoxLayout(this);
	setLayout(mainLayout);

	auto *mainStreamRow = new QHBoxLayout();
	mainStreamLabel = new QLabel(this);
	mainStreamLabel->setStyleSheet("font-weight: bold;");
	mainStreamRow->addWidget(mainStreamLabel);
	mainStreamRow->addStretch(1);
	btnRenameMainStream = new QPushButton("Rename...", this);
	mainStreamRow->addWidget(btnRenameMainStream);
	mainLayout->addLayout(mainStreamRow);

	auto *helpNotice = new QLabel(
		"Only add destinations you want this stream sent to in addition "
		"to your main stream (set in OBS Settings → Stream). Do not "
		"add that same platform again here.",
		this);
	helpNotice->setWordWrap(true);
	mainLayout->addWidget(helpNotice);

	streamingNotice = new QLabel(
		"Stop your stream to add, edit, or remove destinations.", this);
	streamingNotice->setStyleSheet("color: orange;");
	streamingNotice->setVisible(false);
	mainLayout->addWidget(streamingNotice);

	destList = new QListWidget(this);
	mainLayout->addWidget(destList, 1);

	auto *buttonLayout = new QHBoxLayout();
	btnAdd = new QPushButton("Add...", this);
	btnEdit = new QPushButton("Edit...", this);
	btnRemove = new QPushButton("Remove", this);
	buttonLayout->addWidget(btnAdd);
	buttonLayout->addWidget(btnEdit);
	buttonLayout->addWidget(btnRemove);
	buttonLayout->addStretch(1);

	mainLayout->addLayout(buttonLayout);

	connect(destList, &QListWidget::currentRowChanged, this,
		&RestreamDock::OnSelectionChanged);
	connect(btnAdd, &QPushButton::clicked, this, &RestreamDock::OnAdd);
	connect(btnEdit, &QPushButton::clicked, this, &RestreamDock::OnEdit);
	connect(btnRemove, &QPushButton::clicked, this, &RestreamDock::OnRemove);
	connect(btnRenameMainStream, &QPushButton::clicked, this, &RestreamDock::OnRenameMainStream);

	/* OBS doesn't push events for "a restream output just connected", so
	 * poll periodically to keep the live-status lights and the main
	 * stream label current while the dock is visible. */
	refreshTimer = new QTimer(this);
	connect(refreshTimer, &QTimer::timeout, this, &RestreamDock::RefreshLiveStatus);
	refreshTimer->start(1000);

	RefreshLiveStatus();
}

RestreamDock::~RestreamDock() {}

void RestreamDock::PopulateList()
{
	int prevRow = destList->currentRow();

	destList->blockSignals(true);
	destList->clear();
	for (int i = 0; i < ctx->destination_count; i++) {
		const struct restream_destination *d = &ctx->destinations[i];
		/* Colored dot reflects whether this destination is actually
		 * live right now (d->active, set by restream-output.c once
		 * its obs_output_t connects) — not just whether it's enabled
		 * to be included next time streaming starts. */
		QString label = QString("%1  %2  ->  %3%4")
					.arg(d->active ? QString::fromUtf8("\xF0\x9F\x9F\xA2") /* green circle */
							: QString::fromUtf8("\xF0\x9F\x94\xB4") /* red circle */)
					.arg(d->name ? QString::fromUtf8(d->name) : "(unnamed)")
					.arg(d->rtmp_url ? QString::fromUtf8(d->rtmp_url) : "")
					.arg(d->enabled ? "" : "  (disabled)");
		destList->addItem(label);
	}

	if (ctx->destination_count > 0) {
		int row = (prevRow >= 0 && prevRow < ctx->destination_count) ? prevRow : 0;
		destList->setCurrentRow(row);
	}
	destList->blockSignals(false);

	UpdateButtonStates();
}

void RestreamDock::RefreshLiveStatus()
{
	bool live = obs_frontend_streaming_active();
	QString dot = live ? QString::fromUtf8("\xF0\x9F\x9F\xA2") /* green circle */
			   : QString::fromUtf8("\xF0\x9F\x94\xB4") /* red circle */;
	mainStreamLabel->setText(
		QString("%1  Main stream: %2").arg(dot).arg(GetMainStreamValue(ctx)));
	PopulateList();
}

void RestreamDock::OnRenameMainStream()
{
	QString current = ctx->main_stream_label ? QString::fromUtf8(ctx->main_stream_label) : "";

	bool ok;
	QString text = QInputDialog::getText(
		this, "Main Stream Label",
		"Custom label for your main stream (leave blank to auto-detect):",
		QLineEdit::Normal, current, &ok);
	if (!ok)
		return;

	bfree(ctx->main_stream_label);
	QByteArray utf8 = text.trimmed().toUtf8();
	ctx->main_stream_label = utf8.isEmpty() ? nullptr : bstrdup(utf8.constData());

	helixtools_restream_save(ctx);
	RefreshLiveStatus();
}

void RestreamDock::UpdateButtonStates()
{
	bool streaming = ctx->streaming;
	bool hasSelection = destList->currentRow() >= 0;
	bool atCap = ctx->destination_count >= HELIXTOOLS_RESTREAM_MAX_DESTINATIONS;

	streamingNotice->setVisible(streaming);

	btnAdd->setEnabled(!streaming && !atCap);
	btnEdit->setEnabled(!streaming && hasSelection);
	btnRemove->setEnabled(!streaming && hasSelection);
}

void RestreamDock::OnSelectionChanged()
{
	UpdateButtonStates();
}

void RestreamDock::OnAdd()
{
	if (ctx->streaming)
		return;

	if (ctx->destination_count >= HELIXTOOLS_RESTREAM_MAX_DESTINATIONS) {
		QMessageBox::warning(this, "Restream Destinations",
				     QString("You can have at most %1 destinations.")
					     .arg(HELIXTOOLS_RESTREAM_MAX_DESTINATIONS));
		return;
	}

	QString name, url, key;
	bool enabled = true;
	if (!PromptDestinationForm(this, "Add Destination", name, url, key, enabled))
		return;

	if (name.isEmpty() || url.isEmpty()) {
		QMessageBox::warning(this, "Restream Destinations",
				     "Name and server URL are required.");
		return;
	}

	struct restream_destination *d = &ctx->destinations[ctx->destination_count];
	d->name = bstrdup(name.toUtf8().constData());
	d->rtmp_url = bstrdup(url.toUtf8().constData());
	d->stream_key = bstrdup(key.toUtf8().constData());
	d->enabled = enabled;
	d->active = false;
	d->service = nullptr;
	d->output = nullptr;
	d->video_encoder = nullptr;
	d->audio_encoder = nullptr;
	ctx->destination_count++;

	helixtools_restream_save(ctx);
	PopulateList();
	destList->setCurrentRow(destList->count() - 1);
}

void RestreamDock::OnEdit()
{
	if (ctx->streaming)
		return;

	int row = destList->currentRow();
	if (row < 0 || row >= ctx->destination_count)
		return;

	struct restream_destination *d = &ctx->destinations[row];

	QString name = d->name ? QString::fromUtf8(d->name) : "";
	QString url = d->rtmp_url ? QString::fromUtf8(d->rtmp_url) : "";
	QString key = d->stream_key ? QString::fromUtf8(d->stream_key) : "";
	bool enabled = d->enabled;

	if (!PromptDestinationForm(this, "Edit Destination", name, url, key, enabled))
		return;

	if (name.isEmpty() || url.isEmpty()) {
		QMessageBox::warning(this, "Restream Destinations",
				     "Name and server URL are required.");
		return;
	}

	bfree(d->name);
	bfree(d->rtmp_url);
	bfree(d->stream_key);
	d->name = bstrdup(name.toUtf8().constData());
	d->rtmp_url = bstrdup(url.toUtf8().constData());
	d->stream_key = bstrdup(key.toUtf8().constData());
	d->enabled = enabled;

	helixtools_restream_save(ctx);
	PopulateList();
	destList->setCurrentRow(row);
}

void RestreamDock::OnRemove()
{
	if (ctx->streaming)
		return;

	int row = destList->currentRow();
	if (row < 0 || row >= ctx->destination_count)
		return;

	struct restream_destination *d = &ctx->destinations[row];
	auto result = QMessageBox::question(
		this, "Restream Destinations",
		QString("Remove destination \"%1\"?")
			.arg(d->name ? QString::fromUtf8(d->name) : "(unnamed)"));
	if (result != QMessageBox::Yes)
		return;

	bfree(d->name);
	bfree(d->rtmp_url);
	bfree(d->stream_key);

	for (int i = row; i < ctx->destination_count - 1; i++)
		ctx->destinations[i] = ctx->destinations[i + 1];

	ctx->destination_count--;
	memset(&ctx->destinations[ctx->destination_count], 0,
	       sizeof(struct restream_destination));

	helixtools_restream_save(ctx);
	PopulateList();
}
