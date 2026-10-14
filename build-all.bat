@echo off
setlocal

if "%OBS_ROOT%"=="" (
  echo OBS_ROOT is not set. Point it at your obs-studio checkout/build, e.g.:
  echo   set OBS_ROOT=C:\path\to\obs-studio
  exit /b 1
)

REM Normalize to forward slashes. A mixed backslash/forward-slash path
REM (e.g. from `set OBS_ROOT=C:\foo\bar`) gets embedded as a literal
REM string in CMake-generated try_compile scripts (FindThreads, etc.),
REM where backslash sequences like \U or \A are parsed as invalid
REM escapes and abort configuration.
set OBS_ROOT=%OBS_ROOT:\=/%

set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist %CMAKE% set CMAKE=cmake

set SDK_LIBOBS=%OBS_ROOT%/build_x64/libobs
set SDK_FRONTEND=%OBS_ROOT%/build_x64/frontend/api
set SDK_PTHREADS=%OBS_ROOT%/build_x64/deps/w32-pthreads
set OBS_DEPS=%OBS_ROOT%/.deps/obs-deps-2025-08-23-x64
set QT6_DIR=%OBS_ROOT%/.deps/obs-deps-qt6-2025-08-23-x64
set PREFIX=%SDK_LIBOBS%;%SDK_FRONTEND%;%SDK_PTHREADS%;%OBS_DEPS%;%QT6_DIR%

REM libobsConfig.cmake calls find_dependency(SIMDe), which needs
REM obs-studio's own FindSIMDe.cmake finder module on the module path
REM (it's not shipped as part of the installed libobs SDK itself).
set MODULE_PATH=%OBS_ROOT%/cmake/finders

REM MSBuild's file tracker (Tracker.exe) makes link.exe die with a random
REM garbage exit code (MSB6006) on some Windows builds, which also breaks
REM CMake's compiler detection. Nothing here needs incremental-build
REM dependency tracking, so turn it off.
set TrackFileAccess=false
REM Incremental links (.ilk) hit the same failure on rebuilds, so every
REM build links non-incrementally (see CMAKE_SHARED_LINKER_FLAGS below).

set PLUGIN=%1
if "%PLUGIN%"=="" (
  echo Usage: build-all.bat ^<plugin-dir-name^>
  exit /b 1
)

cd %PLUGIN%
%CMAKE% -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="%PREFIX%" ^
  -DCMAKE_MODULE_PATH="%MODULE_PATH%" ^
  -DCMAKE_C_FLAGS="/FS" -DCMAKE_CXX_FLAGS="/FS" ^
  -DCMAKE_SHARED_LINKER_FLAGS="/INCREMENTAL:NO"
if errorlevel 1 exit /b 1
%CMAKE% --build build --config RelWithDebInfo -- /m
