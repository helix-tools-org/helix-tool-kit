/*
 * overlay-type-map.h - Maps Hub overlay URL paths to helixtools-overlays
 * source type IDs, for Push to Studio.
 *
 * This table must stay in sync with the overlay_defs table in
 * helixtools-overlays/src/overlay-source.c (source_id + url_path +
 * display_name columns only — this plugin doesn't need dimensions).
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

struct overlay_type_entry {
	const char *url_path;
	const char *source_id;
	const char *display_name;
};

static const overlay_type_entry overlay_type_map[] = {
	/* Common */
	{"multichat", "helixtools_common_multichat", "Multi-Platform Chat"},
	{"slot", "helixtools_common_slot", "Slot Machine"},
	{"media-looper", "helixtools_common_media_looper", "Media Looper"},
	{"spinning-wheel", "helixtools_common_spinning_wheel", "Spinning Wheel"},
	{"roll-dice", "helixtools_common_roll_dice", "Roll Dice"},
	{"coin-flip", "helixtools_common_coin_flip", "Coin Flip"},
	{"card-reveal", "helixtools_common_card_reveal", "Card Reveal"},
	{"loot-box", "helixtools_common_loot_box", "Loot Box"},
	{"random-picker", "helixtools_common_random_picker", "Random Picker"},
	{"roulette", "helixtools_common_roulette", "Roulette"},
	{"magic-8-ball", "helixtools_common_magic_8_ball", "Magic 8-Ball"},

	/* Combined */
	{"would-you-rather", "helixtools_combined_would_you_rather", "Would You Rather"},
	{"hot-takes", "helixtools_combined_hot_takes", "Hot Takes"},
	{"raffle", "helixtools_combined_raffle", "Raffle"},

	/* Rumble */
	{"rumble/alerts", "helixtools_overlay_rumble_alerts", "Rumble Alerts"},
	{"rumble/chat", "helixtools_overlay_rumble_chat", "Rumble Chat Overlay"},
	{"rumble/chat-commands", "helixtools_overlay_rumble_chatcmd", "Rumble Chat Commands"},
	{"rumble/goal", "helixtools_overlay_rumble_goal", "Rumble Goal"},
	{"rumble/like-goal", "helixtools_overlay_rumble_likegoal", "Rumble Like Goal"},
	{"rumble/emoji", "helixtools_overlay_rumble_emoji", "Rumble Emoji Rain"},
	{"rumble/scroller", "helixtools_overlay_rumble_scroller", "Rumble Scroller"},
	{"rumble/static-text", "helixtools_overlay_rumble_text", "Rumble Static Text"},
	{"rumble/viewer-count", "helixtools_overlay_rumble_viewers", "Rumble Viewer Count"},
	{"rumble/follower-count", "helixtools_overlay_rumble_followers", "Rumble Follower Count"},
	{"rumble/subscriber-count", "helixtools_overlay_rumble_subscribers", "Rumble Subscriber Count"},
	{"rumble/tts", "helixtools_overlay_rumble_tts", "Rumble TTS Narrator"},
	{"rumble/leaderboard", "helixtools_overlay_rumble_leaderboard", "Rumble Leaderboard"},
	{"rumble/battle-royale", "helixtools_overlay_rumble_battle_royale", "Rumble Battle Royale"},

	/* Twitch */
	{"twitch/alerts", "helixtools_overlay_twitch_alerts", "Twitch Alerts"},
	{"twitch/chat", "helixtools_overlay_twitch_chat", "Twitch Chat Overlay"},
	{"twitch/goal", "helixtools_overlay_twitch_goal", "Twitch Goal"},
	{"twitch/emoji", "helixtools_overlay_twitch_emoji", "Twitch Emoji Rain"},
	{"twitch/scroller", "helixtools_overlay_twitch_scroller", "Twitch Scroller"},
	{"twitch/static-text", "helixtools_overlay_twitch_text", "Twitch Static Text"},
	{"twitch/viewer-count", "helixtools_overlay_twitch_viewers", "Twitch Viewer Count"},
	{"twitch/follower-count", "helixtools_overlay_twitch_followers", "Twitch Follower Count"},
	{"twitch/subscriber-count", "helixtools_overlay_twitch_subscribers", "Twitch Subscriber Count"},
	{"twitch/tts", "helixtools_overlay_twitch_tts", "Twitch TTS Narrator"},
	{"twitch/battle-royale", "helixtools_overlay_twitch_battle_royale", "Twitch Battle Royale"},

	/* Blaze */
	{"blaze/alerts", "helixtools_overlay_blaze_alerts", "Blaze Alerts"},
	{"blaze/chat", "helixtools_overlay_blaze_chat", "Blaze Chat Overlay"},
	{"blaze/goal", "helixtools_overlay_blaze_goal", "Blaze Goal"},
	{"blaze/emoji", "helixtools_overlay_blaze_emoji", "Blaze Emoji Rain"},
	{"blaze/scroller", "helixtools_overlay_blaze_scroller", "Blaze Scroller"},
	{"blaze/static-text", "helixtools_overlay_blaze_text", "Blaze Static Text"},
	{"blaze/viewer-count", "helixtools_overlay_blaze_viewers", "Blaze Viewer Count"},
	{"blaze/follower-count", "helixtools_overlay_blaze_followers", "Blaze Follower Count"},
	{"blaze/subscriber-count", "helixtools_overlay_blaze_subscribers", "Blaze Subscriber Count"},
	{"blaze/tts", "helixtools_overlay_blaze_tts", "Blaze TTS Narrator"},
	{"blaze/leaderboard", "helixtools_overlay_blaze_leaderboard", "Blaze Leaderboard"},
	{"blaze/battle-royale", "helixtools_overlay_blaze_battle_royale", "Blaze Battle Royale"},

	/* VPZone */
	{"vpzone/alerts", "helixtools_overlay_vpzone_alerts", "VPZone Alerts"},
	{"vpzone/chat", "helixtools_overlay_vpzone_chat", "VPZone Chat Overlay"},
	{"vpzone/goal", "helixtools_overlay_vpzone_goal", "VPZone Goal"},
	{"vpzone/emoji", "helixtools_overlay_vpzone_emoji", "VPZone Emoji Rain"},
	{"vpzone/scroller", "helixtools_overlay_vpzone_scroller", "VPZone Scroller"},
	{"vpzone/static-text", "helixtools_overlay_vpzone_text", "VPZone Static Text"},
	{"vpzone/viewer-count", "helixtools_overlay_vpzone_viewers", "VPZone Viewer Count"},
	{"vpzone/follower-count", "helixtools_overlay_vpzone_followers", "VPZone Follower Count"},
	{"vpzone/subscriber-count", "helixtools_overlay_vpzone_subscribers", "VPZone Subscriber Count"},
	{"vpzone/tts", "helixtools_overlay_vpzone_tts", "VPZone TTS Narrator"},
	{"vpzone/battle-royale", "helixtools_overlay_vpzone_battle_royale", "VPZone Battle Royale"},
};

static const size_t NUM_OVERLAY_TYPE_MAP =
	sizeof(overlay_type_map) / sizeof(overlay_type_map[0]);
