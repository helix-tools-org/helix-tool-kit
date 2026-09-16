/*
 * clip-manager.h - Core clip logic
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

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

class ClipManager : public QObject {
    Q_OBJECT

public:
    explicit ClipManager(QObject *parent = nullptr);
    ~ClipManager();

    void fetchRumbleApiUrl();
    void pollLiveStream();
    void createClip(const QString &videoId, int lastSeconds,
                    const QString &userId, const QString &token);

    void startPollTimer(int intervalMs);
    void stopPollTimer();

    QString getCurrentVideoId() const { return currentVideoId; }

signals:
    void liveStreamFound(const QString &videoId);
    void clipCreated(const QString &clipId, const QString &mp4Url);
    void clipError(const QString &error);

private slots:
    void onRumbleKeyReply();
    void onPollReply();
    void onClipReply();
    void onPollTimeout();

private:
    QNetworkAccessManager *networkManager;

    QString cachedApiUrl;   /* Full Rumble API URL from Supabase */
    QString currentVideoId;

    QTimer *pollTimer;

    bool isFetchingApiUrl = false;
    int credentialFailCount = 0;
    static constexpr int MAX_CREDENTIAL_RETRIES = 5;
};
