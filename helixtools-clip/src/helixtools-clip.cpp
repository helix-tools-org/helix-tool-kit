/*
 * helixtools-clip.cpp - Plugin entry point
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

#include "helixtools-clip.h"
#include "clip-manager.h"
#include "clip-settings-dialog.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <util/config-file.h>
#include <callback/calldata.h>
#include <callback/proc.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QMainWindow>
#include <QMetaObject>
#include <QStatusBar>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#include <Mmsystem.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winmm.lib")
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-clip", "en-US")

HelixToolsClip *HelixToolsClip::instance_ = nullptr;

HelixToolsClip *HelixToolsClip::instance()
{
    return instance_;
}

static void frontendEventCallback(enum obs_frontend_event event, void *data)
{
    HelixToolsClip *clip = static_cast<HelixToolsClip *>(data);
    if (!clip)
        return;

    switch (event) {
    case OBS_FRONTEND_EVENT_STREAMING_STARTED:
        clip->onStreamStarted();
        break;
    case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
        clip->onStreamStopped();
        break;
    default:
        break;
    }
}

HelixToolsClip::HelixToolsClip()
    : QObject()
    , manager(new ClipManager(this))
{
    instance_ = this;

    connect(manager, &ClipManager::liveStreamFound,
            this, [this](const QString &videoId) {
                currentVideoId = videoId;
                blog(LOG_INFO, "[helixtools-clip] Live stream found: %s",
                     qUtf8Printable(videoId));
            });
    connect(manager, &ClipManager::clipCreated,
            this, &HelixToolsClip::clipCreated);
    connect(manager, &ClipManager::clipCreated,
            this, [this](const QString &clipId, const QString &) {
                if (showToast)
                    showStatusMessage(QString("Clip created successfully! (ID: %1)").arg(clipId));
                if (playSound)
                    playFeedbackSound();
                if (showOverlay) {
                    /* Send clip event to Hub alert system via
                       helixtools-hub proc handler (if available) */
                    obs_source_t *hubSrc = obs_get_source_by_name(
                        "helixtools_hub_service");
                    if (hubSrc) {
                        proc_handler_t *ph =
                            obs_source_get_proc_handler(hubSrc);
                        calldata_t cd = {};
                        calldata_set_string(&cd, "event",
                                            "clip_created");
                        calldata_set_string(&cd, "clip_id",
                                            qUtf8Printable(clipId));
                        proc_handler_call(ph, "send_alert", &cd);
                        calldata_free(&cd);
                        obs_source_release(hubSrc);
                    }
                }
            });
    connect(manager, &ClipManager::clipError,
            this, &HelixToolsClip::clipFailed);
    connect(manager, &ClipManager::clipError,
            this, [this](const QString &error) {
                showStatusMessage(QString("Clip failed: %1").arg(error));
            });

    obs_frontend_add_event_callback(frontendEventCallback, this);

    loadSettings();
    registerHotkeys();

    blog(LOG_INFO, "[helixtools-clip] Plugin loaded v%s", HELIXTOOLS_CLIP_VERSION);
}

HelixToolsClip::~HelixToolsClip()
{
    obs_frontend_remove_event_callback(frontendEventCallback, this);
    stopPolling();
    unregisterHotkeys();
    instance_ = nullptr;
}

void HelixToolsClip::loadSettings()
{
    // Use profile config to match frontend Settings dialog (was global config)
    config_t *config = obs_frontend_get_profile_config();
    if (!config)
        return;

    showStatus = config_get_bool(config, "Clips", SETTINGS_SHOW_STATUS);
    playSound = config_get_bool(config, "Clips", SETTINGS_PLAY_SOUND);

    const char *path = config_get_string(config, "Clips", SETTINGS_CUSTOM_SOUND);
    customSoundPath = path ? QString::fromUtf8(path) : QString();

    /* Defaults: all true on first run (config returns false for missing keys) */
    if (!config_has_user_value(config, "Clips", SETTINGS_SHOW_STATUS))
        showStatus = true;
    if (!config_has_user_value(config, "Clips", SETTINGS_PLAY_SOUND))
        playSound = true;

    showToast = config_has_user_value(config, "Clips", SETTINGS_SHOW_TOAST)
        ? config_get_bool(config, "Clips", SETTINGS_SHOW_TOAST)
        : true;
    showOverlay = config_has_user_value(config, "Clips", SETTINGS_SHOW_OVERLAY)
        ? config_get_bool(config, "Clips", SETTINGS_SHOW_OVERLAY)
        : true;
}

void HelixToolsClip::saveSettings()
{
    // Use profile config to match frontend Settings dialog (was global config)
    config_t *config = obs_frontend_get_profile_config();
    if (!config)
        return;

    config_set_bool(config, "Clips", SETTINGS_SHOW_STATUS, showStatus);
    config_set_bool(config, "Clips", SETTINGS_PLAY_SOUND, playSound);
    config_set_string(config, "Clips", SETTINGS_CUSTOM_SOUND,
                      customSoundPath.toUtf8().constData());
    config_set_bool(config, "Clips", SETTINGS_SHOW_TOAST, showToast);
    config_set_bool(config, "Clips", SETTINGS_SHOW_OVERLAY, showOverlay);
    config_save(config);
}

void HelixToolsClip::registerHotkeys()
{
    obs_hotkey_id id30s = obs_hotkey_register_frontend(
        "helixtools_clip_30s",
        "Clip: 30 Second",
        [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
            if (pressed && data)
                static_cast<HelixToolsClip*>(data)->triggerClip30s();
        },
        this);

    obs_hotkey_id id60s = obs_hotkey_register_frontend(
        "helixtools_clip_60s",
        "Clip: 60 Second",
        [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
            if (pressed && data)
                static_cast<HelixToolsClip*>(data)->triggerClip60s();
        },
        this);

    hotkey30s = id30s;
    hotkey60s = id60s;

    // Restore saved hotkey bindings from profile config on startup
    // OBSBasic::CreateHotkeys only loads bindings for its own hotkeys, so
    // plugin-registered hotkeys must load their own saved bindings.
    auto loadSavedBinding = [](obs_hotkey_id id, const char *name) {
        config_t *config = obs_frontend_get_profile_config();
        if (!config)
            return;
        const char *json = config_get_string(config, "Hotkeys", name);
        if (!json)
            return;
        obs_data_t *data = obs_data_create_from_json(json);
        if (!data)
            return;
        obs_data_array_t *array = obs_data_get_array(data, "bindings");
        obs_hotkey_load(id, array);
        obs_data_array_release(array);
        obs_data_release(data);
    };

    loadSavedBinding(hotkey30s, "helixtools_clip_30s");
    loadSavedBinding(hotkey60s, "helixtools_clip_60s");

    blog(LOG_INFO, "[helixtools-clip] Hotkeys registered");
}

void HelixToolsClip::unregisterHotkeys()
{
    if (hotkey30s != OBS_INVALID_HOTKEY_ID) {
        obs_hotkey_unregister(hotkey30s);
        hotkey30s = OBS_INVALID_HOTKEY_ID;
    }
    if (hotkey60s != OBS_INVALID_HOTKEY_ID) {
        obs_hotkey_unregister(hotkey60s);
        hotkey60s = OBS_INVALID_HOTKEY_ID;
    }
}

void HelixToolsClip::triggerClip30s()
{
    createClip(30);
}

void HelixToolsClip::triggerClip60s()
{
    createClip(60);
}

void HelixToolsClip::createClip(int seconds)
{
    // Show user-visible status bar feedback for all error/success cases

    if (!isStreaming) {
        showStatusMessage(QString("Clip failed: You are not currently streaming"));
        blog(LOG_WARNING, "[helixtools-clip] Clip blocked: not streaming");
        emit clipFailed("Not streaming");
        return;
    }

    if (isCooldownActive()) {
        qint64 remaining = getCooldownRemaining() / 1000;
        showStatusMessage(QString("Clip on cooldown: %1s remaining").arg(remaining));
        blog(LOG_WARNING, "[helixtools-clip] Clip blocked: cooldown active");
        emit clipFailed("Cooldown active");
        return;
    }

    QString videoId = manager->getCurrentVideoId();
    if (videoId.isEmpty()) {
        showStatusMessage(QString("Clip failed: No Rumble live stream detected"));
        blog(LOG_WARNING, "[helixtools-clip] Clip blocked: no active stream");
        emit clipFailed("No active live stream");
        return;
    }

#ifdef _WIN32
    PCREDENTIALW cred = nullptr;
    QString token, userId;

    if (CredReadW(HUB_CREDENTIAL_TOKEN, CRED_TYPE_GENERIC, 0, &cred)) {
        token = QString::fromUtf8((const char*)cred->CredentialBlob,
                                   cred->CredentialBlobSize);
        CredFree(cred);
    }

    if (CredReadW(HUB_CREDENTIAL_USERID, CRED_TYPE_GENERIC, 0, &cred)) {
        userId = QString::fromUtf8((const char*)cred->CredentialBlob,
                                    cred->CredentialBlobSize);
        CredFree(cred);
    }

    if (token.isEmpty() || userId.isEmpty()) {
        showStatusMessage(QString("Clip failed: Not logged in to HelixTools Hub"));
        blog(LOG_WARNING, "[helixtools-clip] Clip blocked: not logged in");
        emit clipFailed("Not logged in");
        return;
    }

    showStatusMessage(QString("Creating %1s clip...").arg(seconds));

    manager->createClip(videoId, seconds, userId, token);

    lastClipTime = QDateTime::currentMSecsSinceEpoch();
    emit cooldownStarted();

    // Re-read from config so Settings changes take effect immediately
    loadSettings();

    if (playSound) {
        playFeedbackSound();
    }
#else
    showStatusMessage(QString("Clip failed: Windows required"));
    emit clipFailed("Windows required");
#endif
}

bool HelixToolsClip::isCooldownActive() const
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    return (now - lastClipTime) < CLIP_COOLDOWN_MS;
}

qint64 HelixToolsClip::getCooldownRemaining() const
{
    qint64 remaining = CLIP_COOLDOWN_MS -
        (QDateTime::currentMSecsSinceEpoch() - lastClipTime);
    return qMax<qint64>(0, remaining);
}

void HelixToolsClip::playFeedbackSound()
{
#ifdef _WIN32
    if (!customSoundPath.isEmpty() && QFile::exists(customSoundPath)) {
        // Stop any previous clip sound
        mciSendStringW(L"close clipSound", nullptr, 0, nullptr);
        // Use MCI for MP3/WAV/WMA support (PlaySoundW only handles WAV)
        QString open = QString("open \"%1\" type mpegvideo alias clipSound")
                           .arg(QDir::toNativeSeparators(customSoundPath));
        mciSendStringW((LPCWSTR)open.utf16(), nullptr, 0, nullptr);
        mciSendStringW(L"play clipSound from 0", nullptr, 0, nullptr);
    } else if (playSound) {
        PlaySound(TEXT("SystemAsterisk"), nullptr,
                  SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
    }
#endif
}

void HelixToolsClip::showStatusMessage(const QString &message)
{
    /* OBS's main window is a real QMainWindow, and its status bar is a
     * standard QStatusBar reachable via QMainWindow::statusBar() — no
     * OBS-specific API needed, so this works on vanilla OBS Studio. */
    void *mainPtr = obs_frontend_get_main_window();
    if (mainPtr) {
        QMainWindow *mainWindow = static_cast<QMainWindow *>(mainPtr);
        if (QStatusBar *statusBar = mainWindow->statusBar()) {
            QMetaObject::invokeMethod(
                statusBar,
                [statusBar, message]() {
                    statusBar->clearMessage();
                    statusBar->showMessage(message, 10000);
                },
                Qt::QueuedConnection);
        }
    }

    if (showToast)
        showToastNotification(message);

    emit statusMessage(message);
    blog(LOG_INFO, "[helixtools-clip] %s", qUtf8Printable(message));
}

void HelixToolsClip::showToastNotification(const QString &message)
{
    /* OBS's status bar text is easy to miss at a glance, so this pops up a
     * small floating panel over the bottom-right of the main window as
     * well — frameless, translucent, self-closing, no external toast
     * library needed. */
    void *mainPtr = obs_frontend_get_main_window();
    if (!mainPtr)
        return;
    QMainWindow *mainWindow = static_cast<QMainWindow *>(mainPtr);

    QMetaObject::invokeMethod(
        mainWindow,
        [mainWindow, message]() {
            auto *toast = new QWidget(mainWindow, Qt::Tool |
                                                   Qt::FramelessWindowHint |
                                                   Qt::WindowStaysOnTopHint);
            toast->setAttribute(Qt::WA_ShowWithoutActivating);
            toast->setAttribute(Qt::WA_DeleteOnClose);
            toast->setAttribute(Qt::WA_StyledBackground);
            toast->setStyleSheet(
                "QWidget { background-color: rgba(32, 32, 36, 235);"
                "border: 1px solid rgba(90, 200, 130, 200);"
                "border-radius: 6px; }");

            auto *label = new QLabel(message, toast);
            label->setStyleSheet(
                "color: white; font-size: 13px; padding: 10px 16px; background: transparent;");
            label->setWordWrap(true);

            auto *layout = new QVBoxLayout(toast);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(label);

            toast->adjustSize();
            int width = qMin(360, qMax(220, label->sizeHint().width() + 32));
            toast->setFixedWidth(width);
            toast->adjustSize();

            QRect area = mainWindow->geometry();
            int x = area.x() + area.width() - toast->width() - 24;
            int y = area.y() + area.height() - toast->height() - 48;
            toast->move(x, y);

            auto *effect = new QGraphicsOpacityEffect(toast);
            toast->setGraphicsEffect(effect);
            effect->setOpacity(0.0);
            toast->show();

            auto *fadeIn = new QPropertyAnimation(effect, "opacity", toast);
            fadeIn->setDuration(200);
            fadeIn->setStartValue(0.0);
            fadeIn->setEndValue(1.0);
            fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

            QTimer::singleShot(3500, toast, [toast, effect]() {
                auto *fadeOut = new QPropertyAnimation(effect, "opacity", toast);
                fadeOut->setDuration(400);
                fadeOut->setStartValue(1.0);
                fadeOut->setEndValue(0.0);
                QObject::connect(fadeOut, &QPropertyAnimation::finished,
                                  toast, &QWidget::close);
                fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
            });
        },
        Qt::QueuedConnection);
}

void HelixToolsClip::startPolling()
{
    manager->fetchRumbleApiUrl();
    manager->pollLiveStream();
    manager->startPollTimer(CLIP_POLL_INTERVAL_MS);
    blog(LOG_INFO, "[helixtools-clip] Started polling for live streams");
}

void HelixToolsClip::stopPolling()
{
    manager->stopPollTimer();
    currentVideoId.clear();
    blog(LOG_INFO, "[helixtools-clip] Stopped polling");
}

void HelixToolsClip::onStreamStarted()
{
    isStreaming = true;
    startPolling();
}

void HelixToolsClip::onStreamStopped()
{
    isStreaming = false;
    stopPolling();
}

void HelixToolsClip::onSettingsChanged()
{
    loadSettings();
}

void HelixToolsClip::SetShowStatus(bool show)
{
    showStatus = show;
    saveSettings();
}

void HelixToolsClip::SetPlaySound(bool play)
{
    playSound = play;
    saveSettings();
}

void HelixToolsClip::SetCustomSound(const QString &path)
{
    customSoundPath = path;
    saveSettings();
}

void HelixToolsClip::SetShowToast(bool show)
{
    showToast = show;
    saveSettings();
}

void HelixToolsClip::SetShowOverlay(bool show)
{
    showOverlay = show;
    saveSettings();
}

/* OBS Plugin Entry Points */

bool obs_module_load()
{
    blog(LOG_INFO, "[helixtools-clip] Loading plugin v%s", HELIXTOOLS_CLIP_VERSION);

    if (!QCoreApplication::instance()) {
        blog(LOG_ERROR, "[helixtools-clip] No QApplication available");
        return false;
    }

    // Actually create the singleton (was calling instance() which returned nullptr)
    if (!HelixToolsClip::instance_) {
        new HelixToolsClip();
    }

    helixtools_clip_init_menu();

    return true;
}

void obs_module_unload()
{
    blog(LOG_INFO, "[helixtools-clip] Unloading plugin");

    if (HelixToolsClip::instance_) {
        delete HelixToolsClip::instance_;
        HelixToolsClip::instance_ = nullptr;
    }
}

const char *obs_module_name()
{
    return "HelixTools Clip";
}

const char *obs_module_description()
{
    return "Clip creation for Rumble streams";
}
