/*
 * dashboard-sites.h - The platform choices for the Dashboard dock's
 * sidebar list.
 *
 * Twitch/YouTube match the (bare-root) URLs already used in
 * helixtools-social's socialMediaSites table. Blaze/VPZone/Kick use
 * newer, more specific dashboard-subdomain URLs than that table has —
 * confirmed against the live sites 2026-09-22, not just copied.
 *
 * Twitch/YouTube's bare roots rely on each site redirecting an
 * already-logged-in session (this dock's persistent cookie jar) to
 * that account's own dashboard/studio home — neither has a static
 * URL that works for every user account the way the other three do.
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

struct DashboardPlatform {
	const char *label;
	const char *url;
	/* Relative path under this module's data dir (data/logos/), loaded
	 * via obs_module_file() — see dashboard-dock.cpp. */
	const char *iconFile;
};

static const DashboardPlatform dashboardPlatforms[] = {
	{"Rumble Studio", "https://studio.rumble.com", "logos/rumble.png"},
	{"Twitch", "https://dashboard.twitch.tv", "logos/twitch.png"},
	{"YouTube", "https://studio.youtube.com", "logos/youtube.png"},
	{"Kick", "https://dashboard.kick.com/stream", "logos/kick.png"},
	{"Blaze", "https://blaze.stream/stream-settings/live-dashboard", "logos/blaze.png"},
	{"VPZone", "https://dashboard.vpzone.tv/stream", "logos/vpzone.png"},
};

static const size_t NUM_DASHBOARD_PLATFORMS =
	sizeof(dashboardPlatforms) / sizeof(dashboardPlatforms[0]);
