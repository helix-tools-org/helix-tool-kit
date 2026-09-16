/*
 * overlay-source.c - Data-driven browser source for Hub overlays
 *
 * Shared implementation for all Hub overlay/game sources. Each
 * registered source's type_data points to its helixtools_overlay_def
 * entry, which defines the URL path, dimensions, and properties.
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

#include "helixtools-overlays.h"

#include <util/dstr.h>

#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#endif

#define OVERLAY_DEFAULT_CSS \
	"body { background-color: rgba(0, 0, 0, 0); " \
	"margin: 0px auto; overflow: hidden; }"

/* ------------------------------------------------------------------ */
/* Overlay definition table                                            */
/* ------------------------------------------------------------------ */

const struct helixtools_overlay_def overlay_defs[] = {
	/* ---- Generic web page (used by "Add URL as Source" context menu) ---- */
	{"helixtools_overlay_browser", "", "Web Page",
	 false, NULL, 1920, 1080},

	/* ================= Common (platform-agnostic) ================= */
	{"helixtools_common_multichat",    "multichat",     "Multi-Platform Chat",
	 false, NULL, 500, 1000},
	{"helixtools_common_slot",         "slot",          "Slot Machine",
	 false, NULL, 800, 700},
	{"helixtools_common_media_looper", "media-looper",  "Media Looper",
	 false, NULL, 1200, 1000},
	{"helixtools_common_spinning_wheel", "spinning-wheel", "Spinning Wheel",
	 true, "Spinning Wheel ID", 600, 600},
	{"helixtools_common_roll_dice",    "roll-dice",     "Roll Dice",
	 true, "Dice Roll ID", 500, 500},
	{"helixtools_common_coin_flip",    "coin-flip",     "Coin Flip",
	 true, "Coin Flip ID", 500, 500},
	{"helixtools_common_card_reveal",  "card-reveal",   "Card Reveal",
	 true, "Card Reveal ID", 600, 1300},
	{"helixtools_common_loot_box",     "loot-box",      "Loot Box",
	 true, "Loot Box ID", 750, 750},
	{"helixtools_common_random_picker","random-picker", "Random Picker",
	 true, "Picker ID", 750, 750},
	{"helixtools_common_roulette",     "roulette",      "Roulette",
	 true, "Roulette ID", 800, 1000},
	{"helixtools_common_magic_8_ball", "magic-8-ball",  "Magic 8-Ball",
	 true, "8-Ball ID", 400, 500},

	/* ================= Combined (multi-platform, token-optional) ================= */
	{"helixtools_combined_would_you_rather", "would-you-rather", "Would You Rather",
	 false, NULL, 1920, 1080},
	{"helixtools_combined_hot_takes",        "hot-takes",        "Hot Takes",
	 false, NULL, 1920, 1080},
	{"helixtools_combined_raffle",           "raffle",           "Raffle",
	 false, NULL, 1920, 1080},

	/* ================= Rumble ================= */
	{"helixtools_overlay_rumble_alerts",     "rumble/alerts",     "Rumble Alerts",
	 false, NULL, 800, 600},
	{"helixtools_overlay_rumble_chat",       "rumble/chat",       "Rumble Chat Overlay",
	 false, NULL, 500, 1000},
	{"helixtools_overlay_rumble_chatcmd",    "rumble/chat-commands", "Rumble Chat Commands",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_goal",       "rumble/goal",       "Rumble Goal",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_likegoal",   "rumble/like-goal",  "Rumble Like Goal",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_emoji",      "rumble/emoji",      "Rumble Emoji Rain",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_scroller",   "rumble/scroller",   "Rumble Scroller",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_text",       "rumble/static-text","Rumble Static Text",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_viewers",    "rumble/viewer-count", "Rumble Viewer Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_followers",  "rumble/follower-count", "Rumble Follower Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_subscribers","rumble/subscriber-count", "Rumble Subscriber Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_tts",        "rumble/tts",        "Rumble TTS Narrator",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_rumble_leaderboard","rumble/leaderboard","Rumble Leaderboard",
	 true, "Leaderboard Key", 600, 1080},
	{"helixtools_overlay_rumble_battle_royale", "rumble/battle-royale", "Rumble Battle Royale",
	 true, "Battle Royale ID", 800, 800},

	/* ================= Twitch ================= */
	{"helixtools_overlay_twitch_alerts",     "twitch/alerts",     "Twitch Alerts",
	 false, NULL, 800, 600},
	{"helixtools_overlay_twitch_chat",       "twitch/chat",       "Twitch Chat Overlay",
	 false, NULL, 500, 1000},
	{"helixtools_overlay_twitch_goal",       "twitch/goal",       "Twitch Goal",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_emoji",      "twitch/emoji",      "Twitch Emoji Rain",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_scroller",   "twitch/scroller",   "Twitch Scroller",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_text",       "twitch/static-text","Twitch Static Text",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_viewers",    "twitch/viewer-count", "Twitch Viewer Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_followers",  "twitch/follower-count", "Twitch Follower Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_subscribers","twitch/subscriber-count", "Twitch Subscriber Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_tts",        "twitch/tts",        "Twitch TTS Narrator",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_twitch_battle_royale", "twitch/battle-royale", "Twitch Battle Royale",
	 true, "Battle Royale ID", 800, 800},

	/* ================= Blaze ================= */
	{"helixtools_overlay_blaze_alerts",     "blaze/alerts",     "Blaze Alerts",
	 false, NULL, 800, 600},
	{"helixtools_overlay_blaze_chat",       "blaze/chat",       "Blaze Chat Overlay",
	 false, NULL, 500, 1000},
	{"helixtools_overlay_blaze_goal",       "blaze/goal",       "Blaze Goal",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_emoji",      "blaze/emoji",      "Blaze Emoji Rain",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_scroller",   "blaze/scroller",   "Blaze Scroller",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_text",       "blaze/static-text","Blaze Static Text",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_viewers",    "blaze/viewer-count", "Blaze Viewer Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_followers",  "blaze/follower-count", "Blaze Follower Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_subscribers","blaze/subscriber-count", "Blaze Subscriber Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_tts",        "blaze/tts",        "Blaze TTS Narrator",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_blaze_leaderboard","blaze/leaderboard","Blaze Leaderboard",
	 true, "Leaderboard Key", 600, 1080},
	{"helixtools_overlay_blaze_battle_royale", "blaze/battle-royale", "Blaze Battle Royale",
	 true, "Battle Royale ID", 800, 800},

	/* ================= VPZone ================= */
	{"helixtools_overlay_vpzone_alerts",     "vpzone/alerts",     "VPZone Alerts",
	 false, NULL, 800, 600},
	{"helixtools_overlay_vpzone_chat",       "vpzone/chat",       "VPZone Chat Overlay",
	 false, NULL, 500, 1000},
	{"helixtools_overlay_vpzone_goal",       "vpzone/goal",       "VPZone Goal",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_emoji",      "vpzone/emoji",      "VPZone Emoji Rain",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_scroller",   "vpzone/scroller",   "VPZone Scroller",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_text",       "vpzone/static-text","VPZone Static Text",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_viewers",    "vpzone/viewer-count", "VPZone Viewer Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_followers",  "vpzone/follower-count", "VPZone Follower Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_subscribers","vpzone/subscriber-count", "VPZone Subscriber Count",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_tts",        "vpzone/tts",        "VPZone TTS Narrator",
	 false, NULL, 1920, 1080},
	{"helixtools_overlay_vpzone_battle_royale", "vpzone/battle-royale", "VPZone Battle Royale",
	 true, "Battle Royale ID", 800, 800},
};

const size_t NUM_OVERLAY_DEFS =
	sizeof(overlay_defs) / sizeof(overlay_defs[0]);

/* ------------------------------------------------------------------ */
/* Source instance data                                                 */
/* ------------------------------------------------------------------ */

struct overlay_source {
	obs_source_t *source;
	obs_source_t *browser;
	const struct helixtools_overlay_def *def;
	char *user_id;
	char *extra_id;
	char *custom_url; /* when set, overrides auto-generated URL */
	bool dark_background; /* multichat only: appends ?bg=black */
	uint32_t width;
	uint32_t height;

	/* Browser settings (forwarded to internal browser_source) */
	char *css;
	int fps;
	bool fps_custom;
	bool shutdown;            /* shutdown source when not visible */
	bool restart_when_active; /* refresh browser when scene becomes active */
	bool reroute_audio;       /* control audio via OBS */
	int webpage_control_level;
};

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static char *read_credential(const wchar_t *target)
{
#ifdef _WIN32
	CREDENTIALW *cred = NULL;
	if (!CredReadW(target, CRED_TYPE_GENERIC, 0, &cred))
		return NULL;

	char *val = bzalloc(cred->CredentialBlobSize + 1);
	memcpy(val, cred->CredentialBlob, cred->CredentialBlobSize);
	val[cred->CredentialBlobSize] = '\0';
	CredFree(cred);
	return val;
#else
	UNUSED_PARAMETER(target);
	return NULL;
#endif
}

/*
 * Plugin-level credential cache. The Hub user ID is written to Windows
 * Credential Manager by the HelixTools login helper (runs once on first
 * use of any HelixTools plugin) and read from here.
 *
 * Overlay URLs only need the user ID — most overlay types are public
 * once you know the streamer's ID. Types that need an auth token
 * (alerts, TTS, Would You Rather, Hot Takes, Raffle) support pasting
 * a full URL (including token) from the Hub dashboard's Widget Settings
 * into the "Overlay URL" field below instead.
 */
static struct {
	char *user_id;
	bool valid;
} cached_creds = {0};

void overlay_free_cred_cache(void)
{
	bfree(cached_creds.user_id);
	cached_creds.user_id = NULL;
	cached_creds.valid = false;
}

static void refresh_cred_cache(void)
{
	bfree(cached_creds.user_id);
	cached_creds.user_id = read_credential(HUB_USERID_CREDENTIAL);
	cached_creds.valid = true;
}

static char *read_hub_user_id(void)
{
	if (!cached_creds.valid)
		refresh_cred_cache();
	return cached_creds.user_id ? bstrdup(cached_creds.user_id) : NULL;
}

/* ------------------------------------------------------------------ */
/* URL construction                                                    */
/* ------------------------------------------------------------------ */

static void overlay_build_url(struct overlay_source *os, struct dstr *out)
{
	/* Custom URL takes priority - allows pasting full Hub URLs with a
	 * share token or query parameters (e.g. Scroller with text/speed/color). */
	if (os->custom_url && *os->custom_url) {
		dstr_copy(out, os->custom_url);
		return;
	}

	if (!os->def || !os->def->url_path[0]) {
		dstr_copy(out, "about:blank");
		return;
	}

	dstr_copy(out, HUB_OVERLAY_BASE);
	dstr_cat(out, os->def->url_path);

	if (os->user_id && *os->user_id) {
		dstr_cat(out, "/");
		dstr_cat(out, os->user_id);
	}

	if (os->def->needs_extra_id && os->extra_id && *os->extra_id) {
		dstr_cat(out, "/");
		dstr_cat(out, os->extra_id);
	}

	if (os->dark_background &&
	    strcmp(os->def->url_path, "multichat") == 0)
		dstr_cat(out, "?bg=black");
}

/* ------------------------------------------------------------------ */
/* Audio capture                                                       */
/* ------------------------------------------------------------------ */

/*
 * The inner browser is a private source (obs_source_create_private),
 * so its own audio never reaches the mixer/stream on its own —
 * enum_active_sources only propagates show/active state (video),
 * not audio. We have to explicitly tap its audio and republish it
 * through the outer (public, in-scene) source, same pattern as
 * helixtools-music's audio-only proxy source.
 */
static void overlay_audio_capture_cb(void *param, obs_source_t *source,
				     const struct audio_data *audio_data,
				     bool muted)
{
	UNUSED_PARAMETER(source);

	struct overlay_source *os = param;
	if (muted || !audio_data || !audio_data->frames)
		return;

	struct obs_audio_info oai;
	if (!obs_get_audio_info(&oai))
		return;

	struct obs_source_audio out = {0};
	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		out.data[i] = audio_data->data[i];

	out.frames = audio_data->frames;
	out.timestamp = audio_data->timestamp;
	out.samples_per_sec = oai.samples_per_sec;
	out.speakers = oai.speakers;
	out.format = AUDIO_FORMAT_FLOAT_PLANAR;

	obs_source_output_audio(os->source, &out);
}

static void overlay_update_browser(struct overlay_source *os)
{
	if (!os->browser)
		return;

	struct dstr url = {0};
	overlay_build_url(os, &url);

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "url", url.array);
	obs_data_set_int(settings, "width", os->width);
	obs_data_set_int(settings, "height", os->height);
	obs_data_set_bool(settings, "reroute_audio", os->reroute_audio);
	obs_data_set_string(settings, "css", os->css ? os->css : "");
	obs_data_set_int(settings, "fps", os->fps > 0 ? os->fps : 30);
	obs_data_set_bool(settings, "fps_custom", os->fps_custom);
	obs_data_set_bool(settings, "shutdown", os->shutdown);
	obs_data_set_bool(settings, "restart_when_active",
			  os->restart_when_active);
	obs_data_set_int(settings, "webpage_control_level",
			 os->webpage_control_level);
	obs_source_update(os->browser, settings);
	obs_data_release(settings);
	dstr_free(&url);
}

/* ------------------------------------------------------------------ */
/* Callbacks                                                           */
/* ------------------------------------------------------------------ */

const char *overlay_get_name(void *type_data)
{
	const struct helixtools_overlay_def *def = type_data;
	return def->display_name;
}

void *overlay_create(obs_data_t *settings, obs_source_t *source)
{
	/* type_data was set during registration */
	const char *src_id = obs_source_get_id(source);
	const struct helixtools_overlay_def *def = NULL;

	for (size_t i = 0; i < NUM_OVERLAY_DEFS; i++) {
		if (strcmp(overlay_defs[i].source_id, src_id) == 0) {
			def = &overlay_defs[i];
			break;
		}
	}

	struct overlay_source *os = bzalloc(sizeof(struct overlay_source));
	os->source = source;
	os->def = def;

	/* Credentials */
	os->user_id = read_hub_user_id();
	os->extra_id =
		bstrdup(obs_data_get_string(settings, "extra_id"));
	os->custom_url =
		bstrdup(obs_data_get_string(settings, "custom_url"));
	os->dark_background =
		obs_data_get_bool(settings, "dark_background");

	/* Dimensions */
	os->width = (uint32_t)obs_data_get_int(settings, "width");
	os->height = (uint32_t)obs_data_get_int(settings, "height");

	if (os->width == 0 && def)
		os->width = def->default_width;
	if (os->height == 0 && def)
		os->height = def->default_height;

	/* Browser settings */
	os->css = bstrdup(obs_data_get_string(settings, "css"));
	os->fps = (int)obs_data_get_int(settings, "fps");
	os->fps_custom = obs_data_get_bool(settings, "fps_custom");
	os->shutdown = obs_data_get_bool(settings, "shutdown");
	os->restart_when_active =
		obs_data_get_bool(settings, "restart_when_active");
	os->reroute_audio = obs_data_get_bool(settings, "reroute_audio");
	os->webpage_control_level =
		(int)obs_data_get_int(settings, "webpage_control_level");

	if (os->fps <= 0)
		os->fps = 30;

	/* Create internal browser source */
	obs_data_t *browser_settings = obs_data_create();
	obs_data_set_int(browser_settings, "width", os->width);
	obs_data_set_int(browser_settings, "height", os->height);
	obs_data_set_string(browser_settings, "url", "");
	obs_data_set_string(browser_settings, "css",
			    os->css ? os->css : "");
	obs_data_set_int(browser_settings, "fps", os->fps);
	obs_data_set_bool(browser_settings, "fps_custom", os->fps_custom);
	obs_data_set_bool(browser_settings, "shutdown", os->shutdown);
	obs_data_set_bool(browser_settings, "restart_when_active",
			  os->restart_when_active);
	obs_data_set_bool(browser_settings, "reroute_audio",
			  os->reroute_audio);
	obs_data_set_int(browser_settings, "webpage_control_level",
			 os->webpage_control_level);

	struct dstr name = {0};
	dstr_printf(&name, "helixtools_%s_browser",
		    def ? def->source_id : "unknown");
	os->browser = obs_source_create_private("browser_source", name.array,
						browser_settings);
	dstr_free(&name);
	obs_data_release(browser_settings);

	if (os->browser)
		obs_source_add_audio_capture_callback(
			os->browser, overlay_audio_capture_cb, os);

	overlay_update_browser(os);

	blog(LOG_INFO,
	     "[helixtools-overlays] Created source '%s' (%ux%u, user=%s)",
	     def ? def->display_name : src_id, os->width, os->height,
	     os->user_id ? "present" : "absent");

	return os;
}

void overlay_destroy(void *data)
{
	struct overlay_source *os = data;

	if (os->browser) {
		obs_source_remove_audio_capture_callback(
			os->browser, overlay_audio_capture_cb, os);
		obs_source_release(os->browser);
		os->browser = NULL;
	}

	bfree(os->user_id);
	bfree(os->extra_id);
	bfree(os->custom_url);
	bfree(os->css);
	bfree(os);
}

/* Compare and swap a bstrdup'd string field. Returns true if changed. */
static bool update_field(char **field, char *new_val)
{
	if (!*field && !new_val)
		return false;
	if (*field && new_val && strcmp(*field, new_val) == 0) {
		bfree(new_val);
		return false;
	}
	bfree(*field);
	*field = new_val;
	return true;
}

void overlay_update(void *data, obs_data_t *settings)
{
	struct overlay_source *os = data;
	bool changed = false;

	/* Invalidate credential cache so we re-read from Credential
	 * Manager once per batch of updates (not once per overlay). */
	cached_creds.valid = false;

	/* Re-read credentials only if they actually changed */
	changed |= update_field(&os->user_id, read_hub_user_id());

	/* Settings fields */
	char *new_extra = bstrdup(
		obs_data_get_string(settings, "extra_id"));
	changed |= update_field(&os->extra_id, new_extra);

	char *new_url = bstrdup(
		obs_data_get_string(settings, "custom_url"));
	changed |= update_field(&os->custom_url, new_url);

	bool dark_bg = obs_data_get_bool(settings, "dark_background");
	if (dark_bg != os->dark_background) {
		os->dark_background = dark_bg;
		changed = true;
	}

	/* Dimensions */
	uint32_t w = (uint32_t)obs_data_get_int(settings, "width");
	uint32_t h = (uint32_t)obs_data_get_int(settings, "height");

	if (w == 0 && os->def)
		w = os->def->default_width;
	if (h == 0 && os->def)
		h = os->def->default_height;

	if (w != os->width || h != os->height) {
		os->width = w;
		os->height = h;
		changed = true;
	}

	/* Browser settings */
	char *new_css = bstrdup(obs_data_get_string(settings, "css"));
	changed |= update_field(&os->css, new_css);

	int fps = (int)obs_data_get_int(settings, "fps");
	if (fps <= 0)
		fps = 30;
	if (fps != os->fps) {
		os->fps = fps;
		changed = true;
	}

	bool fps_custom = obs_data_get_bool(settings, "fps_custom");
	if (fps_custom != os->fps_custom) {
		os->fps_custom = fps_custom;
		changed = true;
	}

	bool shutdown = obs_data_get_bool(settings, "shutdown");
	if (shutdown != os->shutdown) {
		os->shutdown = shutdown;
		changed = true;
	}

	bool restart = obs_data_get_bool(settings, "restart_when_active");
	if (restart != os->restart_when_active) {
		os->restart_when_active = restart;
		changed = true;
	}

	bool reroute = obs_data_get_bool(settings, "reroute_audio");
	if (reroute != os->reroute_audio) {
		os->reroute_audio = reroute;
		changed = true;
	}

	int wcl = (int)obs_data_get_int(settings, "webpage_control_level");
	if (wcl != os->webpage_control_level) {
		os->webpage_control_level = wcl;
		changed = true;
	}

	if (changed)
		overlay_update_browser(os);
}

void overlay_defaults(void *type_data, obs_data_t *settings)
{
	const struct helixtools_overlay_def *def = type_data;
	if (!def)
		return;

	obs_data_set_default_int(settings, "width", def->default_width);
	obs_data_set_default_int(settings, "height", def->default_height);

	/* Browser defaults — overlays default to shutdown-when-hidden to
	 * reduce GPU load from idle CEF processes in non-active scenes. */
	obs_data_set_default_string(settings, "css", OVERLAY_DEFAULT_CSS);
	obs_data_set_default_int(settings, "fps", 30);
	obs_data_set_default_bool(settings, "fps_custom", false);
	obs_data_set_default_bool(settings, "shutdown", true);
	obs_data_set_default_bool(settings, "restart_when_active", false);
	obs_data_set_default_bool(settings, "reroute_audio", false);
	obs_data_set_default_int(settings, "webpage_control_level", 1);
}

/* ------------------------------------------------------------------ */
/* Property callbacks                                                  */
/* ------------------------------------------------------------------ */

static bool overlay_refresh_clicked(obs_properties_t *props,
				    obs_property_t *p, void *data)
{
	struct overlay_source *os = data;
	if (!os || !os->browser)
		return false;

	obs_properties_t *browser_props =
		obs_source_properties(os->browser);
	if (browser_props) {
		obs_property_t *refresh =
			obs_properties_get(browser_props, "refreshnocache");
		if (refresh)
			obs_property_button_clicked(refresh, os->browser);
		obs_properties_destroy(browser_props);
	}

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}

static bool overlay_fps_custom_modified(obs_properties_t *props,
					obs_property_t *p,
					obs_data_t *settings)
{
	bool enabled = obs_data_get_bool(settings, "fps_custom");
	obs_property_t *fps_prop = obs_properties_get(props, "fps");
	if (fps_prop)
		obs_property_set_visible(fps_prop, enabled);

	UNUSED_PARAMETER(p);
	return true;
}

obs_properties_t *overlay_properties(void *data, void *type_data)
{
	const struct helixtools_overlay_def *def = type_data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);

	/* Custom URL - paste full Hub overlay URL to override auto-generated.
	 * Required for overlays that need an auth token (Alerts, TTS,
	 * Would You Rather, Hot Takes, Raffle) or that encode config in
	 * query params (e.g. Scroller). Copy it from the widget's
	 * "Widget Settings" panel on your Hub dashboard. */
	obs_properties_add_text(props, "custom_url",
				"Overlay URL (paste from Widget Settings)",
				OBS_TEXT_DEFAULT);

	/* Extra ID field (only for game overlays that need it) */
	if (def && def->needs_extra_id) {
		obs_properties_add_text(props, "extra_id",
					def->extra_id_label,
					OBS_TEXT_DEFAULT);
	}

	/* Multichat-only: dark background variant */
	if (def && strcmp(def->url_path, "multichat") == 0) {
		obs_properties_add_bool(props, "dark_background",
					"Black background (vs. transparent)");
	}

	/* Dimensions */
	obs_properties_add_int(props, "width", "Width", 100, 3840, 1);
	obs_properties_add_int(props, "height", "Height", 100, 2160, 1);

	/* --- Browser settings (same as normal browser source) --- */

	obs_properties_add_bool(props, "reroute_audio",
				"Control audio via OBS");

	obs_property_t *fps_custom = obs_properties_add_bool(
		props, "fps_custom", "Use custom frame rate");
	obs_property_set_modified_callback(fps_custom,
					   overlay_fps_custom_modified);

	obs_property_t *fps_prop =
		obs_properties_add_int(props, "fps", "FPS", 1, 120, 1);

	/* Hide FPS spinner unless custom FPS is checked */
	if (data) {
		struct overlay_source *os = data;
		obs_property_set_visible(fps_prop, os->fps_custom);
	} else {
		obs_property_set_visible(fps_prop, false);
	}

	obs_properties_add_text(props, "css", "Custom CSS",
				OBS_TEXT_MULTILINE);

	obs_properties_add_bool(props, "shutdown",
				"Shutdown source when not visible");

	obs_properties_add_bool(props, "restart_when_active",
				"Refresh browser when scene becomes active");

	obs_property_t *page_perms = obs_properties_add_list(
		props, "webpage_control_level", "Page permissions",
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(page_perms, "No access to OBS", 0);
	obs_property_list_add_int(page_perms, "Read access to OBS status information", 1);
	obs_property_list_add_int(page_perms, "Read access to user information", 2);
	obs_property_list_add_int(page_perms, "Basic access to OBS", 3);
	obs_property_list_add_int(page_perms, "Advanced access to OBS", 4);
	obs_property_list_add_int(page_perms, "Full access to OBS", 5);

	obs_properties_add_button2(props, "refreshnocache",
				   "Refresh cache of current page",
				   overlay_refresh_clicked, data);

	/* Show active URL for reference */
	if (data) {
		struct overlay_source *os = data;
		struct dstr url = {0};
		overlay_build_url(os, &url);

		struct dstr info_text = {0};
		dstr_printf(&info_text,
			    "Active URL: %s\n"
			    "Leave 'Overlay URL' empty to use the "
			    "auto-generated URL from your Hub account.\n"
			    "If you haven't logged in via the HelixTools "
			    "login helper yet, run it once, or paste your "
			    "full overlay URL from the Hub dashboard above.",
			    url.array);

		obs_properties_add_text(props, "info", info_text.array,
					OBS_TEXT_INFO);
		dstr_free(&info_text);
		dstr_free(&url);
	}

	return props;
}

uint32_t overlay_get_width(void *data)
{
	struct overlay_source *os = data;
	return os->width;
}

uint32_t overlay_get_height(void *data)
{
	struct overlay_source *os = data;
	return os->height;
}

void overlay_enum_active_sources(void *data, obs_source_enum_proc_t cb,
				 void *param)
{
	struct overlay_source *os = data;
	if (os->browser)
		cb(os->source, os->browser, param);
}

void overlay_video_render(void *data, gs_effect_t *effect)
{
	struct overlay_source *os = data;
	if (os->browser)
		obs_source_video_render(os->browser);
	UNUSED_PARAMETER(effect);
}

void overlay_mouse_click(void *data, const struct obs_mouse_event *event,
			 int32_t type, bool mouse_up, uint32_t click_count)
{
	struct overlay_source *os = data;
	if (os->browser)
		obs_source_send_mouse_click(os->browser, event, type,
					    mouse_up, click_count);
}

void overlay_mouse_move(void *data, const struct obs_mouse_event *event,
			bool mouse_leave)
{
	struct overlay_source *os = data;
	if (os->browser)
		obs_source_send_mouse_move(os->browser, event, mouse_leave);
}

void overlay_mouse_wheel(void *data, const struct obs_mouse_event *event,
			 int x_delta, int y_delta)
{
	struct overlay_source *os = data;
	if (os->browser)
		obs_source_send_mouse_wheel(os->browser, event, x_delta,
					    y_delta);
}

void overlay_key_click(void *data, const struct obs_key_event *event,
		       bool key_up)
{
	struct overlay_source *os = data;
	if (os->browser)
		obs_source_send_key_click(os->browser, event, key_up);
}

void overlay_focus(void *data, bool focus)
{
	struct overlay_source *os = data;
	if (os->browser)
		obs_source_send_focus(os->browser, focus);
}
