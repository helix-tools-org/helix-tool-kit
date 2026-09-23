# Platform logos (not bundled)

This repo doesn't ship the platform logo images themselves — each is
that platform's own trademark (Twitch, YouTube, Kick, Blaze, Rumble,
VPZone), and this project is open source, so it doesn't redistribute
them.

The dashboard sidebar (`helixtools-dashboard`) works fine without any
of these — a missing file just means that sidebar entry shows no icon,
nothing breaks (`obs_find_module_file` returns `NULL` for a file that
doesn't exist, and `dashboard-dock.cpp` already checks for that before
calling `QIcon`).

If you want icons, drop your own PNGs into this folder using these
exact filenames (matching `dashboard-sites.h`'s `iconFile` entries),
rendered at 24x24 in the sidebar so a square image around 64-128px
scales down cleanly:

- `rumble.png`
- `twitch.png`
- `youtube.png`
- `kick.png`
- `blaze.png`
- `vpzone.png`
