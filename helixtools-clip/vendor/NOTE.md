# qschannelbackend.dll

Qt6's `QtNetwork` TLS backend plugin (Windows Schannel-based — no
OpenSSL dependency). `helixtools-clip` uses `QNetworkAccessManager`/
`QSslSocket` for its HTTPS calls (Supabase + the clip API), but runs
inside OBS's own process and shares whatever `Qt6Network.dll` OBS itself
loads. A stock OBS Studio install ships `Qt6Network.dll` (pulled in as a
Qt6/Qt6Widgets dependency for OBS's own UI) but **no `tls\` plugin
folder at all** — OBS's own code apparently never uses `QSslSocket`
directly, so it never bundles a TLS backend. Without this file present
in the right place, any HTTPS call made via Qt from an out-of-tree
plugin fails immediately with "No functional TLS backend was found."

## Where this came from

Not built from this repo — extracted as-is from the exact Qt6 bundle
`obsproject/obs-studio` itself used to build the specific OBS release
this was tested against:

1. `obsproject/obs-studio` tag `32.2.2`'s `CMakePresets.json` pins its
   Windows Qt6 dependency to `obs-deps` release `2026-07-15`.
2. That release's `windows-deps-qt6-2026-07-15-x64.zip` contains
   `plugins/tls/qschannelbackend.dll`.
3. `FileVersion`/`ProductVersion` on that DLL: `6.11.1.0` — confirmed to
   exactly match `Qt6Network.dll` (also `6.11.1.0`) in a real installed
   OBS Studio 32.2.2 on this machine.

Source: `https://github.com/obsproject/obs-deps/releases/tag/2026-07-15`

## License

Qt6 is LGPLv3-licensed. This is an unmodified binary straight from Qt's
own official build (via `obs-deps`, which itself builds unmodified Qt
source from `code.qt.io`) — not a HelixTools-authored file. LGPLv3's
source-availability and relink obligations are satisfied by Qt being
fully open source, and by this being a swappable runtime plugin file (a
user can replace it with their own Qt build of the same component at
any time, same as any other Qt TLS backend plugin).

## Important: this is version-locked to OBS's currently-bundled Qt

This fix works because it's byte-identical (well, matching build-key)
to the specific Qt 6.11.1 build inside OBS Studio 32.2.2. **If OBS
Studio bumps its bundled Qt6 version in a future release, this file
will need to be re-sourced the same way**, or `helixtools-clip` will
start failing "No functional TLS backend" again on that newer OBS:

1. Check the new OBS release's tag on GitHub, read its
   `CMakePresets.json` for the `qt6` dependency's `version` field (an
   `obs-deps` release date).
2. Download `windows-deps-qt6-<that-date>-x64.zip` from
   `github.com/obsproject/obs-deps/releases`.
3. Extract `plugins/tls/qschannelbackend.dll`, confirm its `FileVersion`
   matches the target OBS's `Qt6Network.dll`, and replace this file.

If this becomes too much of a moving target to keep up with, the
durable alternative is switching `clip-manager.cpp`'s HTTPS calls to
Windows' native WinHTTP API instead of Qt networking — no Qt TLS plugin
dependency at all, works regardless of whatever Qt version OBS bundles.
That approach was designed and ready to implement, but this simpler
fix was tried first.
