/*
 * social-sites.h - Sidebar entries for the Social Media dock.
 *
 * Same list as an integrated studio app's socialMediaSites
 * table (obs-studio/frontend/widgets/OBSBasic_Browser.cpp).
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

struct SocialMediaSite {
	const char *label;
	const char *url;
	bool isHeader;
};

static const SocialMediaSite socialMediaSites[] = {
	{"Video",     nullptr,                             true},
	{"Rumble",    "https://rumble.com/account/content", false},
	{"YouTube",   "https://studio.youtube.com",         false},
	{"Kick",      "https://kick.com/dashboard",         false},
	{"Twitch",    "https://dashboard.twitch.tv",        false},
	{"Blaze",     "https://blaze.stream",               false},
	{"VPZone",    "https://vpzone.tv",                  false},
	{"Social",    nullptr,                             true},
	{"Pickax",    "https://pickax.com",                 false},
	{"X",         "https://x.com/home",                 false},
	{"Facebook",  "https://www.facebook.com",           false},
	{"Instagram", "https://www.instagram.com",          false},
	{"TikTok",    "https://www.tiktok.com",             false},
	{"Sticks.gg", "https://sticks.gg",                  false},
	{"Messaging", nullptr,                             true},
	{"Discord",   "https://discord.com/channels/@me",   false},
	{"Telegram",  "https://web.telegram.org",           false},
};

static const size_t NUM_SOCIAL_MEDIA_SITES =
	sizeof(socialMediaSites) / sizeof(socialMediaSites[0]);
