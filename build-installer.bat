@echo off
setlocal
call "%~dp0build.bat"
if errorlevel 1 exit /b %errorlevel%
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\package.ps1" %*
exit /b %errorlevel%
