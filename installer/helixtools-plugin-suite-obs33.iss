; helixtools-plugin-suite-obs33.iss - Helix ToolKit installer for OBS Studio
; 33 and newer. OBS 33 changed where third-party plugins live, so this is
; a separate installer from helixtools-plugin-suite.iss (OBS 32 and older).
; It refuses to install onto OBS older than 33.
; Copyright (C) 2026 HelixTools
; SPDX-License-Identifier: GPL-2.0-or-later

#define MyAppName "Helix ToolKit"
; Keep in step with helixtools-plugin-suite.iss so both builds can share one
; release tag (the update checker compares this registry version).
#define MyAppVersion "0.8.0"
#define MyAppPublisher "HelixTools"
#define MyAppURL "https://helixtools.org"

; Build with build-all-33.bat <plugin> for each plugin, then run
; install-all-33.bat to stage <plugin>\build33\rundir.
;
; OBS 33 plugin layout (Windows):
;   installed OBS: C:\ProgramData\obs-studio\plugins\<name>\<name>.dll
;                  C:\ProgramData\obs-studio\plugins\<name>\data\...
;   portable OBS:  <OBS folder>\plugins\<name>\...
; Legacy <OBS>\obs-plugins\64bit still loads in 33 but stops in OBS 34.
#define ServiceRoot   "..\helixtools-service\build33\rundir"
#define OverlaysRoot  "..\helixtools-overlays\build33\rundir"
#define MusicRoot     "..\helixtools-music\build33\rundir"
#define ClipRoot      "..\helixtools-clip\build33\rundir"
#define MacrosRoot    "..\helixtools-macros\build33\rundir"
#define HubRoot       "..\helixtools-hub\build33\rundir"
#define SocialRoot    "..\helixtools-social\build33\rundir"
#define RestreamRoot  "..\helixtools-restream\build33\rundir"
#define DashboardRoot "..\helixtools-dashboard\build33\rundir"

#define PluginRoot "{code:GetPluginRoot}"

[Setup]
; Same AppId as the OBS 32 installer: it is the same product, so installing
; this over an older Helix ToolKit is an upgrade, not a second program.
AppId={{6D3F9A21-7C4B-4E8D-9A1F-3B5C7D2E8F4A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion} (OBS 33+)
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/support
DefaultDirName={code:GetOBSInstallDir}
DisableDirPage=no
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=LICENSE-with-footer.txt
OutputDir=Output
OutputBaseFilename=HelixToolKit-{#MyAppVersion}-OBS33-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
MinVersion=10.0.19041
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayName={#MyAppName} (OBS 33+)
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
Name: "restream";     Description: "Restream (multi-destination RTMP)";         Types: full custom
Name: "dashboard";    Description: "Dashboard (Twitch/YouTube/Kick/Blaze/VPZone selector dock)"; Types: full custom

[Files]
Source: "{#ServiceRoot}\obs-plugins\64bit\helixtools-service.dll"; DestDir: "{#PluginRoot}\helixtools-service"; Flags: ignoreversion; Components: service

Source: "{#OverlaysRoot}\obs-plugins\64bit\helixtools-overlays.dll"; DestDir: "{#PluginRoot}\helixtools-overlays"; Flags: ignoreversion; Components: overlays

Source: "{#MusicRoot}\obs-plugins\64bit\helixtools-music.dll"; DestDir: "{#PluginRoot}\helixtools-music"; Flags: ignoreversion; Components: music

Source: "{#ClipRoot}\obs-plugins\64bit\helixtools-clip.dll"; DestDir: "{#PluginRoot}\helixtools-clip"; Flags: ignoreversion; Components: clip
Source: "{#ClipRoot}\obs-plugins\64bit\*.dll"; DestDir: "{#PluginRoot}\helixtools-clip"; Excludes: "helixtools-*.dll"; Flags: ignoreversion skipifsourcedoesntexist; Components: clip
; Qt's Schannel TLS backend plugin, matching the Qt6 build OBS Studio
; itself bundles (see helixtools-clip\vendor\NOTE.md). This one comes from
; the obs-deps 2026-10-23 Qt bundle that OBS 33.0.1 is built with (Qt
; 6.11.1). It must live in OBS's own folder.
Source: "..\helixtools-clip\vendor\obs33\qschannelbackend.dll"; DestDir: "{app}\bin\64bit\tls"; Flags: ignoreversion; Components: clip

Source: "{#MacrosRoot}\obs-plugins\64bit\helixtools-macros.dll"; DestDir: "{#PluginRoot}\helixtools-macros"; Flags: ignoreversion; Components: macros
Source: "{#MacrosRoot}\obs-plugins\64bit\*.dll"; DestDir: "{#PluginRoot}\helixtools-macros"; Excludes: "helixtools-*.dll"; Flags: ignoreversion skipifsourcedoesntexist; Components: macros
Source: "{#MacrosRoot}\data\obs-plugins\helixtools-macros\*"; DestDir: "{#PluginRoot}\helixtools-macros\data"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist; Components: macros

Source: "{#HubRoot}\obs-plugins\64bit\helixtools-hub.dll"; DestDir: "{#PluginRoot}\helixtools-hub"; Flags: ignoreversion; Components: hub

Source: "{#SocialRoot}\obs-plugins\64bit\helixtools-social.dll"; DestDir: "{#PluginRoot}\helixtools-social"; Flags: ignoreversion; Components: social

Source: "{#RestreamRoot}\obs-plugins\64bit\helixtools-restream.dll"; DestDir: "{#PluginRoot}\helixtools-restream"; Flags: ignoreversion; Components: restream

Source: "{#DashboardRoot}\obs-plugins\64bit\helixtools-dashboard.dll"; DestDir: "{#PluginRoot}\helixtools-dashboard"; Flags: ignoreversion; Components: dashboard
Source: "{#DashboardRoot}\data\obs-plugins\helixtools-dashboard\*"; DestDir: "{#PluginRoot}\helixtools-dashboard\data"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist; Components: dashboard

; License and third-party notices (Qt's TLS plugin is LGPL-3.0, so its
; license text and source pointers ship with the installer)
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "HelixTools-Plugins-LICENSE.txt"; Flags: ignoreversion
Source: "..\THIRD-PARTY-NOTICES.md"; DestDir: "{app}"; DestName: "HelixTools-Third-Party-Notices.txt"; Flags: ignoreversion
Source: "..\licenses\*"; DestDir: "{app}\HelixTools-Licenses"; Flags: ignoreversion

[Registry]
Root: HKLM; Subkey: "SOFTWARE\HelixTools\PluginSuite"; ValueType: string; ValueName: "OBSInstallDir"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\HelixTools\PluginSuite"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\HelixTools\PluginSuite"; ValueType: string; ValueName: "PluginRoot"; ValueData: "{code:GetPluginRoot}"; Flags: uninsdeletekey

[InstallDelete]
; Remove copies left by the OBS 32 installer (legacy layout). OBS 33 would
; otherwise list them as Legacy plugins, and OBS 34 will not load them.
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-service.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-overlays.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-restream.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-music.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-clip.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-macros.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-hub.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-social.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-dashboard.dll"
Type: files; Name: "{app}\obs-plugins\64bit\helixtools-rumble-studio.dll"
Type: filesandordirs; Name: "{app}\data\obs-plugins\helixtools-macros"
Type: filesandordirs; Name: "{app}\data\obs-plugins\helixtools-dashboard"

[UninstallDelete]
Type: filesandordirs; Name: "{app}\HelixTools-Licenses"
Type: files; Name: "{app}\bin\64bit\tls\qschannelbackend.dll"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-service"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-overlays"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-music"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-clip"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-macros"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-hub"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-social"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-restream"
Type: filesandordirs; Name: "{code:GetPluginRoot}\helixtools-dashboard"

[Code]
const
  MinOBSMajor = 33;

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

// Where the plugin folders go. The detected (installed) OBS uses the shared
// ProgramData location OBS 33 scans. Any other folder the user picked is
// treated as a portable OBS, whose plugins live in <OBS folder>\plugins.
function GetPluginRoot(Param: String): String;
var
  Detected: String;
begin
  Detected := DetectOBSInstallDir();
  if (Detected <> '') and
     (CompareText(RemoveBackslash(Detected), RemoveBackslash(ExpandConstant('{app}'))) = 0) then
    Result := ExpandConstant('{commonappdata}\obs-studio\plugins')
  else
    Result := RemoveBackslash(ExpandConstant('{app}')) + '\plugins';
end;

// Major version of obs64.exe in the given OBS folder, or 0 if unreadable.
function GetOBSMajor(Dir: String): Integer;
var
  MS, LS: Cardinal;
begin
  Result := 0;
  if GetVersionNumbers(RemoveBackslash(Dir) + '\bin\64bit\obs64.exe', MS, LS) then
    Result := MS shr 16;
end;

function OldOBSMessage(Major: Integer): String;
begin
  Result := 'This installer is for OBS Studio 33 and newer, but OBS Studio ' +
            IntToStr(Major) + '.x was found in that folder.' + #13#10#13#10 +
            'Please use the standard Helix ToolKit installer for OBS Studio 32 and earlier, ' +
            'or update OBS Studio first.';
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Dir: String;
  Major: Integer;
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
      exit;
    end;
    Major := GetOBSMajor(Dir);
    if (Major > 0) and (Major < MinOBSMajor) then
    begin
      MsgBox(OldOBSMessage(Major), mbError, MB_OK);
      Result := False;
    end;
  end;
end;

// Hard gate that also runs for silent installs (the directory page, and so
// NextButtonClick, is skipped with /SILENT or /VERYSILENT).
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Major: Integer;
begin
  Result := '';
  Major := GetOBSMajor(ExpandConstant('{app}'));
  if (Major > 0) and (Major < MinOBSMajor) then
    Result := OldOBSMessage(Major);
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
