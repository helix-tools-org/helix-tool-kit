/*
 * service-menu.cpp - Tools menu action to select Rumble as the stream
 * service.
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

#include "helixtools-service.h"

#include <obs-frontend-api.h>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QWidget>

static void on_setup_rumble(void *)
{
	QWidget *parent = static_cast<QWidget *>(obs_frontend_get_main_window());

	bool ok = false;
	QStringList options = {"Rumble Studio (studio.rumble.com)",
			       "Rumble Live (rumble.com)"};
	QString choice = QInputDialog::getItem(
		parent, "Set Up Rumble Streaming",
		"Which Rumble endpoint are you streaming to?", options, 0,
		false, &ok);
	if (!ok)
		return;

	bool isStudio = (choice == options[0]);
	const char *server = isStudio ? RUMBLE_STUDIO_RTMP_URL : RUMBLE_LIVE_RTMP_URL;
	const char *keyLink = isStudio ? RUMBLE_STUDIO_KEY_LINK : RUMBLE_LIVE_KEY_LINK;

	QString key = QInputDialog::getText(
		parent, "Set Up Rumble Streaming",
		QString("Paste your static stream key from:\n%1")
			.arg(keyLink),
		QLineEdit::Password, "", &ok);

	if (!ok || key.isEmpty())
		return;

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "server", server);
	obs_data_set_string(settings, "key", key.toUtf8().constData());

	/* Deliberately "rtmp_custom", not our own "helixtools_rumble_service"
	 * type: OBS's Settings dialog (LoadStream1Settings) only knows how
	 * to correctly round-trip rtmp_custom / rtmp_common / whip_custom.
	 * Any other type isn't recognized, and the dialog silently falls
	 * back to whatever the first common preset is (e.g. Twitch) —
	 * which then gets WRITTEN OVER our service if the user clicks
	 * OK/Apply there. rtmp_custom is exactly what "Custom..." in that
	 * dialog already uses, so this is fully UI-safe.
	 *
	 * obs_frontend_set_streaming_service() takes ownership of this
	 * reference (matches OBSBasic::SetService()'s own usage pattern) */
	obs_service_t *service = obs_service_create(
		"rtmp_custom", isStudio ? "Rumble Studio" : "Rumble Live",
		settings, nullptr);
	obs_data_release(settings);

	if (!service) {
		QMessageBox::warning(parent, "Set Up Rumble Streaming",
				     "Failed to create the Rumble service.");
		return;
	}

	obs_frontend_set_streaming_service(service);
	obs_frontend_save_streaming_service();

	QMessageBox::information(
		parent, "Set Up Rumble Streaming",
		QString("%1 is now your active stream service.\n\n"
			"Rumble recommended settings: H.264, CBR, 2s keyframe\n"
			"interval, 1080p, 6000-10000 Kbps.\n\n"
			"If you previously connected a YouTube or Twitch "
			"account here, OBS may still show a leftover "
			"\"Manage Broadcast\" prompt when you go live. If "
			"so, just open Settings -> Stream and click OK once "
			"(no changes needed) to clear it.")
			.arg(isStudio ? "Rumble Studio" : "Rumble Live"));
}

void helixtools_service_init_menu(void)
{
	obs_frontend_add_tools_menu_item("Set Up Rumble Streaming...",
					 on_setup_rumble, nullptr);
}
