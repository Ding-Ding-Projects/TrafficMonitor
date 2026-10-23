# Installation and startup

This fork builds the maintained native x64 edition. Its installer is a genuine unsigned
Squirrel.Windows package containing the application and its matching Microsoft CRT/MFC runtime
DLLs. The runtime files are installed beside the application so setup does not replace unrelated
system-wide runtime installations. Squirrel checks its .NET Framework prerequisite.

## First installation

Installed mode is identified by the package's `installed-mode.ini` marker. On first launch,
missing display preferences default to showing the taskbar monitor and hiding the main floating
window. Saved preferences take precedence. Use the tray menu to show the main window or open settings.

Setup registers the current user's login startup through the installation's stable `Update.exe`
launcher. It does not register elevated startup or change the operating system's sign-in behavior.
An existing startup entry for a different installation is protected rather than overwritten.

## Updates and removal

Installed builds provide [silent staged updates](silent-updates.md), checking every
five minutes and restarting cooperatively when no active editing work would be lost.
The independent automatic-restart preference can leave application until next launch.

Installed configuration and traffic history use the existing application-data location rather
than versioned executable folders. Updates preserve startup choices and existing display settings.
Stable shortcuts continue to work across versions; updates do not recreate shortcuts a user deleted.
Uninstall removes the owned startup entry and application shortcuts while retaining user data.

Portable builds retain upstream portable-mode behavior and display defaults. Plugin interfaces
remain native; optional temperature monitoring uses the upstream hardware plugin and may have
separate administrator requirements. Setup does not silently install third-party plugins.

## Build, download, and verification

See [build and package instructions](../../build/README.md). Customized releases appear in this
fork's [release list](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases).
An installer is not accepted merely because compilation succeeds: verify package hashes, runtime
DLLs, actual installation, startup command, user-setting preservation, and real taskbar behavior.
No restart or sign-out is performed by the verification procedure.

The interface redesign and broader required-feature delivery are a later stage. Current build
and runtime evidence is recorded in [the handoff](../../../HANDOFF.md).
