# Fork changes

## Accepted readiness release 1.0.31

Normal release 1.0.31 is bound to source 891fe49f89914d088278c2d29a84d61ce30b43cd. All four release assets were downloaded and matched their published sizes and SHA-256 digests. The matching existing installation was manually updated through safe cooperative closure, Squirrel update and stable launcher, each exit 0. Released and installed executable hashes match; saved data and startup registration were preserved. Hidden main/taskbar-visible readiness is metadata proof only. The new identity-readiness handshake still requires a later genuine natural update. See [sanitized acceptance](docs/features/installation/silent-installed31-acceptance.json). Tests, reviews, audits and screenshots were not run.

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

## Unreleased modeless regex presentation

- Replace the modal regex decision with a caller-owned modeless native tool window.
- Revalidate current owner rows on Apply, cancel drafts on owner changes and preserve menu lifecycle routing.
- Keep the bounded NFA engine unchanged; runtime acceptance and complete parity remain unfinished.

## Pending silent updater feed compatibility

- Accept uppercase and lowercase SHA-1 spelling from Squirrel while retaining exact hash, byte-size and single-package verification.
- Retain bounded local updater stage, phase, numeric transport result and HTTP status without response bodies or URLs. Managed startup rejection now remains a failed status instead of disappearing as disabled.
- Compile/package evidence is recorded separately. Automatic scheduling, safe restart and recovery remain unverified.

## Native expression collections candidate

- Add explicit named Save, Load draft, Rename, Delete, Refresh and backup recovery in the existing modeless workbench.
- Persist bounded engine-bound expression records in the existing data directory without subjects or implicit history.
- Preserve independent owner filters and current-corpus Apply validation; full parity and runtime acceptance remain unfinished.

## Pending UTF-8 updater feed marker repair

- Verify the original RELEASES bytes before skipping one leading UTF-8 BOM for row parsing.
- Preserve strict digest, size, basename, entry count and accepted-main checks; record fixed local integrity rejection substages.
- Runtime automatic-update acceptance remains unverified.

- The supported package writer emits and reads back explicit strict BOM-free UTF-8 RELEASES before public checksum generation, retaining Squirrel row text and package bytes. Prior BOM-prefixed feeds remain supported.

## Pending updater handoff correction

- Preserve and retire fully validated already-installed staging before discovering later releases; never apply a downgrade.
- Retain fixed helper stages and numeric launch/exit results instead of discarding early handoff failures. Automatic restart remains unverified.

## Pending automatic close routing correction

- Queue a real WM_CLOSE rather than invoking the close handler from the custom update pulse. Preserve cooperative saves and report queue failures numerically without forcing exit.
- Record the real helper parent-exit timeout, supported manual 1.0.27 installation and separate local 1.0.9016 production. Automatic restart after the correction remains unverified.

## Pending synchronous update-close refinement

- Refine the unpublished queued-close candidate to synchronous WM_CLOSE on the checked UI thread, marking closing before dispatch and returning without later window access. Remove its unused unreleased queue-failure API.
- Retain 1.0.9016 as historical and record source-bound local 1.0.9017 production. Automatic restart remains unverified.

## Pending validated updater readiness

- Require complete inherited parent PID, creation-time and executable validation before either normal UI close or early startup exit.
- Retain one helper with zero-timeout UI readiness/exit probes, unnamed exact-handle acknowledgment and cooperative cancellation. Early startup alone may wait up to ten seconds.
- Keep synchronous cooperative WM_CLOSE, same-thread safe UI checks, accepted-main and package/feed integrity, stable-launcher recovery after trust, and bounded attempts.
- Split fixed numeric parent diagnostics into handle/PID, time-query/creation, image-query/path and readiness stages.
- Record later natural 29-to-30 result 13 separately from successful manual recovery; the exact failing identity subcheck remains unproven. Local 9018 production is not runtime acceptance.

## Native first-open setup candidate

The first explicit Open dashboard action now opens a resumable native setup wizard until its four steps are completed. Login and ordinary monitoring launch do not enter setup. Language and message-style controls currently apply to this wizard only. Adapter choices resolve current GUID identities, taskbar changes use an idempotent wrapper around the existing consumer, and startup uses the existing installed/portable registration route. Save and close preserves only the bounded draft. Successful partial operations remain recorded for retry; completion requires saved outcomes.

Source implementation compiled and packaged through the exact root build-installer.bat -Version 1.0.9019 at a500e6c, exit 0 (23.64 seconds, 114 warnings, zero errors). See [production receipt](docs/features/native-dashboard/setup-build.json). Runtime, accessibility, language/theme/DPI and visual acceptance are pending. Broader interface conversion, complete search-builder parity and universal required features remain unfinished. See [first-open setup](docs/features/native-dashboard/setup.md).