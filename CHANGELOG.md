# Fork changes

## Native network workspace candidate

The explicit main-window action now opens a modeless native network workspace with real immutable
background snapshots, selected-adapter rates and graph, native adapter/sample tables, details,
plain name/description search, display pause/resume and observational restart. Existing History,
Plugins and Settings commands remain reachable; the previous floating-window toggle remains a
workspace button. Startup does not open the workspace or change hidden-main/taskbar/tray defaults.
See [workspace behavior and limits](docs/features/native-dashboard/README.md).

The first exact root build.bat --silent run at f5d626e63369d32805dd9b30f11b5940a8151b95 passed with exit 0, 114 warnings and zero errors in 24.36 seconds. The follow-up explicitly marks the new Chinese source files as UTF-8 for the native compiler; its package result is pending. The selected expedited mode excludes new tests,
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
