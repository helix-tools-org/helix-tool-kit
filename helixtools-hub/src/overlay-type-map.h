/*
 * overlay-type-map.h - Maps Hub overlay URL paths to a display name and
 * default browser size, for Push to Studio.
 *
 * This table must stay in sync with the overlay_defs table in
 * helixtools-overlays/src/overlay-source.c (url_path, display_name and
 * default width/height). source_id is kept for reference only; Push to
 * Studio now creates plain browser sources, not helixtools_* types.
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
	int width;
	int height;
};

static const overlay_type_entry overlay_type_map[] = {
	/* Common */
	{"multichat", "helixtools_common_multichat", "Multi-Platform Chat", 500, 1000},
	{"slot", "helixtools_common_slot", "Slot Machine", 800, 700},
	{"media-looper", "helixtools_common_media_looper", "Media Looper", 1200, 1000},
	{"spinning-wheel", "helixtools_common_spinning_wheel", "Spinning Wheel", 600, 600},
	{"roll-dice", "helixtools_common_roll_dice", "Roll Dice", 500, 500},
	{"coin-flip", "helixtools_common_coin_flip", "Coin Flip", 500, 500},
	{"card-reveal", "helixtools_common_card_reveal", "Card Reveal", 600, 1300},
	{"loot-box", "helixtools_common_loot_box", "Loot Box", 750, 750},
	{"random-picker", "helixtools_common_random_picker", "Random Picker", 750, 750},
	{"roulette", "helixtools_common_roulette", "Roulette", 800, 1000},
	{"magic-8-ball", "helixtools_common_magic_8_ball", "Magic 8-Ball", 400, 500},

	/* Combined */
	{"would-you-rather", "helixtools_combined_would_you_rather", "Would You Rather", 1920, 1080},
	{"hot-takes", "helixtools_combined_hot_takes", "Hot Takes", 1920, 1080},
	{"raffle", "helixtools_combined_raffle", "Raffle", 1920, 1080},

	/* Rumble */
	{"rumble/alerts", "helixtools_overlay_rumble_alerts", "Rumble Alerts", 800, 600},
	{"rumble/chat", "helixtools_overlay_rumble_chat", "Rumble Chat Overlay", 500, 1000},
	{"rumble/chat-commands", "helixtools_overlay_rumble_chatcmd", "Rumble Chat Commands", 1920, 1080},
	{"rumble/goal", "helixtools_overlay_rumble_goal", "Rumble Goal", 1920, 1080},
	{"rumble/like-goal", "helixtools_overlay_rumble_likegoal", "Rumble Like Goal", 1920, 1080},
	{"rumble/emoji", "helixtools_overlay_rumble_emoji", "Rumble Emoji Rain", 1920, 1080},
	{"rumble/scroller", "helixtools_overlay_rumble_scroller", "Rumble Scroller", 1920, 1080},
	{"rumble/static-text", "helixtools_overlay_rumble_text", "Rumble Static Text", 1920, 1080},
	{"rumble/viewer-count", "helixtools_overlay_rumble_viewers", "Rumble Viewer Count", 1920, 1080},
	{"rumble/follower-count", "helixtools_overlay_rumble_followers", "Rumble Follower Count", 1920, 1080},
	{"rumble/subscriber-count", "helixtools_overlay_rumble_subscribers", "Rumble Subscriber Count", 1920, 1080},
	{"rumble/tts", "helixtools_overlay_rumble_tts", "Rumble TTS Narrator", 1920, 1080},
	{"rumble/leaderboard", "helixtools_overlay_rumble_leaderboard", "Rumble Leaderboard", 600, 1080},
	{"rumble/battle-royale", "helixtools_overlay_rumble_battle_royale", "Rumble Battle Royale", 800, 800},

	/* Twitch */
	{"twitch/alerts", "helixtools_overlay_twitch_alerts", "Twitch Alerts", 800, 600},
	{"twitch/chat", "helixtools_overlay_twitch_chat", "Twitch Chat Overlay", 500, 1000},
	{"twitch/goal", "helixtools_overlay_twitch_goal", "Twitch Goal", 1920, 1080},
	{"twitch/emoji", "helixtools_overlay_twitch_emoji", "Twitch Emoji Rain", 1920, 1080},
	{"twitch/scroller", "helixtools_overlay_twitch_scroller", "Twitch Scroller", 1920, 1080},
	{"twitch/static-text", "helixtools_overlay_twitch_text", "Twitch Static Text", 1920, 1080},
	{"twitch/viewer-count", "helixtools_overlay_twitch_viewers", "Twitch Viewer Count", 1920, 1080},
	{"twitch/follower-count", "helixtools_overlay_twitch_followers", "Twitch Follower Count", 1920, 1080},
	{"twitch/subscriber-count", "helixtools_overlay_twitch_subscribers", "Twitch Subscriber Count", 1920, 1080},
	{"twitch/tts", "helixtools_overlay_twitch_tts", "Twitch TTS Narrator", 1920, 1080},
	{"twitch/battle-royale", "helixtools_overlay_twitch_battle_royale", "Twitch Battle Royale", 800, 800},

	/* Blaze */
	{"blaze/alerts", "helixtools_overlay_blaze_alerts", "Blaze Alerts", 800, 600},
	{"blaze/chat", "helixtools_overlay_blaze_chat", "Blaze Chat Overlay", 500, 1000},
	{"blaze/goal", "helixtools_overlay_blaze_goal", "Blaze Goal", 1920, 1080},
	{"blaze/emoji", "helixtools_overlay_blaze_emoji", "Blaze Emoji Rain", 1920, 1080},
	{"blaze/scroller", "helixtools_overlay_blaze_scroller", "Blaze Scroller", 1920, 1080},
	{"blaze/static-text", "helixtools_overlay_blaze_text", "Blaze Static Text", 1920, 1080},
	{"blaze/viewer-count", "helixtools_overlay_blaze_viewers", "Blaze Viewer Count", 1920, 1080},
	{"blaze/follower-count", "helixtools_overlay_blaze_followers", "Blaze Follower Count", 1920, 1080},
	{"blaze/subscriber-count", "helixtools_overlay_blaze_subscribers", "Blaze Subscriber Count", 1920, 1080},
	{"blaze/tts", "helixtools_overlay_blaze_tts", "Blaze TTS Narrator", 1920, 1080},
	{"blaze/leaderboard", "helixtools_overlay_blaze_leaderboard", "Blaze Leaderboard", 600, 1080},
	{"blaze/battle-royale", "helixtools_overlay_blaze_battle_royale", "Blaze Battle Royale", 800, 800},

	/* VPZone */
	{"vpzone/alerts", "helixtools_overlay_vpzone_alerts", "VPZone Alerts", 800, 600},
	{"vpzone/chat", "helixtools_overlay_vpzone_chat", "VPZone Chat Overlay", 500, 1000},
	{"vpzone/goal", "helixtools_overlay_vpzone_goal", "VPZone Goal", 1920, 1080},
	{"vpzone/emoji", "helixtools_overlay_vpzone_emoji", "VPZone Emoji Rain", 1920, 1080},
	{"vpzone/scroller", "helixtools_overlay_vpzone_scroller", "VPZone Scroller", 1920, 1080},
	{"vpzone/static-text", "helixtools_overlay_vpzone_text", "VPZone Static Text", 1920, 1080},
	{"vpzone/viewer-count", "helixtools_overlay_vpzone_viewers", "VPZone Viewer Count", 1920, 1080},
	{"vpzone/follower-count", "helixtools_overlay_vpzone_followers", "VPZone Follower Count", 1920, 1080},
	{"vpzone/subscriber-count", "helixtools_overlay_vpzone_subscribers", "VPZone Subscriber Count", 1920, 1080},
	{"vpzone/tts", "helixtools_overlay_vpzone_tts", "VPZone TTS Narrator", 1920, 1080},
	{"vpzone/battle-royale", "helixtools_overlay_vpzone_battle_royale", "VPZone Battle Royale", 800, 800},
};

static const size_t NUM_OVERLAY_TYPE_MAP =
	sizeof(overlay_type_map) / sizeof(overlay_type_map[0]);
