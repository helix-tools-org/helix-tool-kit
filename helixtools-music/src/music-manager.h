/*
 * music-manager.h - Mirrors the Music dock's playback into a native
 * OBS audio source on a global mixer channel
 *
 * See music-manager.cpp for the full design rationale (why a second
 * browser_source was removed, and why this polls overlay_data instead).
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

#include <obs.h>

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>

#include <set>
#include <string>

class MusicManager : public QObject {
	Q_OBJECT

public:
	explicit MusicManager(QObject *parent = nullptr);
	~MusicManager() override;

	/* Called from OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING/_CHANGED
	 * and module load/unload — see helixtools-music.cpp. */
	void createAudioSource();
	void teardownAudioSource();

	/* Called from OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED so a scene
	 * created after this collection loaded still gets the source. Safe
	 * to call any time audioSource may or may not exist yet. Scenes
	 * already handled (in injectedScenes) are skipped even if the user
	 * has since removed the item from them — this never fights a
	 * deliberate removal. */
	void syncScenes();

private slots:
	void pollOverlayData();
	void onPollReply();

private:
	std::string cachedUserId();
	void injectIntoScene(obs_scene_t *scene, const char *sceneName);
	void applyBaseSettings(obs_source_t *source);

	QNetworkAccessManager *networkManager;
	QTimer *pollTimer;
	obs_source_t *audioSource = nullptr;
	std::set<std::string> injectedScenes;

	std::string lastLoadedTrackId;
	std::string lastPlaybackStatus;
};
