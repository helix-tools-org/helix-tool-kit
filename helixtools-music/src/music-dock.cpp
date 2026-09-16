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
#include "vendor/browser-panel.hpp"

#include <obs-module.h>

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
	browser->setStartupScript(DISABLE_POPUPS_SCRIPT);
	layout->addWidget(browser);
}

MusicDock::~MusicDock()
{
	if (cookies)
		delete cookies;
}
