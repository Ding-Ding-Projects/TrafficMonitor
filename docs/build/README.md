# Native build and Squirrel packaging

Run `build.bat` for the maintained x64 `Release (lite)` build using MSVC v143,
MFC and Windows SDK 10.0.22621.0. Run `build-installer.bat -Version 1.0.1`
to build and produce a genuine unsigned Squirrel.Windows installer.

The scripts discover Visual Studio 2022, reuse an existing compatible installation,
and obtain missing components through the Microsoft-signed official Build Tools
bootstrapper. Installation preserves existing installations and may request native
UAC consent. Cancellation, failed installation and restart-required outcomes fail
with a nonzero result. No host restart is initiated. The bootstrapper uses the
Microsoft-maintained VS 17 release channel and validates its Authenticode identity;
that channel is not an immutable toolchain pin. NuGet 6.12.1 and Squirrel.Windows
2.0.1 downloads are pinned with SHA-256 hashes in `scripts/package.ps1`.

## Installed runtime inventory

The payload includes the matching x64 Microsoft VC143 CRT and MFC redistributable
DLLs from the selected compiler installation, installed beside TrafficMonitor.exe.
This application-local installation requires no separate runtime installation,
preserves compatible system runtimes, and makes DLL readiness part of packaging.
The package checks mfc140u.dll, vcruntime140.dll, vcruntime140_1.dll and msvcp140.dll.
Squirrel Setup.exe requires .NET Framework 4.8 and carries its genuine framework
bootstrap selection; the supported Windows 10/11 target normally includes it.
Missing framework installation follows the bootstrapper's native consent and result
handling. Restart/cancellation and fresh-host installer runtime acceptance still
require actual native verification; packaging alone proves neither.

The package contains language and skin resources, license, `build-manifest.json`
(source commit/version/configuration) and `installed-mode.ini` with
`[installation] managed=true`. Stable package identity is `TrafficMonitor`.

## Outputs and publication

`artifacts/releases` contains Setup.exe, RELEASES, the full .nupkg and SHA256SUMS.txt.
These are real Squirrel outputs. An input NuGet package alone is not an installer.
The workflow calls both root entrypoints and publishes normal releases targeted at
the exact workflow source SHA. A unique run-based tag prevents overwriting releases.
Automatic publication is limited to main pushes; tag pushes cannot recurse.
Manual dispatch must target main. The workflow has no test or lint step.

Build dependencies: Windows PowerShell 5.1, .NET Framework, Microsoft Visual Studio
2022 Build Tools, v143 x64 tools, MFC, SDK 10.0.22621.0, NuGet 6.12.1 and
Squirrel.Windows 2.0.1. Publication additionally uses the hosted runner's GitHub CLI.
Safe build logs and release outputs upload even after failure. Source, caches,
credentials and dependency installations are excluded. Fresh-host bootstrap,
installation, cancellation, update and uninstall acceptance remain unverified until
recorded from actual runs.
# Build and release documentation

- [Documentation download binding](release-download-binding.md): source-bound release metadata and separate publication/runtime verdicts.

