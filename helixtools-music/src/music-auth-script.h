/*
 * music-auth-script.h - JS injected into the HelixTools Music dock
 *
 * Watches Supabase's own localStorage session key for the Music
 * project and hands the logged-in user's ID (only the ID, nothing
 * else) back to native code, so MusicManager can poll overlay_data
 * for this account.
 *
 * Deliberately narrower than helixtools-hub's equivalent
 * (push-to-studio-script.h's "Auth session capture" block): that one
 * also captures access/refresh tokens, because helixtools-clip needs
 * an authenticated bearer token to query a table gated by RLS to the
 * owning user. overlay_data has a public SELECT policy instead (the
 * same one the web overlay itself relies on for unauthenticated OBS
 * browser sources), so a poller only needs the bare user ID to scope
 * its query — capturing a real access/refresh token here would be
 * exposing a credential this feature has no use for. This repo is
 * open source, so the smaller the secret surface shipped in cleartext
 * JS, the better.
 *
 * Same document.title signaling technique as Hub (see that file's
 * header comment for why): QCefWidget's public API has no direct
 * JS-to-native call, so the captured value is briefly written to
 * document.title and read back via QCefWidget::titleChanged.
 *
 * The localStorage key is NOT Supabase's default "sb-<ref>-auth-token"
 * (that's what Hub's equivalent script reads, and it's only the
 * default when a client is created with no custom storage options).
 * The Music web app's client (src/integrations/supabase-music/client.ts)
 * passes a custom storage wrapper that prefixes every key with
 * "music-prefix:" and sets storageKey to "music-auth-token", so the
 * real key is "music-prefix:music-auth-token" — fixed 2026-09-22 after
 * the original sb-<ref>-auth-token guess never found a session.
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

static const char *HELIXTOOLS_MUSIC_AUTH_SCRIPT =
	"(function(){"
	"  if(window.__helixtools_music_auth_init) return;"
	"  window.__helixtools_music_auth_init = true;"
	""
	"  var __lastUid = '';"
	"  function checkAuthSession(){"
	"    try {"
	"      var raw = localStorage.getItem("
	"        'music-prefix:music-auth-token');"
	"      if(!raw) return;"
	"      var sess = JSON.parse(raw);"
	"      var uid = sess && sess.user && sess.user.id;"
	"      if(!uid || uid === __lastUid) return;"
	"      __lastUid = uid;"
	"      var orig = document.title;"
	"      document.title = 'HELIXTOOLS_MUSIC_AUTH:' +"
	"        JSON.stringify({userId:uid});"
	"      setTimeout(function(){ document.title = orig; }, 200);"
	"    } catch(e) {}"
	"  }"
	"  setInterval(checkAuthSession, 2000);"
	"  checkAuthSession();"
	"})();";
