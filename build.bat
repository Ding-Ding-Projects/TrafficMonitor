@echo off
setlocal
if /i "%~1"=="--verify-readonly-text" goto verifyReadOnlyText
if /i "%~1"=="--verify-result-list" goto verifyResultList
if /i "%~1"=="--verify-preferences" goto verifyPreferences
if /i "%~1"=="--verify-settings" goto verifySettings
if /i "%~1"=="--docs-companion" goto docsCompanion
if /i "%~1"=="--verify-updater" goto verifyUpdater
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
:verifyUpdater
if /i "%~2"=="--baseline" goto verifyUpdaterBaseline
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-update-protocol.ps1"
exit /b %errorlevel%
:verifySettings
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-settings.ps1"
exit /b %errorlevel%
:verifyUpdaterBaseline
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-update-protocol.ps1" -Baseline
exit /b %errorlevel%
:docsCompanion
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-docs-companion.ps1" %2 %3 %4 %5 %6 %7 %8 %9
exit /b %errorlevel%

:verifyPreferences
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-preferences.ps1"
exit /b %errorlevel%

:verifyResultList
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-result-list.ps1"
exit /b %errorlevel%

:verifyReadOnlyText
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-readonly-text.ps1"
exit /b %errorlevel%
