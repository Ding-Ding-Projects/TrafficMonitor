# Fork changes

## Native task-oriented settings candidate

The settings entrypoint now provides real Monitoring, Floating monitor and Taskbar draft controls,
plain group search, Apply, Revert and page-default drafts. Apply retains the existing consumer and
configuration save path. All settings (legacy) keeps the complete original dialog available.
See [settings behavior and limits](docs/features/native-settings/README.md). The exact root build-installer.bat -Version 1.0.9002 at 93b32abc6acb80e13cbc5826d2cb873d9d083d47 exited 0, with 78 warnings and zero errors in its 9.56-second native build. The [local build receipt](docs/features/native-settings/build-receipt.json) binds those bytes to that source. Later documentation/inventory commits require their own release producer; the local candidate is not installed or published. No new tests, reviews, audits or screenshots run in this
expedited pass; full regex, localization, accessibility and remaining conversion stay unfinished.

The previous normal release [1.0.17](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.17-run39938393354-1)
is bound to f0d74c476842e1546ef1d7d666e5af914fc06044. All four release jobs succeeded and four
nonempty downloadable files were verified by the release owner. Installed-update evidence remains
separate. This prior release does not verify the new settings implementation.

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

## Pending silent installed updates

- Installed builds check the owning normal release channel every five minutes on a bounded background worker.
- Download and validate full packages quietly, defer cooperative restart through active editors, and retain next-launch apply.
- Add a persisted automatic-restart preference; preserve the existing automatic-check opt-out.
- Runtime update and recovery acceptance remain unverified in the current compile/package-only scope.

## Unreleased native search workbench

- Add independent native pattern editing and bounded asynchronous filtering to workspace, Settings and context-menu searches.
- Add search, literal replacement and split previews, syntax insertion and match-span navigation.
- Retain explicit unsupported syntax, parity gaps and deferred runtime evidence.

## Pending silent updater feed compatibility

- Accept uppercase and lowercase SHA-1 spelling from Squirrel while retaining exact hash, byte-size and single-package verification.
- Retain bounded local updater stage, phase, numeric transport result and HTTP status without response bodies or URLs. Managed startup rejection now remains a failed status instead of disappearing as disabled.
- Compile/package evidence is recorded separately. Automatic scheduling, safe restart and recovery remain unverified.
