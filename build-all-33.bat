@echo off
setlocal

REM Builds one HelixTools plugin against OBS Studio 33.x.
REM The 32.x build uses build-all.bat; this script is only for the 33+ branch.
REM
REM One-time SDK setup (OBS 33.0.1 source checked out to C:\obs33-sdk):
REM   cmake -S . -B build_x64 -G "Visual Studio 17 2022" -A x64 ^
REM     -DENABLE_FRONTEND=OFF -DENABLE_BROWSER=OFF -DENABLE_PLUGINS=OFF ^
REM     -DENABLE_SCRIPTING=OFF -DENABLE_VLC=OFF -DENABLE_AJA=OFF ^
REM     -DENABLE_WEBSOCKET=OFF -DENABLE_NEW_MPEGTS_OUTPUT=OFF
REM   cmake --build build_x64 --config RelWithDebInfo ^
REM     --target libobs obs-frontend-api w32-pthreads
REM
REM OBS_ROOT defaults to C:/obs33-sdk. Keep the SDK outside Dropbox.

if "%OBS_ROOT%"=="" set OBS_ROOT=C:/obs33-sdk
set OBS_ROOT=%OBS_ROOT:\=/%

set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist %CMAKE% set CMAKE=cmake

set SDK_LIBOBS=%OBS_ROOT%/build_x64/libobs
set SDK_FRONTEND=%OBS_ROOT%/build_x64/frontend/api
set SDK_PTHREADS=%OBS_ROOT%/build_x64/deps/w32-pthreads
set OBS_DEPS=%OBS_ROOT%/.deps/obs-deps-2026-10-23-x64
set QT6_DIR=%OBS_ROOT%/.deps/obs-deps-qt6-2026-10-23-x64
set PREFIX=%SDK_LIBOBS%;%SDK_FRONTEND%;%SDK_PTHREADS%;%OBS_DEPS%;%QT6_DIR%

REM libobsConfig.cmake needs obs-studio's own FindSIMDe.cmake etc.
set MODULE_PATH=%OBS_ROOT%/cmake/finders

REM Same Windows build workarounds as build-all.bat.
set TrackFileAccess=false

set PLUGIN=%1
if "%PLUGIN%"=="" (
  echo Usage: build-all-33.bat ^<plugin-dir-name^>
  exit /b 1
)

REM Separate build dir so 32.x and 33.x builds never mix.
cd %PLUGIN%
%CMAKE% -S . -B build33 -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="%PREFIX%" ^
  -DCMAKE_MODULE_PATH="%MODULE_PATH%" ^
  -DCMAKE_C_FLAGS="/FS" -DCMAKE_CXX_FLAGS="/FS" ^
  -DCMAKE_SHARED_LINKER_FLAGS="/INCREMENTAL:NO"
if errorlevel 1 exit /b 1
%CMAKE% --build build33 --config RelWithDebInfo -- /m
