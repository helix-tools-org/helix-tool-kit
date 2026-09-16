/*
 * helixtools-clip.h - Clip creation plugin for Rumble streams
 *
 * Provides hotkey-triggered clip creation (30s or 60s) from Rumble live streams.
 * Uses Supabase authentication and Hub APIs.
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

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QString>
#include <QObject>

#ifndef HELIXTOOLS_CLIP_VERSION
#define HELIXTOOLS_CLIP_VERSION "0.1.0"
#endif

/* Supabase project + clip API endpoint. Not defined here — see
 * clip-backend-config.h (gitignored, private) and
 * clip-backend-config.h.example (the public template). */
#include "clip-backend-config.h"

/* Windows Credential Manager target names */
#define HUB_CREDENTIAL_TOKEN    L"HelixTools/HubToken"
#define HUB_CREDENTIAL_USERID   L"HelixTools/HubUserId"

/* Settings keys */
#define SETTINGS_SHOW_STATUS    "ClipShowStatus"
#define SETTINGS_PLAY_SOUND     "ClipPlaySound"
#define SETTINGS_CUSTOM_SOUND   "ClipCustomSound"
#define SETTINGS_SHOW_TOAST     "ClipShowToast"
#define SETTINGS_SHOW_OVERLAY   "ClipShowOverlay"
#define SETTINGS_HOTKEY_30S     "ClipHotkey30s"
#define SETTINGS_HOTKEY_60S     "ClipHotkey60s"

/* Cooldown (milliseconds) */
#define CLIP_COOLDOWN_MS        15000

/* Polling interval (milliseconds) */
#define CLIP_POLL_INTERVAL_MS   30000

class ClipManager;

class HelixToolsClip : public QObject {
    Q_OBJECT

public:
    HelixToolsClip();
    ~HelixToolsClip();

    void setHotkey30s(obs_hotkey_id id) { hotkey30s = id; }
    void setHotkey60s(obs_hotkey_id id) { hotkey60s = id; }
    obs_hotkey_id getHotkey30s() const { return hotkey30s; }
    obs_hotkey_id getHotkey60s() const { return hotkey60s; }

    void triggerClip30s();
    void triggerClip60s();

    bool isCooldownActive() const;
    qint64 getCooldownRemaining() const;

    QString getCurrentVideoId() const { return currentVideoId; }

    static HelixToolsClip *instance();

public slots:
    void onStreamStarted();
    void onStreamStopped();
    void onSettingsChanged();

public:
    void loadSettings();
    void saveSettings();
    void registerHotkeys();
    void unregisterHotkeys();

    void SetShowStatus(bool show);
    void SetPlaySound(bool play);
    void SetCustomSound(const QString &path);

signals:
    void clipCreated(const QString &clipId, const QString &mp4Url);
    void clipFailed(const QString &error);
    void cooldownStarted();
    void statusMessage(const QString &message);

private:
    void startPolling();
    void stopPolling();
    void createClip(int seconds);

    void playFeedbackSound();
    void showStatusMessage(const QString &message);

    ClipManager *manager;

    obs_hotkey_id hotkey30s = OBS_INVALID_HOTKEY_ID;
    obs_hotkey_id hotkey60s = OBS_INVALID_HOTKEY_ID;

    qint64 lastClipTime = 0;
    QString currentVideoId;
    bool isStreaming = false;

    bool showStatus = true;
    bool playSound = true;
    bool showToast = true;
    bool showOverlay = true;
    QString customSoundPath;

    static HelixToolsClip *instance_;

    friend bool obs_module_load();
    friend void obs_module_unload();
};
