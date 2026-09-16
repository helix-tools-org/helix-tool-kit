@echo off
set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist %CMAKE% set CMAKE=cmake

for %%P in (helixtools-service helixtools-overlays helixtools-music helixtools-clip helixtools-macros helixtools-hub helixtools-social helixtools-rumble-studio helixtools-restream) do (
  echo === Installing %%P ===
  %CMAKE% --install %%P\build --config RelWithDebInfo --prefix %%P\build\rundir
  if errorlevel 1 exit /b 1
)
