@echo off
setlocal EnableExtensions
title AI Depth Pro - Ready-to-use Installer
echo Close DaVinci Resolve before installing.
echo Windows may request administrator permission.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install-bundle.ps1" -BundlePath "%~dp0AI-Depth-Pro.ofx.bundle"
if errorlevel 1 (
    echo Installation failed. See the error above.
    pause
    exit /b 1
)
echo Installation finished. Restart Resolve and add AI Depth Pro from OpenFX.
pause
exit /b 0
