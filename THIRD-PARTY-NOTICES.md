# Third-Party Notices

Helix ToolKit is licensed under the GNU General Public License, version 2
or (at your option) any later version. See [LICENSE](LICENSE).

Helix ToolKit is an independent project. It is **not affiliated with or
endorsed by the OBS Project**. "OBS" and "OBS Studio" are names of the
OBS Project's software; they are used here only to describe what these
plugins work with.

This file lists the third-party code and files that are included in this
repository or shipped in the Helix ToolKit installer.

## OBS Studio (obs-browser panel interface)

- **Files:** `helixtools-*/src/vendor/browser-panel.hpp`
- **From:** <https://github.com/obsproject/obs-studio>
  (`plugins/obs-browser/panel/browser-panel.hpp`)
- **License:** GPL-2.0-or-later
- **Notes:** Copied verbatim, with only a provenance comment added at the
  top. This is the interface obs-browser exposes for embedding browser
  panels in docks.

## obs-websocket (vendor API header)

- **File:** `helixtools-macros/src/vendor/obs-websocket-api.h`
- **From:** <https://github.com/obsproject/obs-websocket>
- **Copyright:** (C) 2016-2021 Stephane Lepin, (C) 2020-2022 Kyle Manning
- **License:** GPL-2.0-or-later (full header kept in the file)

## Qt 6 TLS backend plugin (shipped in the installer)

- **File installed:** `bin\64bit\tls\qschannelbackend.dll` (installed
  only with the Clip component; not stored in this repository)
- **From:** the unmodified Qt 6.11.1 build bundled with OBS Studio's
  dependencies, `obs-deps` release `2026-07-15`:
  <https://github.com/obsproject/obs-deps/releases/tag/2026-07-15>
- **Qt source code:** <https://code.qt.io/cgit/qt/qtbase.git/> (tag
  `v6.11.1`), and <https://download.qt.io/official_releases/qt/>
- **License:** GNU Lesser General Public License v3 (LGPL-3.0). The full
  text of the LGPL-3.0 and the GPL-3.0 it refers to are in
  [licenses/](licenses/) and are installed next to the plugins in
  `HelixTools-Licenses\`.
- **Notes:** The file is a separate, unmodified runtime plugin that Qt
  loads at start-up. You can replace it with your own build of the same
  Qt component. How it was obtained is documented in
  [helixtools-clip/vendor/NOTE.md](helixtools-clip/vendor/NOTE.md).

## Libraries loaded from your OBS Studio installation

The plugins run inside OBS Studio and use libraries that OBS already
ships (libobs, obs-frontend-api, Qt 6, and the browser panel). Those are
not redistributed by Helix ToolKit and remain under their own licenses.

## Platform names and logos

The Streaming Dashboards plugin shows the names and logos of Rumble,
Twitch, YouTube, Kick, Blaze and VPZone to identify each platform's
dashboard. These are trademarks of their respective owners. Helix
ToolKit is not affiliated with or endorsed by any of them. The logo image
files are not stored in this repository.
