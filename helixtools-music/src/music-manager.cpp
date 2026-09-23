/*
 * music-manager.cpp - Mirrors the Music dock's playback into a native
 * OBS audio source on a global mixer channel
 *
 * Background (see SESSION_NOTES.md 2026-09-22): this plugin used to
 * pin a second, independent browser_source (a headless CEF instance
 * playing the same page, logged out, with no state sync to the dock)
 * to a mixer channel. Its fader controlled silent audio and served no
 * real function, and it was also the source of the freeze/crash fixed
 * 2026-09-18. It was removed entirely.
 *
 * This replaces it with something that actually works: the dock's own
 * playback state is already written to a Supabase table
 * (overlay_data, via OptimizedAudioContext.tsx on the website side)
 * for the existing "now playing" browser overlay. That table has a
 * public SELECT RLS policy — the overlay itself is read by
 * unauthenticated OBS browser sources — so this can poll it with just
 * the project's anon key, scoped to the logged-in user's ID (captured
 * separately by music-dock.cpp; see music-auth-script.h for why only
 * the bare ID is captured, never an access/refresh token).
 *
 * On a track/state change, this points a native ffmpeg_source (real
 * audio decoding, no CEF) at the track's audio_url and mirrors
 * play/pause — so the Mixer fader for this source now controls the
 * same audio you hear from the dock, instead of a second, disconnected
 * instance of it.
 *
 * NOT on a global output channel, unlike the removed browser_source
 * version — fixed 2026-09-22 after that turned out to reproduce the
 * exact same "unwanted thing rendered on the canvas" bug for a
 * different reason: ffmpeg_source unconditionally declares
 * OBS_SOURCE_ASYNC_VIDEO alongside OBS_SOURCE_AUDIO (see
 * plugins/obs-ffmpeg/obs-ffmpeg-source.c's obs_source_info in the OBS
 * checkout), and most tracks carry embedded cover art (ID3 APIC),
 * which ffmpeg decodes as a one-frame video stream. libobs's main view
 * still walks every global channel and renders whatever occupies it —
 * a global channel has no crop/transform to hide that, since that only
 * exists at the scene-item level. Confirmed no way to make
 * media-playback skip video decoding either (traced through
 * shared/media-playback: struct mp_media_info has no such flag, and
 * media.c's mp_decode_init(m, AVMEDIA_TYPE_VIDEO, ...) call is
 * unconditional).
 *
 * So instead this adds the source as a real scene item — 0x0 scale, so
 * nothing renders — to every scene, via injectIntoScene()/syncScenes().
 * That also means OBS's own scene-collection lifecycle now owns
 * teardown/recreation of the *items*; only the shared obs_source_t
 * itself still needs the manual scene-collection-switch handling
 * (createAudioSource()/teardownAudioSource(), called from
 * helixtools-music.cpp) since it's the same underlying source object
 * referenced by an item in every scene, not scene-owned itself.
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

#include "music-manager.h"
#include "helixtools-music.h"
#include "music-backend-config.h"
#include "music-credentials.h"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

#include <windows.h>
#include <wincred.h>

namespace {
constexpr int POLL_INTERVAL_MS = 1500;
}

MusicManager::MusicManager(QObject *parent) : QObject(parent)
{
	networkManager = new QNetworkAccessManager(this);

	pollTimer = new QTimer(this);
	connect(pollTimer, &QTimer::timeout, this, &MusicManager::pollOverlayData);
}

MusicManager::~MusicManager()
{
	teardownAudioSource();
}

std::string MusicManager::cachedUserId()
{
	PCREDENTIALW cred = nullptr;
	if (!CredReadW(MUSIC_CRED_USERID, CRED_TYPE_GENERIC, 0, &cred))
		return "";

	std::string userId(reinterpret_cast<const char *>(cred->CredentialBlob),
			    cred->CredentialBlobSize);
	CredFree(cred);
	return userId;
}

void MusicManager::createAudioSource()
{
	if (audioSource)
		return;

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "is_local_file", false);
	obs_data_set_bool(settings, "looping", false);
	obs_data_set_bool(settings, "close_when_inactive", false);

	audioSource = obs_source_create("ffmpeg_source", MUSIC_AUDIO_SOURCE_NAME,
					 settings, nullptr);
	obs_data_release(settings);

	if (!audioSource) {
		blog(LOG_ERROR, "[helixtools-music] Failed to create audio source");
		return;
	}

	injectedScenes.clear();
	syncScenes();

	lastTrackId.clear();
	lastPlaybackStatus.clear();

	/* Not gated on a cached user ID already existing: the dock's auth
	 * capture (music-dock.cpp) runs on its own ~2s interval after
	 * login and can easily land after this runs (both fire around
	 * plugin/dock startup). pollOverlayData() already no-ops cleanly
	 * whenever the credential isn't there yet, so it's safe to just
	 * always start the timer and let each tick check for itself. */
	pollTimer->start(POLL_INTERVAL_MS);
}

void MusicManager::injectIntoScene(obs_scene_t *scene, const char *sceneName)
{
	if (injectedScenes.count(sceneName))
		return;
	injectedScenes.insert(sceneName);

	obs_sceneitem_t *item = obs_scene_add(scene, audioSource);
	if (!item) {
		blog(LOG_WARNING, "[helixtools-music] Failed to add audio item to scene '%s'",
		     sceneName);
		return;
	}

	/* Zero scale, not merely hidden: get_frame()'s decoded video (most
	 * tracks carry embedded cover art ffmpeg treats as a one-frame
	 * video stream — see the header comment for why this can't be
	 * avoided at the source level) still renders at full size for a
	 * visible-but-differently-flagged item. A zero-scale item draws
	 * nothing, while its audio is completely unaffected — audio mixing
	 * doesn't consult the item's transform at all. */
	struct vec2 zero = {0.0f, 0.0f};
	obs_sceneitem_set_scale(item, &zero);
}

void MusicManager::syncScenes()
{
	if (!audioSource)
		return;

	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	for (size_t i = 0; i < scenes.sources.num; i++) {
		obs_source_t *sceneSource = scenes.sources.array[i];
		obs_scene_t *scene = obs_scene_from_source(sceneSource);
		if (scene)
			injectIntoScene(scene, obs_source_get_name(sceneSource));
	}

	obs_frontend_source_list_free(&scenes);
}

void MusicManager::teardownAudioSource()
{
	pollTimer->stop();

	if (!audioSource)
		return;

	/* Scene items referencing audioSource are torn down by OBS itself
	 * as part of the scene-collection switch this is called for — this
	 * just releases MusicManager's own reference to the shared source
	 * object. */
	obs_source_release(audioSource);
	audioSource = nullptr;
	injectedScenes.clear();
}

void MusicManager::pollOverlayData()
{
	std::string userId = cachedUserId();
	if (userId.empty()) {
		/* Not logged into the dock yet (or Credential Manager entry
		 * missing) — keep the timer running and just skip this
		 * tick, rather than tearing anything down. */
		return;
	}

	QUrl url(QString(MUSIC_SUPABASE_BASE_URL) + "/rest/v1/overlay_data");
	QUrlQuery query;
	query.addQueryItem("user_id", "eq." + QString::fromStdString(userId));
	query.addQueryItem("select", "track_id,audio_url,playback_status,progress_ms");
	query.addQueryItem("order", "updated_at.desc");
	query.addQueryItem("limit", "1");
	url.setQuery(query);

	QNetworkRequest request(url);
	request.setRawHeader("apikey", MUSIC_SUPABASE_ANON_KEY);
	request.setRawHeader("Authorization",
			     QString("Bearer %1").arg(MUSIC_SUPABASE_ANON_KEY).toUtf8());
	request.setRawHeader("Accept", "application/json");

	QNetworkReply *reply = networkManager->get(request);
	connect(reply, &QNetworkReply::finished, this, &MusicManager::onPollReply);
}

void MusicManager::onPollReply()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply)
		return;

	if (reply->error() != QNetworkReply::NoError) {
		blog(LOG_WARNING, "[helixtools-music] overlay_data poll failed: %s",
		     qUtf8Printable(reply->errorString()));
		reply->deleteLater();
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
	reply->deleteLater();

	if (!doc.isArray() || doc.array().isEmpty() || !audioSource)
		return;

	QJsonObject row = doc.array().first().toObject();
	std::string trackId = row.value("track_id").toString().toStdString();
	std::string audioUrl = row.value("audio_url").toString().toStdString();
	std::string playbackStatus = row.value("playback_status").toString().toStdString();
	int progressMs = row.value("progress_ms").toInt();

	if (audioUrl.empty())
		return;

	if (trackId != lastTrackId) {
		lastTrackId = trackId;
		lastPlaybackStatus.clear();

		obs_data_t *settings = obs_source_get_settings(audioSource);
		obs_data_set_string(settings, "input", audioUrl.c_str());
		obs_source_update(audioSource, settings);
		obs_data_release(settings);

		if (progressMs > 0)
			obs_source_media_set_time(audioSource, progressMs);
	}

	if (playbackStatus != lastPlaybackStatus) {
		lastPlaybackStatus = playbackStatus;

		if (playbackStatus == "playing")
			obs_source_media_play_pause(audioSource, false);
		else if (playbackStatus == "paused")
			obs_source_media_play_pause(audioSource, true);
	}
}
