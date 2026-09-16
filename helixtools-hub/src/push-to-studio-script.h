/*
 * push-to-studio-script.h - JS injected into the HelixTools Hub dock.
 *
 * Two jobs:
 *   1. Add "Push to Studio" buttons next to every overlay/game URL on
 *      the page.
 *   2. Watch for a logged-in Supabase session and hand the user ID +
 *      tokens back to native code, so other HelixTools plugins can
 *      read them from Credential Manager without their own login UI
 *      (see the "Auth session capture" block near the bottom).
 *
 * Ported from an integrated studio app's equivalent script (in
 * obs-studio/frontend/widgets/OBSBasic_Browser.cpp), adjusted for the
 * current Hub site:
 *   - Domain: helixtools.org/overlay
 *   - Path gate: /hub (was /dashboard) — matches this site's router
 *   - Card detection: this site's Card component renders
 *     "rounded-lg bg-primary/10 ..." with no .border/.bg-card classes,
 *     so cards are matched by ".rounded-lg" + containing an <h3>,
 *     rather than the fork's ".rounded-lg.border.bg-card" selector.
 *   - Icon-only copy buttons (fork's "Pattern C") are not handled —
 *     every overlay/game page checked so far uses a labeled "Copy ..."
 *     button or a readonly <input>, so Patterns A+B cover them.
 *
 * Two page patterns:
 *   A) Overlay config pages (Chat, Alerts, Goal, etc.) — URL sits in a
 *      readonly <input> element.
 *   B) Game preset pages (Coin Flip, Roll Dice, etc.) — URL is NOT in
 *      the DOM; it's generated on click by a "Copy ..." button via
 *      navigator.clipboard.writeText().
 *
 * The captured URL and preset name are sent to C++ by briefly setting
 * document.title = "HELIXTOOLS_PUSH:" + JSON.stringify({url, name}).
 * The C++ side listens for QCefWidget::titleChanged.
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

#include "hub-backend-config.h"

static const char *HELIXTOOLS_PUSH_TO_STUDIO_SCRIPT =
	"(function(){"
	"  if(window.__helixtools_push_init) return;"
	"  window.__helixtools_push_init = true;"
	""
	/* --- Clipboard intercept for game pages --- */
	"  var origWrite = navigator.clipboard.writeText.bind("
	"    navigator.clipboard);"
	"  var captureResolve = null;"
	"  navigator.clipboard.writeText = function(text){"
	"    if(captureResolve && text &&"
	"       text.indexOf('helixtools.org/overlay') >= 0){"
	"      captureResolve(text);"
	"      captureResolve = null;"
	"    }"
	"    return origWrite(text);"
	"  };"
	""
	/* --- Button styling --- */
	"  var btnCSS = '"
	"    display:inline-flex; align-items:center; justify-content:center;"
	"    gap:6px; padding:6px 12px; border-radius:6px; border:none;"
	"    background:#7C3AED; color:white; font-size:13px;"
	"    font-weight:600; cursor:pointer; white-space:nowrap;"
	"    transition:background 0.15s;"
	"  ';"
	""
	/* --- Extract card/preset name from nearest heading --- */
	"  function getCardName(el){"
	"    var card = el.closest('.rounded-lg') || el.parentElement;"
	"    if(!card) return '';"
	"    var h = card.querySelector('h3');"
	"    if(!h) return '';"
	"    var span = h.querySelector('span.truncate');"
	"    if(span) return span.textContent.trim();"
	"    var clone = h.cloneNode(true);"
	"    var btns = clone.querySelectorAll('div,button');"
	"    btns.forEach(function(b){ b.remove(); });"
	"    return clone.textContent.trim();"
	"  }"
	""
	/* --- Signal to C++ via document.title --- */
	"  function signalPush(url, name){"
	"    var orig = document.title;"
	"    document.title = 'HELIXTOOLS_PUSH:' +"
	"      JSON.stringify({url:url, name:name});"
	"    setTimeout(function(){"
	"      document.title = orig;"
	"    }, 200);"
	"  }"
	""
	/* --- Create a Push to Studio button --- */
	"  var pushSVG = '"
	"    <svg xmlns=\"http://www.w3.org/2000/svg\" width=\"14\""
	"     height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\""
	"     stroke=\"currentColor\" stroke-width=\"2\""
	"     stroke-linecap=\"round\" stroke-linejoin=\"round\">"
	"     <path d=\"M5 12h14\"/><path d=\"m12 5 7 7-7 7\"/>"
	"    </svg>"
	"    Push to Studio';"
	""
	"  function makePushBtn(){"
	"    var btn = document.createElement('button');"
	"    btn.setAttribute('data-helixtools-push','1');"
	"    btn.setAttribute('type','button');"
	"    btn.style.cssText = btnCSS;"
	"    btn.innerHTML = pushSVG;"
	"    btn.onmouseenter = function(){"
	"      btn.style.background='#6D28D9';};"
	"    btn.onmouseleave = function(){"
	"      btn.style.background='#7C3AED';};"
	"    return btn;"
	"  }"
	""
	"  function resetBtn(b){"
	"    setTimeout(function(){"
	"      b.style.background='#7C3AED';"
	"      b.innerHTML=pushSVG;"
	"    }, 1500);"
	"  }"
	""
	"  function clipPush(trigger, name, btn){"
	"    new Promise(function(resolve){"
	"      captureResolve = resolve;"
	"      trigger.click();"
	"      setTimeout(function(){"
	"        if(captureResolve){"
	"          captureResolve = null; resolve('');"
	"        }"
	"      }, 500);"
	"    }).then(function(url){"
	"      if(!url){"
	"        btn.style.background='#EF4444';"
	"        btn.innerHTML='No URL found';"
	"      } else {"
	"        signalPush(url, name);"
	"        btn.style.background='#22C55E';"
	"        btn.innerHTML='Added!';"
	"      }"
	"      resetBtn(btn);"
	"    });"
	"  }"
	""
	/* --- Inject buttons into the page --- */
	"  function injectButtons(){"
	"    var path = window.location.pathname;"
	"    if(path.indexOf('/hub/') < 0 && path !== '/hub') return;"
	""
	/* Pattern A: inputs with overlay URLs (overlay config pages) */
	"    document.querySelectorAll('input').forEach(function(inp){"
	"      var val = inp.value || '';"
	"      if(val.indexOf('helixtools.org/overlay') < 0) return;"
	"      var wrapper = inp.parentElement;"
	"      if(!wrapper) return;"
	"      if(wrapper.querySelector('[data-helixtools-push]')) return;"
	"      var btn = makePushBtn();"
	"      btn.style.flexShrink = '0';"
	"      btn.onclick = function(e){"
	"        e.preventDefault(); e.stopPropagation();"
	"        signalPush(inp.value, getCardName(inp));"
	"        btn.style.background='#22C55E';"
	"        btn.innerHTML='Added!';"
	"        resetBtn(btn);"
	"      };"
	"      wrapper.appendChild(btn);"
	"    });"
	""
	/* Pattern B: buttons with "Copy" in their text (games/leaderboards) */
	"    document.querySelectorAll('button').forEach(function(cb){"
	"      if(cb.getAttribute('data-helixtools-push')) return;"
	"      if(cb.getAttribute('data-helixtools-processed')) return;"
	"      var t = cb.textContent.trim();"
	"      if(t.indexOf('Push') >= 0) return;"
	"      if(t.indexOf('Copy') < 0) return;"
	"      cb.setAttribute('data-helixtools-processed','1');"
	"      var name = getCardName(cb);"
	"      var btn = makePushBtn();"
	"      btn.onclick = function(e){"
	"        e.preventDefault(); e.stopPropagation();"
	"        clipPush(cb, name, btn);"
	"      };"
	"      btn.style.width = '100%';"
	"      btn.style.marginTop = '4px';"
	"      var p = cb.parentElement;"
	"      if(p) p.insertBefore(btn, cb.nextSibling);"
	"    });"
	""
	/* Pattern C: icon-only copy buttons (no text, just a Copy icon —
	 * e.g. Loot Box, Battle Royale preset cards). lucide-react renders
	 * its Copy icon as an <svg class="lucide lucide-copy ..."> inside
	 * the button, which is a much steadier marker than text content. */
	"    document.querySelectorAll('button').forEach(function(cb){"
	"      if(cb.getAttribute('data-helixtools-push')) return;"
	"      if(cb.getAttribute('data-helixtools-processed')) return;"
	"      var t = cb.textContent.trim();"
	"      if(t !== '') return;"
	"      if(!cb.querySelector('svg.lucide-copy')) return;"
	"      cb.setAttribute('data-helixtools-processed','1');"
	"      var name = getCardName(cb);"
	"      var btn = makePushBtn();"
	"      btn.onclick = function(e){"
	"        e.preventDefault(); e.stopPropagation();"
	"        clipPush(cb, name, btn);"
	"      };"
	"      var p = cb.parentElement;"
	"      if(p) p.insertBefore(btn, cb.nextSibling);"
	"    });"
	"  }"
	""
	"  var obs = new MutationObserver(function(){"
	"    try { injectButtons(); } catch(e) {}"
	"  });"
	"  obs.observe(document.documentElement,"
	"    { childList: true, subtree: true });"
	"  injectButtons();"
	/* Several overlay pages render their URL <input> empty at first
	 * (waiting on an async user-ID fetch) and fill it in later via a
	 * React value-only update, which adds/removes no DOM nodes — so
	 * the MutationObserver above never re-fires for it. Poll too, so
	 * the button still gets attached once the real URL shows up. */
	"  setInterval(function(){"
	"    try { injectButtons(); } catch(e) {}"
	"  }, 1000);"
	""
	/* --- Auth session capture -------------------------------------
	 * Watches Supabase's own localStorage session key for this
	 * project and signals the logged-in user's ID + tokens to C++
	 * the same way Push to Studio does (via document.title), so the
	 * native side can cache them in Windows Credential Manager for
	 * other HelixTools plugins (overlays, clip) to read. Polls on an
	 * interval rather than depending on a storage event, since the
	 * session can appear via a same-tab redirect that doesn't always
	 * fire one. */
	"  var __lastAuthSig = '';"
	"  function checkAuthSession(){"
	"    try {"
	"      var raw = localStorage.getItem("
	"        'sb-" HUB_SUPABASE_PROJECT_ID "-auth-token');"
	"      if(!raw) return;"
	"      var sess = JSON.parse(raw);"
	"      var uid = sess && sess.user && sess.user.id;"
	"      var at = sess && sess.access_token;"
	"      var rt = sess && sess.refresh_token;"
	"      if(!uid || !at) return;"
	"      var sig = uid + '|' + at;"
	"      if(sig === __lastAuthSig) return;"
	"      __lastAuthSig = sig;"
	"      var orig = document.title;"
	"      document.title = 'HELIXTOOLS_AUTH:' + JSON.stringify("
	"        {userId:uid, accessToken:at, refreshToken:rt || ''});"
	"      setTimeout(function(){ document.title = orig; }, 200);"
	"    } catch(e) {}"
	"  }"
	"  setInterval(checkAuthSession, 2000);"
	"  checkAuthSession();"
	"})();";
