@echo off
setlocal
if /i "%~1"=="--site" goto site
if /i "%~1"=="/s" goto silent
if /i "%~1"=="--silent" goto silent
if "%SILENT%"=="1" goto silent
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" %*
exit /b %errorlevel%
:silent
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" -Silent
exit /b %errorlevel%
:site
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-site.ps1"
exit /b %errorlevel%
