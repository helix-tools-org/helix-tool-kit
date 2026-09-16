# History: shared-browser design dropped for a two-browser fallback

`helixtools-music` shipped 2026-09-15 using the safer fallback design:
a plain `QCefWidget` dock (`music-dock.cpp`, same proven pattern as
`helixtools-hub`/`helixtools-social`) for browsing/login/playback
control, plus a separate private `browser_source` pinned to Mixer
channel 7 (`helixtools-music.cpp`) for stream audio. Two independent
browser instances, not one shared one — so dock playback state and
stream audio state are **not** guaranteed to match (if you press play
in the dock, the audio-only instance underneath doesn't know about it
unless it's also playing/logged in there).

## Why not the shared-instance design

An earlier attempt tried to make both jobs (visible+interactive dock,
*and* Mixer-routed audio) come from the exact same browser instance, by
rendering an `obs_source` directly into a custom GPU-backed Qt widget
(`obs_display_create` + manual mouse/keyboard forwarding, modeled on
OBS's own "Interact" window). That had a confirmed, live-reproduced
rendering bug: the widget's render surface bled onto / duplicated over
the actual Scene Preview canvas, and the overlapping area wasn't
selectable (a rendering artifact, not a misplaced but otherwise normal
widget). Root cause was never isolated — it needs real GPU debugging
(RenderDoc, or a line-by-line diff against `OBSQTDisplay`/
`OBSBasicInteraction`) to attempt safely, so it was abandoned rather
than iterated on further live in the user's actual OBS session.

If perfect dock/stream sync is needed again later, that's the design to
revisit — with proper GPU-debugging tooling available before testing
live, not by guessing again.

## Canvas-bleed bug (fixed 2026-09-15) and known Mixer limitation

The hidden audio browser was originally created at 400x400 and pinned to
Mixer channel 7 via `obs_set_output_source()`. That API doesn't just
feed the audio mixer — libobs's main render loop
(`obs_render_main_texture`) walks every global output channel each
frame and calls `obs_source_video_render()` on whatever occupies it,
the same mechanism that draws the active program scene on channel 0.
A video-capable source on that channel was therefore being painted
directly onto the canvas every frame with no scene item behind it —
unselectable, undeletable, fixed in place regardless of scene. It was
never a CEF popup window (that was a wrong first guess). Fixed by
shrinking the hidden source's render target to 1x1 in
`helixtools-music.cpp` — audio capture doesn't depend on video size.

Separately: per `OBSBasic_SceneItems.cpp`'s `SourceActivated`/
`ActivateAudioSource`, *any* audio-capable source that becomes active
on *any* channel (not just OBS's own 1-6) gets a strip added to the
Audio Mixer dock automatically — no special-casing needed to "unlock"
that. But the hidden background browser and the visible
dock browser are two separate CEF instances, so the Mixer strip for
"HelixTools Music" only has anything to show once that hidden instance
is itself producing audio — pressing play in the dock does not drive
it. Current known state (confirmed 2026-09-15): the visible dock's
audio plays out through the normal OS/Desktop Audio device instead, not
through the isolated Mixer channel. Revisit with a real dock/mixer sync
mechanism (or the shared-instance redesign above) if isolated audio
routing is needed later.
