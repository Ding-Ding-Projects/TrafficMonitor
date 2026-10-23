# Fork changes

## Native network workspace candidate

The explicit main-window action now opens a modeless native network workspace with real immutable
background snapshots, selected-adapter rates and graph, native adapter/sample tables, details,
plain name/description search, display pause/resume and observational restart. Existing History,
Plugins and Settings commands remain reachable; the previous floating-window toggle remains a
workspace button. Startup does not open the workspace or change hidden-main/taskbar/tray defaults.
See [workspace behavior and limits](docs/features/native-dashboard/README.md).

The exact root build-installer.bat -Version 1.0.9001 run at 7e560bfefa61952339f534b72a794bd6b93f4181 passed with exit 0. Its incremental native build reported 2 warnings and zero errors in 6.07 seconds. The preceding full native build passed with 114 warnings and zero errors. The local package is not installed or published. See the [source-bound build receipt](docs/features/native-dashboard/build-receipt.json). The selected expedited mode excludes new tests,
reviews, audits and screenshots. No new runtime, accessibility or visual acceptance is claimed.
Native tables and partial English/Chinese chrome do not satisfy full custom table or language
acceptance. Full regex parity, onboarding and all other unfinished requirements remain open.

## Installer foundation, in verification

- Add native x64 build and genuine unsigned Squirrel.Windows packaging entrypoints.
- Include matching application-local Microsoft CRT and MFC runtime dependencies.
- Handle installer callbacks before interactive startup and single-instance detection.
- Register current-user login startup through a stable launcher, with ownership checks and readback.
- Keep installed configuration outside versioned executable folders.
- Default missing installed preferences to taskbar-visible and main-window-hidden.
- Preserve saved settings, startup choices, portable behavior, plugins, and deleted shortcuts.
- Add a source-bound normal-release workflow with retained safe build outputs.

Runtime installation and update acceptance remain separate from source and build verification.
