# Native-first continuation

## Resumed work

The owner resumed implementation on 2026-10-26. `.release-hold` stays in place and
installation remains deferred.

Measurement-before-placement is corrected. Settings, the shared preference editor and its
action footer now collect GDI measurements in a `MeasurementBatch` and publish them only
after the measured windows are placed. Publication rereads registration, state and client
geometry; a DPI change since measurement withholds the record so it stays incomplete. The
reader's `renderer-client-geometry-changed` check is unchanged. `build.bat
--verify-layout-observations` adds three hidden-window cases (nothing published before
placement, post-placement client geometry, measuring font retained). Restoring immediate
publication fails two of them. The full `build.bat --silent` product build, `--verify-preferences` (23 passed) and
`--verify-settings` (46 passed) also succeed at this source. A live Settings/preference
observation run has not been performed yet, so native layout acceptance remains unavailable.

Regex collections gained `ExportCollection` and an all-or-nothing `ImportCollection`
(merge skips identical records and rejects a differing case-insensitive name match; replace
swaps the set). Imports keep the `.bak` used by Recover. `build.bat
--verify-regex-collections` runs 31 checks and a built-in mutant that must fail. The regex
workbench does not expose these actions yet. The merged source passes the full product build.

Individual branches retain their earlier build/check results, including 320 old/new drawing
comparisons and transport-v2 checks. They do not verify the stopped combined source. The last
bounded product runtime result remains candidate 9050 below. Task worktrees and private
evidence are retained; no destructive cleanup was requested or performed.

Latest bounded candidate: 9050 at `965241da6501b9d188809202d483b624d5987212` verifies the
repaired 320 × 400 menu at 96 DPI. Regex is at x=220..308, all visible controls are contained,
filter/clear retains size and selection, submenu/Back works and owned Regex cancellation
retains the menu. Four private images and the original failed 9049 tuple remain separate.
The structural observer completed one live run with matching identities, seven controls,
no raw text and protected output. Its complete-receipt/v2/v3 flags all remain false.
See docs/evidence/menu-reflow-9050.json and native-components-9049.json. Normal exit preserved
five profile hashes/startup and released processes, mutex and desktop. Nothing was installed
or newly released. Original narrow creation, higher DPI and renderer evidence stay pending.

The earlier bounded native source is `337903e3bf08bf068d7a5dfc6471419150113ce1`, packaged locally as 1.0.9048. Product publication remains held by `.release-hold`. The owner permits normal closure of task-targeted TrafficMonitor instances and explicitly defers installation. Continue with isolated portable fixtures. Complete the native C++/MFC application before operational release acceptance, then finish hosted documentation, wiki and companions last. Keep native caption controls and preserve monitoring, plugins, saved preferences/history and startup.

## Current bounded result

- Shared-list source `c5d0f2a` replaces quadratic insertion measurement. The actual HWND regression needs 240 measurements for 120 appends; restoring the old loop produces 7380 and is rejected. Portable 9045 completed bilingual/dark Apply with 120 history rows and 50 adapters. Workspace refreshes took 632.5234 and 590.9424 ms; sampled liveness remained below one second. See [the source-bound receipt](docs/evidence/list-responsiveness-9045.json).
- Compact shared Interface source `cdd0e7c` has measured language/appearance rows, a small selectable preview and transient contextual Details. Portable 9046 covered 12 language/theme/size tuples at 96 DPI and mouse/Enter/Space disclosure. Its earlier unavailable UIA-client result remains historical. See [9046 evidence](docs/evidence/compact-interface-9046.json).
- Combined source `fd0a87a`, candidate 9047, groups all six Notifications and places setup preferences/actions/style ranges in one body with a fixed footer. Actual unseeded first open, style 2/3 save/resume, Back/Save and close, Keep-current completion and unapplied-preference preservation passed. Both surfaces covered 12 language/theme/size tuples at actual 96 DPI. The original 60 private images retain their original source and two discovered defects.
- Candidate 9048 fixes Preview Tab handling and disabled-label painting. Double-click selection remains native; Tab reaches Use saved. Six dark notification tuples show single muted label strokes. Disabled 32767 GB was applied and read back before restoration, zero left configuration bytes unchanged, Revert recovered 32767 GB and the fixture returned to disabled 200 MB. Four unavailable temperature values remained 80, with explanations and Details available.
- A fresh 9048 follow-up covers the declared 960 × 760 Setup client size in all six language/theme tuples. English and Cantonese fit without overflow; bilingual reaches both style ranges and scope text through the rail. Actual forward Tab traverses Preview, Use saved, Reset, both ranges, Next and Save and close in English/light and bilingual/dark. Each focused body rectangle is fully revealed. Reverse Shift+Tab is still unverified.
- The committed native MTA observer builds through `build.bat --uia-observer`. On source-bound 9048, Details 56206 exposes working ExpandCollapse, repeated Expand/Collapse is idempotent, and transition events are observed. This does not establish every provider, screen reader or legacy accessibility path.

The [combined 9047/9048 receipt](docs/evidence/native-surfaces-9048.json) records source/package/executable hashes and exact limits. All corrected fixtures preserved five normal-profile file hashes and startup absence, exited normally, released their mutexes and closed their empty hidden desktops. No candidate was installed or published. The 9047 matrix is not relabelled as 9048 evidence.

## Active next work

1. Add renderer-owned font/glyph/text-area/custom-scroll and paint-sequence snapshots, starting with a bounded native Interface state, and combine them with the verified external observer. Keep missing data explicitly incomplete. Remaining owner geometry and typography stay unfinished.
2. Complete remaining workspace, menu, regex and application-owned legacy surfaces with real actions. Preserve owner identities, drafts, native editing/IME and accessibility consumers.
3. Obtain complete native renderer-specific font/glyph/text-area and scroll observations. Raw images remain private because native layout v2 and promotion v3 receipts are incomplete. No zero-findings or finished-remake claim is justified.
4. Verify physical 125/150/200 percent scales, reverse traversal, high contrast, reduced motion, full keyboard/IME/accessibility and restart/recovery. Resizing at 96 DPI is not physical-scale evidence.
5. Complete the applicable required-feature inventory, installer/updater negative paths and release/build companions. Installation remains deferred by current owner direction. Remove `.release-hold` only after the required acceptance and permitted delivery actions are complete.
6. Complete hosted documentation, full wiki content and documentation-companion delivery last. Existing publication and historical evidence remain preserved.

## Verification and tooling

Build recovery `bee8cb6` distinguishes strict 2022 compiler/MFC/SDK22621 readiness from incomplete registration discovery. Both previous registered directories were absent. A supported installation at a TrafficMonitor-owned destination restored the exact root build and packaging route without deleting unrelated registrations. Historical bootstrapper self-update success was not component readiness.

Focused verification includes 23 preference cases, 24 setup-serialization cases and expected mutation rejection; 46 settings validation/binding cases with five rejected ownership mutations; and a real read-only text HWND regression proving Tab flags change from 0x8d to 0x89 while ordinary key semantics and selection remain. The integrated `build.bat --verify-readonly-text` passed. Compilation is separate from runtime acceptance.

The feature inventory contains 111 identities. The added action-sound identity stays unfinished on applicable surfaces. All 155 inventory validator regressions passed with 1544 acceptance gaps retained. Independent design review verified 122 SVG references and inspected ten representative renders; these remain design proposals, not product images.

Use `scripts/prepare-portable-diagnostic.ps1` for fresh ignored fixtures. Its 15 synthetic checks and independent review cover hash/path/archive rejection, omission of managed markers/updaters and consistent private baseline preparation. It never launches or proves source/package association by itself. Stop on preparation failure and use a separate immutable production receipt. Print only allowlisted receipt fields, never encoded baseline contents.

## Data-preservation incident

The first 9044 fixture mistakenly retained `installed-mode.ini`, which overrode its portable-mode file and caused writes to normal application data. No installer ran. Four current files were preserved before normal exit and five afterward, with matching live hashes. No prelaunch baseline exists, so restoration is unverified and no guessed restoration was made. Preserve those private snapshots and original evidence. Later unchanged-baseline checks do not prove restoration of the earlier state.

The 9044, 9045, 9046, 9047 and both 9048 fixtures are closed. Revalidate process identities before any future operation. A temporary task-owned loopback Lowlevel service remains available for the ongoing goal; recorded PIDs are historical and never reusable authority. Do not touch the visible desktop or unrelated processes.

## Delivery and remaining preservation

The accepted normal release remains `v1.0.48-run40021045002-1` at `ddcf441f4c3cab2a5e3b62e9ff6aff1babf3bdfa`. Six unchanged before-state images and their original provenance remain under `evidence/ui/native-release48/`. They do not depict the completed requested redesign. Local 904x packages are diagnostic output, not releases.

All completed native source above is on main and was pushed with exact remote readback. The 337903e publication-language run 40147875461 succeeded; product release and documentation deployment were skipped under the hold. Preserve active task branches/worktrees and private images. Cleanup is deferred while the native goal remains active and ownership/preservation gates are incomplete.

The corresponding shared-instruction synchronization remains incomplete because its primary checkout has unrelated local changes and was 31 commits behind its fetched main. Do not overwrite that work or force a push. Reuse issue 1 and Discussion 2 for scoped progress. Earlier Status Hub/Project limitations remain unverified until a supported live check succeeds. Account allowance is unknown; no fixed token budget was selected.
