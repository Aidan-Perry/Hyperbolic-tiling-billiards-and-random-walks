@echo off
setlocal EnableExtensions
title Hyperbolic Tiling Billiards

set "SCRIPT_DIR=%~dp0"
set "APP_URL=http://127.0.0.1:8765"
set "BUILD_DIR=%SCRIPT_DIR%build-windows"
set "ENGINE_PATH=%BUILD_DIR%\htb_engine.exe"

cd /d "%SCRIPT_DIR%"
if errorlevel 1 goto directory_error

rem Prefer the MSYS2 UCRT64 toolchain. MSYS2_ROOT may override its usual location.
if exist "C:\msys64\ucrt64\bin\gcc.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if defined MSYS2_ROOT if exist "%MSYS2_ROOT%\ucrt64\bin\gcc.exe" set "PATH=%MSYS2_ROOT%\ucrt64\bin;%PATH%"

call :health
if not errorlevel 1 (
  echo Hyperbolic Tiling Billiards is already running.
  call :open_workspace
  exit /b 0
)

echo Starting Hyperbolic Tiling Billiards for Windows...

for %%P in (node.exe npm.cmd cmake.exe ninja.exe pkg-config.exe gcc.exe) do (
  where %%P >nul 2>&1
  if errorlevel 1 (
    set "MISSING_PROGRAM=%%P"
    goto missing_dependency
  )
)

pkg-config --exists flint jansson
if errorlevel 1 goto missing_native_library

if not exist "%SCRIPT_DIR%node_modules\p5\lib\p5.min.js" (
  echo Installing the local p5.js dependency...
  call npm install
  if errorlevel 1 goto npm_error
)

echo Checking the native engine...
cmake -S "%SCRIPT_DIR%." -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=gcc
if errorlevel 1 goto cmake_error

cmake --build "%BUILD_DIR%" --parallel 4
if errorlevel 1 goto build_error

if not exist "%ENGINE_PATH%" goto engine_error

echo Launching the local server...
set "HTB_ENGINE=%ENGINE_PATH%"
start "" /b powershell.exe -NoProfile -WindowStyle Hidden -Command "$base='%APP_URL%'; for($i=0; $i -lt 100; $i++){ try { $health=Invoke-RestMethod -Uri ($base+'/api/health') -TimeoutSec 1; if($health.ok){ Start-Process ($base+'/disk'); Start-Process ($base+'/plane'); Start-Process ($base+'/diagnostics'); exit 0 } } catch {}; Start-Sleep -Milliseconds 100 }; exit 1"

echo.
echo The simulator will open three synchronized browser pages.
echo Keep this window open. Press Control-C to stop the server.
node "%SCRIPT_DIR%src\server\server.js"
set "SERVER_EXIT=%ERRORLEVEL%"
echo.
echo The local server has stopped with exit code %SERVER_EXIT%.
pause
exit /b %SERVER_EXIT%

:health
powershell.exe -NoProfile -Command "try { $health=Invoke-RestMethod -Uri '%APP_URL%/api/health' -TimeoutSec 1; if($health.ok){ exit 0 } } catch {}; exit 1" >nul 2>&1
exit /b %ERRORLEVEL%

:open_workspace
start "" "%APP_URL%/disk"
start "" "%APP_URL%/plane"
start "" "%APP_URL%/diagnostics"
exit /b 0

:missing_dependency
echo.
echo Missing required program: %MISSING_PROGRAM%
echo Install Node.js 20 or newer and the MSYS2 UCRT64 packages documented in README.md.
echo The launcher automatically recognizes MSYS2 at C:\msys64 or the MSYS2_ROOT environment variable.
goto failure

:missing_native_library
echo.
echo FLINT 3.x or Jansson is unavailable to pkg-config.
echo Install the MSYS2 UCRT64 FLINT and Jansson packages documented in README.md.
goto failure

:npm_error
echo.
echo npm install failed. Check the messages above.
goto failure

:cmake_error
echo.
echo CMake configuration failed.
goto failure

:build_error
echo.
echo The native engine failed to compile.
goto failure

:engine_error
echo.
echo The native executable was not produced at:
echo   %ENGINE_PATH%
goto failure

:directory_error
echo.
echo The launcher could not enter its application directory.

:failure
echo.
pause
exit /b 1
