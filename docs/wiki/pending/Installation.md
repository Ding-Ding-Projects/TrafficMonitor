# Install or update TrafficMonitor

Use this fork's normal releases, not an upstream portable archive:
[TrafficMonitor releases](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases).
Download `Setup.exe` from the intended release. The installer is unsigned and may trigger an
unknown-publisher or SmartScreen warning. No public certificate authenticity is claimed.

## Installation behavior

Installation is per current user. Required matching CRT/MFC DLLs are installed beside the versioned
application, so global runtime installation is not needed for those files. The Squirrel bootstrap
also handles its supported framework requirement. This host already had that framework; a genuinely
fresh host without it has not been verified.

On a fresh configuration, startup at interactive sign-in is enabled, the taskbar meter is visible,
and the floating main window is hidden. Existing saved settings override those defaults. Startup and
shortcuts use the stable installation launcher, not a version directory that an update can remove.

## Updates and data

Installed settings live outside version directories in the current user's application-data area.
Supported updates preserve explicit startup/display choices and existing user data. Release 1.0.8
was installed through the supported updater from 1.0.4; saved configuration bytes and startup
registration were unchanged. Do not replace installed files by copying an upstream portable archive.

Manual update checks in installed mode open this fork's release page. Automatic checking, staging
and ready-to-restart UI remain unfinished. Portable-mode update behavior remains upstream behavior.

## Removal and optional hardware monitoring

Use the supported uninstaller. It removes only this installation's owned startup registration and
shortcuts; saved data is preserved by default. Do not delete unrelated startup entries or settings.
Optional hardware plugins are acquired separately through their supported upstream instructions.
They are not silently installed. Some hardware access may require elevation; elevated automatic
startup is not enabled by default.
