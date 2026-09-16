# Status: shipped in the plugin suite installer (2026-09-16)

`helixtools-restream` now has a working destinations editor: a real OBS
dock, "Restream Destinations" (`restream-dock-init.cpp` registers it via
`obs_frontend_add_dock_by_id`, matching how `helixtools-hub`/`-social`/
`-music` register their own docks). It shows up under View -> Docks,
can be dragged into the layout or left floating, and OBS persists its
shown/hidden state and position across restarts on its own — the
plugin doesn't need to do anything extra for that.

The dock (`restream-dock.cpp`) lets the user add/edit/remove
destinations (name, server URL, stream key, enabled). Every mutation
immediately calls `helixtools_restream_save()` (`restream-storage.c`),
which persists to `destinations.json` under this plugin's
`obs_module_config_path()` directory
(`%APPDATA%/obs-studio/plugin_config/helixtools-restream/destinations.json`
on Windows). `obs_module_load()` calls `helixtools_restream_load()` to
restore them on startup.

The start/stop/output logic in `restream-output.c` is unchanged — it
was already correct, and already wired to
`OBS_FRONTEND_EVENT_STREAMING_STARTED`/`STOPPING`. It shares the main
stream's video/audio encoders by default (zero extra CPU cost).

The editor disables Add/Edit/Remove while a stream is active (it only
edits the plain destination metadata, not the live `obs_output_t`/
`obs_service_t` objects `restream-output.c` owns while streaming), so
there's no live-mutation hazard to handle yet — that's the next thing
to revisit if per-destination start/stop while already live is wanted.

It's back in `installer/helixtools-plugin-suite.iss`'s `RestreamRoot`
define, `[Components]`, and `[Files]` sections, and in
`install-all.bat`'s plugin list.
