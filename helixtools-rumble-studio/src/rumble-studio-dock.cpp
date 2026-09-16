/*
 * rumble-studio-dock.cpp - Plain browser dock for Rumble Studio
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

#include "rumble-studio-dock.h"
#include "helixtools-rumble-studio.h"
#include "vendor/browser-panel.hpp"

#include <obs-module.h>

RumbleStudioDock::RumbleStudioDock(QCef *cef_, QWidget *parent) : QWidget(parent), cef(cef_)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	/* panel_cookies-equivalent: own persistent cookie jar so the
	 * Rumble login survives across OBS restarts, same as the
	 * integrated app's Rumble Studio dock. */
	BPtr<char> cookiePath = obs_module_config_path("cookies");
	cookies = cef->create_cookie_manager(cookiePath ? cookiePath.Get() : "",
					     true);

	browser = cef->create_widget(this, RUMBLE_STUDIO_URL, cookies);
	layout->addWidget(browser);
}

RumbleStudioDock::~RumbleStudioDock()
{
	if (cookies)
		delete cookies;
}
