/*
 * social-link-menu-script.h - In-page "Copy Link Address" context menu
 *
 * Vanilla OBS's browser dock context menu only offers "Copy URL", which
 * copies the address of the page currently loaded, never the link under
 * the cursor. Plugins can't hook CEF's native context menu, so this
 * script handles right-clicks on links itself: when the click lands on
 * (or inside) an <a href>, it shows a small in-page menu with
 * "Copy Link Address" / "Open Link". Right-clicks anywhere else fall
 * through to the normal native menu untouched.
 *
 * Copyright (C) 2026 HelixTools
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

static const char *HELIXTOOLS_LINK_MENU_SCRIPT =
	"(function(){"
	"  if (window.__helixLinkMenu) return;"
	"  window.__helixLinkMenu = true;"
	"  var menu = null;"
	"  function closeMenu(){"
	"    if (menu && menu.parentNode) menu.parentNode.removeChild(menu);"
	"    menu = null;"
	"  }"
	"  function copyText(text){"
	"    function fallback(){"
	"      var ta = document.createElement('textarea');"
	"      ta.value = text;"
	"      ta.style.cssText = 'position:fixed;left:-9999px;top:0;opacity:0';"
	"      document.body.appendChild(ta);"
	"      ta.focus(); ta.select();"
	"      try { document.execCommand('copy'); } catch(e) {}"
	"      document.body.removeChild(ta);"
	"    }"
	"    if (navigator.clipboard && navigator.clipboard.writeText) {"
	"      navigator.clipboard.writeText(text).catch(fallback);"
	"    } else {"
	"      fallback();"
	"    }"
	"  }"
	"  function addItem(label, onClick){"
	"    var it = document.createElement('div');"
	"    it.textContent = label;"
	"    it.style.cssText = 'padding:7px 18px;cursor:pointer;white-space:nowrap;';"
	"    it.onmouseenter = function(){ it.style.background = '#7C3AED'; };"
	"    it.onmouseleave = function(){ it.style.background = 'transparent'; };"
	"    it.onmousedown = function(e){ e.preventDefault(); e.stopPropagation(); };"
	"    it.onclick = function(e){ e.preventDefault(); e.stopPropagation(); onClick(); closeMenu(); };"
	"    menu.appendChild(it);"
	"  }"
	"  document.addEventListener('contextmenu', function(e){"
	"    closeMenu();"
	"    var a = e.target && e.target.closest ? e.target.closest('a[href]') : null;"
	"    if (!a || !a.href) return;"
	"    var href = a.href;"
	"    if (/^javascript:/i.test(href)) return;"
	"    e.preventDefault();"
	"    e.stopPropagation();"
	"    menu = document.createElement('div');"
	"    menu.style.cssText = 'position:fixed;z-index:2147483647;background:#1A1E24;"
	"color:#eee;border:1px solid #3A3F47;border-radius:6px;padding:4px 0;"
	"font:13px Segoe UI,Arial,sans-serif;box-shadow:0 4px 14px rgba(0,0,0,.5);';"
	"    addItem('Copy Link Address', function(){ copyText(href); });"
	"    addItem('Open Link', function(){ window.location.href = href; });"
	"    document.body.appendChild(menu);"
	"    var x = Math.min(e.clientX, window.innerWidth - menu.offsetWidth - 4);"
	"    var y = Math.min(e.clientY, window.innerHeight - menu.offsetHeight - 4);"
	"    menu.style.left = Math.max(0, x) + 'px';"
	"    menu.style.top = Math.max(0, y) + 'px';"
	"  }, true);"
	"  document.addEventListener('mousedown', closeMenu, true);"
	"  document.addEventListener('scroll', closeMenu, true);"
	"  window.addEventListener('blur', closeMenu);"
	"  document.addEventListener('keydown', function(e){"
	"    if (e.key === 'Escape') closeMenu();"
	"  }, true);"
	"})();";
