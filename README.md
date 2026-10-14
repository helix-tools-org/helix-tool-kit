# HelixTools Plugin Suite

A collection of OBS Studio plugins built for Rumble streamers, distributed
as a standalone installer for a vanilla (unmodified) OBS Studio install.

## Plugins

| Plugin | Description |
|---|---|
| `helixtools-service` | Rumble RTMP service (static key support) |
| `helixtools-overlays` | Hub-hosted browser-source overlays (chat, alerts, goals, minigames) |
| `helixtools-music` | Background music dock + isolated mixer channel |
| `helixtools-clip` | Hotkey-driven clip creation |
| `helixtools-macros` | Hotkey action-sequence macro engine |
| `helixtools-hub` | HelixTools Hub dock (Push to Studio) |
| `helixtools-social` | Social media browser dock |
| `helixtools-dashboard` | Streaming dashboards dock (Rumble Studio, Twitch, YouTube, Kick, Blaze, VPZone) |
| `helixtools-rumble-studio` | Standalone Rumble Studio browser dock (superseded by `helixtools-dashboard`; not packaged in the installer) |
| `helixtools-restream` | Multi-RTMP restreaming output (not currently packaged in the installer — no destinations UI yet) |

## Building

Each plugin is an independent out-of-tree CMake project targeting a
standard OBS Studio SDK, the same way any third-party OBS plugin is built.

Prerequisites:
- Visual Studio 2022 (Desktop C++ workload)
- CMake 3.28+
- A local build of [obsproject/obs-studio](https://github.com/obsproject/obs-studio)
  (needed for the exported `libobs` / `obs-frontend-api` CMake packages),
  or an OBS SDK layout that provides them
- Qt 6, matching the Qt6 build OBS itself was built against
  (`helixtools-clip`, `helixtools-hub`, `helixtools-macros`,
  `helixtools-music`, `helixtools-social`, `helixtools-rumble-studio` link Qt6)

Set `OBS_ROOT` to the root of your obs-studio checkout/build, then:

```
set OBS_ROOT=C:\path\to\obs-studio
build-all.bat helixtools-hub
```

This configures and builds the named plugin into `<plugin-dir>\build`,
installing to `<plugin-dir>\build\rundir\obs-plugins\64bit\`.

## Installer

`installer\helixtools-plugin-suite.iss` (built with
[Inno Setup 6](https://jrsoftware.org/isinfo.php)) packages the built
plugins into a single installer that targets a user's existing OBS
Studio install. Build every plugin first, then compile the script.

## License

GPLv2-or-later. See `LICENSE`. These plugins communicate with OBS
Studio (libobs) through its public plugin API and are independent works,
not derived from OBS Studio itself.
