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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "music-manager.h"
#include "helixtools-music.h"
#include "music-backend-config.h"
#include "music-credentials.h"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QDateTime>
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

	/* The source is a real, named scene item now (see the header
	 * comment), so it's persisted as part of the scene collection and
	 * already exists on disk after the first run. Reuse it by name
	 * instead of always creating a new one — obs_source_create() would
	 * otherwise collide with the persisted source's name and get
	 * silently auto-numbered by OBS ("HelixTools Music 2", "3", ...),
	 * leaving a new orphaned duplicate source + mixer channel on every
	 * OBS restart. */
	audioSource = obs_get_source_by_name(MUSIC_AUDIO_SOURCE_NAME);
	if (audioSource) {
		applyBaseSettings(audioSource);

		injectedScenes.clear();
		syncScenes();

		lastLoadedTrackId.clear();
		lastPlaybackStatus.clear();

		pollTimer->start(POLL_INTERVAL_MS);
		return;
	}

	obs_data_t *settings = obs_data_create();
	audioSource = obs_source_create("ffmpeg_source", MUSIC_AUDIO_SOURCE_NAME,
					 settings, nullptr);
	obs_data_release(settings);

	if (!audioSource) {
		blog(LOG_ERROR, "[helixtools-music] Failed to create audio source");
		return;
	}

	applyBaseSettings(audioSource);

	injectedScenes.clear();
	syncScenes();

	lastLoadedTrackId.clear();
	lastPlaybackStatus.clear();

	/* Not gated on a cached user ID already existing: the dock's auth
	 * capture (music-dock.cpp) runs on its own ~2s interval after
	 * login and can easily land after this runs (both fire around
	 * plugin/dock startup). pollOverlayData() already no-ops cleanly
	 * whenever the credential isn't there yet, so it's safe to just
	 * always start the timer and let each tick check for itself. */
	pollTimer->start(POLL_INTERVAL_MS);
}

void MusicManager::applyBaseSettings(obs_source_t *source)
{
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_set_bool(settings, "is_local_file", false);
	obs_data_set_bool(settings, "looping", false);
	obs_data_set_bool(settings, "close_when_inactive", false);
	/* Explicitly restored to true (ffmpeg_source's own default), not left
	 * alone: a source persisted by an earlier, buggy build of this
	 * plugin may already have this saved as false in the scene
	 * collection file, and obs_source_get_settings() above returns
	 * exactly that persisted value if untouched. obs-ffmpeg's
	 * ffmpeg_source_deactivate() only calls media_playback_stop() when
	 * this is true — since this source lives in every scene, "deactivate"
	 * only actually fires when OBS itself is tearing every scene down
	 * (shutdown), so leaving this false permanently defeats that
	 * stop-before-destroy safety net, letting the decode thread keep
	 * pushing audio into audio_callback() while OBS's main thread is
	 * concurrently freeing the source in ClearSceneData — the exact crash
	 * hit before. */
	obs_data_set_bool(settings, "restart_on_activate", true);
	/* Also explicitly cleared, not left as whatever was persisted: for a
	 * non-local-file ffmpeg_source (true here — this always plays a
	 * remote URL, never a local path), *every* settings update
	 * unconditionally reopens and starts playback (obs-ffmpeg always
	 * treats a streaming input's settings change as "should restart"),
	 * regardless of restart_on_activate or any pause we issue afterward.
	 * Reusing a source from a prior session with the last real track's
	 * URL still saved in "input" meant this call alone was enough to
	 * kick off playback of a stale track — and if that first attempt hit
	 * the network before it was ready, ffmpeg_source's own reconnect
	 * thread would retry ~10s later (its default reconnect_delay_sec)
	 * and start playing anyway, bypassing whatever pause we'd already
	 * sent. Clearing "input" here means there is nothing left to
	 * reconnect to — only pollOverlayData()'s own explicit, freshly
	 * polled track URL should ever cause this source to actually play. */
	obs_data_set_string(settings, "input", "");
	obs_source_update(source, settings);
	obs_data_release(settings);
}

void MusicManager::injectIntoScene(obs_scene_t *scene, const char *sceneName)
{
	if (injectedScenes.count(sceneName))
		return;
	injectedScenes.insert(sceneName);

	/* audioSource may have just been reused from an existing, persisted
	 * source (see createAudioSource()) whose scene items were already
	 * loaded from the scene collection file — in that case an item for
	 * it already exists here from a prior session and must not be
	 * duplicated. injectedScenes alone doesn't catch this since it's
	 * reset to empty at the start of every createAudioSource() call. */
	if (obs_scene_find_source(scene, obs_source_get_name(audioSource)))
		return;

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
	query.addQueryItem("select", "track_id,audio_url,playback_status,progress_ms,updated_at");
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
	QDateTime updatedAt = QDateTime::fromString(row.value("updated_at").toString(), Qt::ISODateWithMs);

	if (audioUrl.empty())
		return;

	/* The dock only writes this row while its own page is open, so a row
	 * left over from a session that ended without ever writing "paused"
	 * (tab/browser closed mid-track, PC slept, etc.) would otherwise
	 * read back as permanently "playing" and auto-start audio the moment
	 * this plugin next polls it — with nobody having pressed play. If
	 * the row hasn't been touched recently, treat it as abandoned and
	 * never resume playback from it, regardless of its stored status. */
	constexpr qint64 STALE_MS = 10000;
	bool stale = !updatedAt.isValid() ||
		     updatedAt.msecsTo(QDateTime::currentDateTimeUtc()) > STALE_MS;
	if (stale && playbackStatus == "playing")
		playbackStatus = "paused";

	/* lastLoadedTrackId tracks what's actually been pushed into the
	 * source's "input" — not merely the last track_id seen from a poll —
	 * so that a track deferred earlier (seen while paused/stale, so never
	 * loaded) still gets loaded the moment it's genuinely playing, even
	 * though its track_id was already "seen" on an earlier tick. */
	if (playbackStatus == "playing" && trackId != lastLoadedTrackId) {
		lastLoadedTrackId = trackId;
		lastPlaybackStatus = playbackStatus;

		/* Only ever push a new "input" when the end state we actually
		 * want is playing. For a non-local-file ffmpeg_source (always
		 * true here — this plays a remote URL), obs_source_update()
		 * unconditionally reopens and starts playback regardless of
		 * anything else, and if that first open attempt fails (e.g.
		 * network not ready yet), ffmpeg_source's own reconnect thread
		 * retries on its own schedule (~10s by default) and starts
		 * playing once it succeeds — with no memory of any pause
		 * issued in between. Never setting "input" for a track we
		 * don't yet want playing means there's nothing left for a
		 * delayed reconnect to surprise us with. */
		obs_data_t *settings = obs_source_get_settings(audioSource);
		obs_data_set_string(settings, "input", audioUrl.c_str());
		obs_source_update(audioSource, settings);
		obs_data_release(settings);

		if (progressMs > 0)
			obs_source_media_set_time(audioSource, progressMs);

		obs_source_media_play_pause(audioSource, false);

		/* Diagnostic logging in an earlier build of this function
		 * (obs_source_media_get_state()) showed the source reporting
		 * "PLAYING" continuously through a switch that was actually
		 * silent — so the state API doesn't reflect this at all. The
		 * user found the real fix by hand: toggling the Mixer's audio
		 * monitoring for this source off and back on brings the sound
		 * back, with no change to play/pause state. That points at
		 * OBS's separate local monitor-output stream getting stuck on
		 * the abrupt format/timestamp discontinuity a live "input" swap
		 * creates, independent of the actual program/stream output —
		 * so this likely never reached viewers at all, only the
		 * streamer's own local monitoring. Automating exactly that
		 * manual fix here: drop monitoring to none and restore whatever
		 * it was a moment later, forcing OBS to reinitialize that
		 * monitor stream against the new track cleanly. */
		enum obs_monitoring_type monitoring = obs_source_get_monitoring_type(audioSource);
		if (monitoring != OBS_MONITORING_TYPE_NONE) {
			obs_source_set_monitoring_type(audioSource, OBS_MONITORING_TYPE_NONE);
			QTimer::singleShot(250, this, [this, trackId, monitoring]() {
				if (!audioSource || trackId != lastLoadedTrackId)
					return;
				obs_source_set_monitoring_type(audioSource, monitoring);
			});
		}
		return;
	}

	if (playbackStatus != lastPlaybackStatus) {
		lastPlaybackStatus = playbackStatus;

		if (playbackStatus == "playing")
			obs_source_media_play_pause(audioSource, false);
		else if (playbackStatus == "paused")
			obs_source_media_play_pause(audioSource, true);
	}
}
