@echo off
REM Stages every plugin built by build-all-33.bat into <plugin>\build33\rundir,
REM which is what installer\helixtools-plugin-suite-obs33.iss packages from.
REM The 32.x equivalent is install-all.bat (uses build\ and build\rundir).
set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist %CMAKE% set CMAKE=cmake

for %%P in (helixtools-service helixtools-overlays helixtools-music helixtools-clip helixtools-macros helixtools-hub helixtools-social helixtools-restream helixtools-dashboard) do (
  echo === Installing %%P ===
  %CMAKE% --install %%P\build33 --config RelWithDebInfo --prefix %%P\build33\rundir
  if errorlevel 1 exit /b 1
)
