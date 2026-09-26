@echo off
setlocal EnableExtensions
title AI Depth Pro Installer
cd /d "%~dp0"

set "BUILD_ONLY=0"
set "NO_PAUSE=0"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--build-only" (
    set "BUILD_ONLY=1"
) else if /I "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else (
    echo Unknown option: %~1
    goto failed
)
shift
goto parse_args

:args_done
where powershell.exe >nul 2>&1
if errorlevel 1 (
    echo PowerShell was not found.
    goto failed
)

where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo CMake was not found. Install Visual Studio 2022 C++ tools and CMake first.
    goto failed
)

echo [1/4] Preparing the verified AI model and runtime...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\setup-dependencies.ps1"
if errorlevel 1 goto failed

echo.
echo [2/4] Building AI Depth Pro...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\verify.ps1"
if errorlevel 1 goto failed

echo.
echo [3/4] Build and all tests passed.
if "%BUILD_ONLY%"=="1" goto success

echo.
echo [4/4] Installing the OpenFX bundle...
echo Windows may ask for administrator permission.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\install-bundle.ps1" -BundlePath "%~dp0build\AI-Depth-Pro.ofx.bundle"
if errorlevel 1 goto failed

:success
echo.
if "%BUILD_ONLY%"=="1" (
    echo AI Depth Pro is ready in build\AI-Depth-Pro.ofx.bundle
) else (
    echo AI Depth Pro was installed successfully.
    echo Restart DaVinci Resolve, then add AI Depth Pro from the OpenFX effects list.
)
if "%NO_PAUSE%"=="0" pause
exit /b 0

:failed
echo.
echo Installation stopped because a step failed. No new plugin was installed.
if "%NO_PAUSE%"=="0" pause
exit /b 1
