@echo off
setlocal
if /i "%~1"=="--verify-layout-observations" goto verifyLayoutObservations
if /i "%~1"=="--layout-observer" goto layoutObserver
if /i "%~1"=="--verify-material-visuals" goto verifyMaterialVisuals
if /i "%~1"=="--verify-material-inputs" goto verifyMaterialInputs
if /i "%~1"=="--verify-material-tabs-tooltip" goto verifyMaterialTabsTooltip
if /i "%~1"=="--verify-material-table" goto verifyMaterialTable
if /i "%~1"=="--uia-observer" goto uiaObserver
if /i "%~1"=="--verify-readonly-text" goto verifyReadOnlyText
if /i "%~1"=="--verify-title-bar" goto verifyTitleBar
if /i "%~1"=="--verify-alert-dialog" goto verifyAlertDialog
if /i "%~1"=="--verify-frameless-windows" goto verifyFramelessWindows
if /i "%~1"=="--verify-result-list" goto verifyResultList
if /i "%~1"=="--verify-preferences" goto verifyPreferences
if /i "%~1"=="--verify-regex-collections" goto verifyRegexCollections
if /i "%~1"=="--verify-settings" goto verifySettings
if /i "%~1"=="--verify-app-icons" goto verifyAppIcons
if /i "%~1"=="--docs-companion" goto docsCompanion
if /i "%~1"=="--verify-updater" goto verifyUpdater
if /i "%~1"=="--verify-update-banner" goto verifyUpdateBanner
if /i "%~1"=="--verify-diagnostic-instance" goto verifyDiagnosticInstance
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
:verifyUpdateBanner
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-update-banner.ps1"
exit /b %errorlevel%
:verifyDiagnosticInstance
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-diagnostic-instance.ps1"
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

:verifyRegexCollections
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-regex-collections.ps1"
exit /b %errorlevel%

:verifyResultList
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-result-list.ps1"
exit /b %errorlevel%

:uiaObserver
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-native-uia-observer.ps1"
exit /b %errorlevel%

:layoutObserver
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-native-layout-observer.ps1"
exit /b %errorlevel%

:verifyTitleBar
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-title-bar.ps1"
exit /b %errorlevel%

:verifyAlertDialog
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-alert-dialog.ps1"
exit /b %errorlevel%

:verifyFramelessWindows
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-frameless-windows.ps1"
exit /b %errorlevel%

:verifyReadOnlyText
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-readonly-text.ps1"
exit /b %errorlevel%

:verifyMaterialVisuals
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-material-visuals.ps1"
exit /b %errorlevel%

:verifyMaterialInputs
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-material-inputs.ps1"
exit /b %errorlevel%

:verifyMaterialTabsTooltip
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-material-tabs-tooltip.ps1"
exit /b %errorlevel%

:verifyMaterialTable
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-tables.ps1"
exit /b %errorlevel%

:verifyLayoutObservations
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-native-layout-observations.ps1"
exit /b %errorlevel%

:verifyAppIcons
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\test-app-icons.ps1" %2 %3 %4 %5 %6 %7 %8 %9
exit /b %errorlevel%
