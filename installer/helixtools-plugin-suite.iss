; helixtools-plugin-suite.iss - Combined standalone installer for all
; HelixTools OBS plugins, targeting a user's existing (vanilla) OBS
; Studio install.
; Copyright (C) 2026 HelixTools
; SPDX-License-Identifier: GPL-2.0-or-later

#define MyAppName "Helix ToolKit"
#define MyAppVersion "0.5.1"
#define MyAppPublisher "HelixTools"
#define MyAppURL "https://helixtools.org"

; Build every plugin first (each is its own out-of-tree CMake project):
;   cd standalone\<plugin-dir>
;   cmake -B build -DCMAKE_PREFIX_PATH="<path to an OBS libobs SDK/rundir>"
;   cmake --build build --config RelWithDebInfo
; Output lands in <plugin-dir>\build\rundir (see each plugin's CMakeLists.txt)
;
; helixtools-clip and helixtools-macros link Qt6 (Core/Network/Widgets).
; They do NOT need their own bundled Qt6 DLLs: they load into the OBS
; process, and Windows resolves their Qt6 imports from the copy OBS
; Studio itself already ships next to obs64.exe (the main executable's
; directory is part of the standard DLL search order for every module
; loaded into that process). This only holds if the target OBS Studio's
; bundled Qt6 is ABI-compatible with the Qt6 these plugins were built
; against (both currently use obsproject/obs-deps prebuilt Qt6, so this
; should match for any reasonably current OBS Studio release) — verify
; against a real install before shipping.
; helixtools-restream is back (2026-09-16) now that a "Restream
; Destinations..." Tools menu dialog exists to add/edit/remove
; destinations and persist them to disk (see restream-dialog.cpp /
; restream-storage.c) — it's no longer a no-op.
; helixtools-music is back (2026-09-15, third time) using the safer
; fallback design: a plain QCefWidget dock (same proven pattern as
; helixtools-hub/-social) for browsing/playback control, plus a
; separate private browser_source pinned to a Mixer channel for audio
; — two independent browser instances instead of one shared one. See
; NOTE.md in ..\helixtools-music for the full history of why the
; original shared-instance design (custom obs_display-backed dock
; widget) was dropped.
#define ServiceRoot   "..\helixtools-service\build\rundir"
#define OverlaysRoot  "..\helixtools-overlays\build\rundir"
#define MusicRoot     "..\helixtools-music\build\rundir"
#define ClipRoot      "..\helixtools-clip\build\rundir"
#define MacrosRoot    "..\helixtools-macros\build\rundir"
#define HubRoot       "..\helixtools-hub\build\rundir"
#define SocialRoot    "..\helixtools-social\build\rundir"
#define RumbleStudioRoot "..\helixtools-rumble-studio\build\rundir"
#define RestreamRoot  "..\helixtools-restream\build\rundir"

[Setup]
AppId={{6D3F9A21-7C4B-4E8D-9A1F-3B5C7D2E8F4A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/support
DefaultDirName={code:GetOBSInstallDir}
DisableDirPage=no
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=LICENSE-with-footer.txt
OutputDir=Output
OutputBaseFilename=HelixToolKit-{#MyAppVersion}-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
MinVersion=10.0.19041
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayName={#MyAppName}
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (all plugins)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "service";  Description: "Rumble RTMP Service (native stream key setup)"; Types: full custom; Flags: fixed
Name: "overlays"; Description: "Hub Overlays (chat, alerts, goals, minigames)";  Types: full custom
Name: "music";    Description: "HelixTools Music dock + mixer channel";         Types: full custom
Name: "clip";     Description: "Clip Creation (hotkey clipping)";               Types: full custom
Name: "macros";   Description: "Macro Engine (hotkey action sequences)";        Types: full custom
Name: "hub";      Description: "HelixTools Hub Dock (Push to Studio)";          Types: full custom
Name: "social";   Description: "HelixTools Social Media Dock";                  Types: full custom
Name: "rumblestudio"; Description: "Rumble Studio Dock";                        Types: full custom
Name: "restream";     Description: "Restream (multi-destination RTMP)";         Types: full custom

[Files]
Source: "{#ServiceRoot}\obs-plugins\64bit\helixtools-service.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: service

Source: "{#OverlaysRoot}\obs-plugins\64bit\helixtools-overlays.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: overlays

Source: "{#MusicRoot}\obs-plugins\64bit\helixtools-music.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: music

Source: "{#ClipRoot}\obs-plugins\64bit\helixtools-clip.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion recursesubdirs; Components: clip
Source: "{#ClipRoot}\obs-plugins\64bit\*.dll"; DestDir: "{app}\obs-plugins\64bit"; Excludes: "helixtools-*.dll"; Flags: ignoreversion skipifsourcedoesntexist; Components: clip

Source: "{#MacrosRoot}\obs-plugins\64bit\helixtools-macros.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: macros
Source: "{#MacrosRoot}\obs-plugins\64bit\*.dll"; DestDir: "{app}\obs-plugins\64bit"; Excludes: "helixtools-*.dll"; Flags: ignoreversion skipifsourcedoesntexist; Components: macros
Source: "{#MacrosRoot}\data\obs-plugins\helixtools-macros\*"; DestDir: "{app}\data\obs-plugins\helixtools-macros"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist; Components: macros

Source: "{#HubRoot}\obs-plugins\64bit\helixtools-hub.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: hub

Source: "{#SocialRoot}\obs-plugins\64bit\helixtools-social.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: social

Source: "{#RumbleStudioRoot}\obs-plugins\64bit\helixtools-rumble-studio.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: rumblestudio

Source: "{#RestreamRoot}\obs-plugins\64bit\helixtools-restream.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: restream

; License
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "HelixTools-Plugins-LICENSE.txt"; Flags: ignoreversion

[Registry]
Root: HKLM; Subkey: "SOFTWARE\HelixTools\PluginSuite"; ValueType: string; ValueName: "OBSInstallDir"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\HelixTools\PluginSuite"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

[UninstallDelete]
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-service.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-overlays.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-restream.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-music.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-clip.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-macros.dll"
Type: filesandordirs; Name: "{app}\data\obs-plugins\helixtools-macros"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-hub.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-social.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-rumble-studio.dll"

[Code]
// Look for a vanilla OBS Studio install via the same registry keys OBS's
// own installer and third-party plugin installers (obs-websocket, etc.)
// rely on. Falls back to the standard Program Files location, and finally
// to an empty string so the wizard's directory page prompts the user.
function DetectOBSInstallDir(): String;
var
  Dir, ExePath: String;
begin
  Result := '';

  if RegQueryStringValue(HKLM64, 'SOFTWARE\OBS Studio', '', Dir) and DirExists(Dir) then
  begin
    Result := Dir;
    exit;
  end;

  if RegQueryStringValue(HKLM32, 'SOFTWARE\OBS Studio', '', Dir) and DirExists(Dir) then
  begin
    Result := Dir;
    exit;
  end;

  if RegQueryStringValue(HKLM64, 'SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\obs64.exe', 'Path', Dir) and DirExists(Dir) then
  begin
    Result := Dir;
    exit;
  end;

  if RegQueryStringValue(HKLM64, 'SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\obs64.exe', '', ExePath) then
  begin
    Dir := ExtractFileDir(ExePath);
    if DirExists(Dir) then
    begin
      Result := Dir;
      exit;
    end;
  end;

  Dir := ExpandConstant('{autopf}\obs-studio');
  if DirExists(Dir) then
    Result := Dir;
end;

function GetOBSInstallDir(Param: String): String;
begin
  Result := DetectOBSInstallDir();
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Dir: String;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    Dir := WizardForm.DirEdit.Text;
    if not FileExists(Dir + '\bin\64bit\obs64.exe') then
    begin
      MsgBox('bin\64bit\obs64.exe was not found in that folder. Please select the folder ' +
             'that contains your OBS Studio install (e.g. C:\Program Files\obs-studio).', mbError, MB_OK);
      Result := False;
    end;
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  if CheckForMutexes('OBSStudioPortableMutex,OBSStudioMutex') then
  begin
    MsgBox('OBS Studio is currently running. Please close it before installing this plugin suite.', mbError, MB_OK);
    Result := False;
  end;
end;
