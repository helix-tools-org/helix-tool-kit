/*
 * clip-manager.cpp - Core clip logic implementation
 *
 * Fetches the user's Rumble API URL from Supabase, polls the Rumble
 * livestream API directly for active streams, and creates clips via
 * the configured clip backend (see clip-backend-config.h).
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

#include "clip-manager.h"
#include "helixtools-clip.h"
#include <obs.h>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#pragma comment(lib, "advapi32.lib")
#endif

ClipManager::ClipManager(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , pollTimer(new QTimer(this))
{
    connect(pollTimer, &QTimer::timeout, this, &ClipManager::onPollTimeout);
}

ClipManager::~ClipManager() = default;

void ClipManager::startPollTimer(int intervalMs)
{
    pollTimer->start(intervalMs);
}

void ClipManager::stopPollTimer()
{
    pollTimer->stop();
    currentVideoId.clear();
    cachedApiUrl.clear();
    credentialFailCount = 0;
}

/* ------------------------------------------------------------------ */
/* Step 1: Fetch Rumble API URL from Supabase rumble_credentials      */
/* ------------------------------------------------------------------ */

void ClipManager::fetchRumbleApiUrl()
{
    if (isFetchingApiUrl || !cachedApiUrl.isEmpty())
        return;

    if (credentialFailCount >= MAX_CREDENTIAL_RETRIES)
        return;

    isFetchingApiUrl = true;

#ifdef _WIN32
    PCREDENTIALW cred = nullptr;
    if (!CredReadW(HUB_CREDENTIAL_USERID, CRED_TYPE_GENERIC, 0, &cred)) {
        blog(LOG_WARNING, "[helixtools-clip] No user ID found in credentials");
        isFetchingApiUrl = false;
        credentialFailCount++;
        if (credentialFailCount >= MAX_CREDENTIAL_RETRIES)
            blog(LOG_INFO, "[helixtools-clip] Credential lookup failed %d times, stopping retries",
                 credentialFailCount);
        return;
    }

    QString userIdStr = QString::fromUtf8((const char*)cred->CredentialBlob,
                                            cred->CredentialBlobSize);
    CredFree(cred);

    /* Read the user's JWT for Supabase RLS authentication */
    QString userToken;
    if (CredReadW(HUB_CREDENTIAL_TOKEN, CRED_TYPE_GENERIC, 0, &cred)) {
        userToken = QString::fromUtf8((const char*)cred->CredentialBlob,
                                       cred->CredentialBlobSize);
        CredFree(cred);
    }

    if (userToken.isEmpty()) {
        blog(LOG_WARNING, "[helixtools-clip] No auth token found in credentials");
        isFetchingApiUrl = false;
        credentialFailCount++;
        return;
    }
#else
    isFetchingApiUrl = false;
    return;
#endif

    QUrl url(QString(SUPABASE_BASE_URL) + "/rest/v1/rumble_credentials?user_id=eq." + userIdStr);
    QNetworkRequest request(url);
    request.setRawHeader("apikey", SUPABASE_ANON_KEY);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(userToken).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &ClipManager::onRumbleKeyReply);
}

void ClipManager::onRumbleKeyReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    isFetchingApiUrl = false;

    if (reply->error() != QNetworkReply::NoError) {
        blog(LOG_WARNING, "[helixtools-clip] Failed to fetch rumble credentials: %s",
             qUtf8Printable(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        blog(LOG_WARNING, "[helixtools-clip] Invalid rumble credentials response");
        reply->deleteLater();
        return;
    }

    QJsonArray arr = doc.array();
    if (arr.isEmpty()) {
        credentialFailCount++;
        if (credentialFailCount >= MAX_CREDENTIAL_RETRIES)
            blog(LOG_WARNING, "[helixtools-clip] No rumble credentials found after %d attempts "
                 "(connect Rumble account in Hub to enable clips)",
                 credentialFailCount);
        else
            blog(LOG_INFO, "[helixtools-clip] No rumble credentials found (attempt %d/%d)",
                 credentialFailCount, MAX_CREDENTIAL_RETRIES);
        reply->deleteLater();
        return;
    }

    QJsonObject obj = arr.first().toObject();
    QString apiUrl = obj.value("api_url").toString();

    if (apiUrl.isEmpty()) {
        blog(LOG_WARNING, "[helixtools-clip] Empty api_url in rumble credentials");
        reply->deleteLater();
        return;
    }

    cachedApiUrl = apiUrl;
    blog(LOG_INFO, "[helixtools-clip] Rumble API URL cached");

    /* Immediately poll now that we have the URL */
    pollLiveStream();

    reply->deleteLater();
}

/* ------------------------------------------------------------------ */
/* Step 2: Poll Rumble API directly for active livestreams            */
/* ------------------------------------------------------------------ */

void ClipManager::pollLiveStream()
{
    if (cachedApiUrl.isEmpty()) {
        fetchRumbleApiUrl();
        return;
    }

    QUrl pollUrl(cachedApiUrl);
    QNetworkRequest request(pollUrl);
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &ClipManager::onPollReply);
}

void ClipManager::onPollReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    if (reply->error() != QNetworkReply::NoError) {
        blog(LOG_WARNING, "[helixtools-clip] Rumble API poll failed: %s",
             qUtf8Printable(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        blog(LOG_WARNING, "[helixtools-clip] Invalid Rumble API response");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    QJsonArray livestreams = obj.value("livestreams").toArray();

    if (!livestreams.isEmpty()) {
        QJsonObject stream = livestreams.first().toObject();
        bool isLive = stream.value("is_live").toBool(false);
        QString videoId = stream.value("id").toString();

        if (isLive && !videoId.isEmpty()) {
            if (videoId != currentVideoId) {
                currentVideoId = videoId;
                blog(LOG_INFO, "[helixtools-clip] Live stream detected: %s",
                     qUtf8Printable(videoId));
                emit liveStreamFound(videoId);
            }
        } else if (!isLive && !currentVideoId.isEmpty()) {
            blog(LOG_INFO, "[helixtools-clip] Stream went offline");
            currentVideoId.clear();
        }
    } else if (!currentVideoId.isEmpty()) {
        blog(LOG_INFO, "[helixtools-clip] No livestreams in response");
        currentVideoId.clear();
    }

    reply->deleteLater();
}

void ClipManager::onPollTimeout()
{
    pollLiveStream();
}

/* ------------------------------------------------------------------ */
/* Step 3: Create clip via the configured clip backend                */
/* ------------------------------------------------------------------ */

void ClipManager::createClip(const QString &videoId, int lastSeconds,
                              const QString &userId, const QString &token)
{
    if (videoId.isEmpty()) {
        emit clipError("No active live stream");
        return;
    }

    QUrl url(CLIP_API_URL);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

    QJsonObject body;
    body.insert("videoId", videoId);
    body.insert("lastSeconds", lastSeconds);
    body.insert("userId", userId);

    QByteArray bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = networkManager->post(request, bodyData);
    connect(reply, &QNetworkReply::finished, this, &ClipManager::onClipReply);
}

void ClipManager::onClipReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("Clip API error: %1").arg(reply->errorString());
        blog(LOG_WARNING, "[helixtools-clip] %s", qUtf8Printable(error));
        emit clipError(error);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit clipError("Invalid clip response");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    QJsonObject clip = obj.value("clip").toObject();

    if (clip.isEmpty()) {
        emit clipError("No clip in response");
        reply->deleteLater();
        return;
    }

    QString clipId = clip.value("id").toString();
    QString mp4Url = clip.value("mp4_url").toString();

    blog(LOG_INFO, "[helixtools-clip] Clip created: %s", qUtf8Printable(clipId));
    emit clipCreated(clipId, mp4Url);

    reply->deleteLater();
}
