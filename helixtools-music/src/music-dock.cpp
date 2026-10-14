/*
 * music-dock.cpp - Plain browser dock for the HelixTools Music player
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

#include "music-dock.h"
#include "helixtools-music.h"
#include "music-auth-script.h"
#include "music-credentials.h"
#include "vendor/browser-panel.hpp"

#include <obs-module.h>

#include <string>
#include <windows.h>
#include <wincred.h>

static bool writeCredential(const wchar_t *target, const std::string &value)
{
	CREDENTIALW cred = {};
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = const_cast<LPWSTR>(target);
	cred.CredentialBlobSize = (DWORD)value.size();
	cred.CredentialBlob = (LPBYTE)value.data();
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	return CredWriteW(&cred, 0) == TRUE;
}

namespace {
/*
 * The site opens a companion "mini player" popup window (its own
 * /mini-player route) via window.open() when playback starts. Left
 * uncontrolled, that becomes a bare, chromeless native window sitting
 * on top of everything with no close affordance of its own.
 *
 * obs-browser's own popup-handling API (QCefWidget::allowAllPopups,
 * QCefWidget::popupRequested) turned out to be unreliable here — the
 * popupRequested signal's exact signature differs between obs-browser
 * builds (confirmed: this OBS install's obs-browser 2.26.9 logs
 * "No such signal QCefWidgetInternal::popupRequested(const QString &)"
 * when we try to connect to it), and allowAllPopups(false) alone did
 * not stop the popup from opening as an uncontrolled window either.
 *
 * Neutralizing window.open() in the page itself sidesteps obs-browser's
 * popup machinery entirely, so it can't drift out from under us on a
 * different OBS/obs-browser version the way the signal-based approach
 * just did. The mini-player feature simply won't have anywhere to pop
 * out to; that's an acceptable tradeoff for not having a stray
 * uncloseable window on top of everything.
 */
static const char *DISABLE_POPUPS_SCRIPT =
	"(function(){ window.open = function(){ return null; }; })();";

/*
 * The dock is a plain, unmuted browser widget, so the page's own
 * <audio>/<video> elements play through the system's normal audio
 * output — captured live by "Desktop Audio" like any other app. That
 * was fine before MusicManager existed (it was the only real audio
 * path), but now MusicManager (music-manager.cpp) separately decodes
 * and plays the same track again as a native ffmpeg_source mixer
 * channel, mirroring the dock's playback_status for the Mixer fader to
 * actually do something. With the page itself left unmuted, both paths
 * play for real at once — Desktop Audio capturing the page's own sound
 * and the Mixer channel decoding it independently — so viewers hear the
 * track twice (confirmed via the ~1.5s gap between them, matching
 * MusicManager's own poll interval). Muting every media element here
 * makes the native mixer channel the sole real audio path, matching the
 * original intent (the Mixer fader controls the same audio you hear
 * from the dock), which assumed there'd only ever be one real playing
 * instance. A MutationObserver
 * is needed because the player mounts a fresh <audio> element per
 * track rather than reusing one.
 */
static const char *MUTE_AUDIO_SCRIPT =
	"(function(){"
	"  function muteAll(){"
	"    document.querySelectorAll('audio,video').forEach(function(el){"
	"      el.muted = true; el.volume = 0;"
	"    });"
	"  }"
	"  muteAll();"
	"  new MutationObserver(muteAll).observe(document.documentElement, "
	"    { childList: true, subtree: true });"
	"})();";
} // namespace

MusicDock::MusicDock(QCef *cef_, QWidget *parent) : QWidget(parent), cef(cef_)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	/* Own cookie storage, separate from the Hub dock's — HelixTools
	 * Music is a distinct Supabase project/login from the Hub, so
	 * sharing a cookie jar wouldn't help and could only confuse the
	 * two sessions. */
	BPtr<char> cookiePath = obs_module_config_path("cookies");
	cookies = cef->create_cookie_manager(cookiePath ? cookiePath.Get() : "",
					     true);

	browser = cef->create_widget(this, MUSIC_PLAYER_URL, cookies);
	browser->setStartupScript(std::string(DISABLE_POPUPS_SCRIPT) + MUTE_AUDIO_SCRIPT +
				   HELIXTOOLS_MUSIC_AUTH_SCRIPT);
	layout->addWidget(browser);

	/* QCefWidget's real implementation lives inside obs-browser.dll,
	 * loaded dynamically at runtime — its meta-object symbols aren't
	 * linkable at build time, so the new-style &QCefWidget::titleChanged
	 * connect syntax fails to link. The old string-based SIGNAL/SLOT
	 * form resolves via the object's actual (runtime) meta-object
	 * instead, which works fine here. */
	connect(browser, SIGNAL(titleChanged(const QString &)), this,
		SLOT(onTitleChanged(const QString &)));
}

MusicDock::~MusicDock()
{
	if (cookies)
		delete cookies;
}

void MusicDock::onTitleChanged(const QString &title)
{
	static const QString authPrefix = "HELIXTOOLS_MUSIC_AUTH:";

	if (title.startsWith(authPrefix))
		handleAuthCommand(title.mid(authPrefix.length()));
}

void MusicDock::handleAuthCommand(const QString &json)
{
	obs_data_t *cmd = obs_data_create_from_json(json.toUtf8().constData());
	if (!cmd) {
		blog(LOG_WARNING, "[helixtools-music] Auth capture: bad JSON");
		return;
	}

	const char *userId = obs_data_get_string(cmd, "userId");
	if (!userId || !*userId) {
		blog(LOG_WARNING, "[helixtools-music] Auth capture: missing userId");
		obs_data_release(cmd);
		return;
	}

	bool ok = writeCredential(MUSIC_CRED_USERID, userId);
	blog(LOG_INFO, "[helixtools-music] Auth capture: %s",
	     ok ? "cached Music user ID to Credential Manager"
		: "failed to write credential");

	obs_data_release(cmd);
}
