# TrafficMonitor delivery roadmap

Implementation resumed at the owner's request on 2026-10-26. On the same day the owner chose to
release the current main source, so `.release-hold` was removed. The release does not close the
unfinished items below; this checklist records completed and unfinished work.

- [x] Preserve producer ec177ac and reader 6be9101 on their owning remote branches.
- [x] Publish Settings and preference GDI measurements only after their windows are placed; a hidden-window regression fails when measurements publish before placement.
- [x] Build the combined renderer-observation source and pass every native focused check (layout observations, regex collections, preferences, settings, result list, read-only text, material visuals).
- [x] Republish scroll rail observations after rail-driven viewport scrolls and text-field edits, undo, line scrolling and IME composition.
- [ ] Run a live Settings/preference renderer observation on the built application.
- [x] Publish normal release v1.0.70 from 868ef35 after the maintainer removed the release hold.
- [x] Align documentation inventory checks with the 111-item inventory so documentation and companion builds succeed.
- [x] Add saved-expression descriptions (collection schema 2, schema 1 still readable) and a distinct export success message.
- [x] Check text-field rail republication after line scrolling, replacement and typed line breaks.
- [x] Add the native update ready banner (Restart to install update on choice only, Later), visible manual check results and readable updater state; automatic restart now defaults to off.
- [x] Add opt-in native action sounds (Off, Important, All) through one non-blocking system-sound dispatcher.
- [ ] Apply the action-sound switch immediately instead of on Apply, and add import outcome cues once native personal-wording import exists.
- [ ] Exercise the update banner, Restart flow and action sounds in the built application and record runtime receipts.
- [x] Give the documentation companion and its inner executable the project icon; `build.bat --verify-app-icons` checks all built executables.
- [x] Let a marker-gated diagnostic copy run beside the installed copy so runtime captures use the unmodified build.
- [x] Fix the literal ampersand on the regex workbench Load draft button; a check rejects stray ampersands in Material button labels.
- [x] Measure wrapped Settings text the way the static control breaks it, fixing the clipped Monitoring updater paragraph.
- [x] Add a custom Material title bar (frameless, Snap Layouts, DPI, dark, UI Automation) to Settings, the regex workbench and the setup wizard.
- [ ] Follow design/native-mfc/material-conversion-plan.md until no stock control, system caption, message box, scrollbar or tooltip remains on an owned surface.
- [x] Refresh the README with 76 categorized current captures of the unmodified build and move superseded image references to docs/screenshots-archive.md.
- [x] Replace the 35 legacy system message boxes with a Material alert dialog.
- [x] Add the custom title bar to the network workspace; make the update banner and menu popup frameless Material surfaces.
- [x] Add self-drawn Material switch, checkbox, radio group, segmented button, number stepper, searchable dropdown and labelled text field with clear button.
- [x] Convert the four remaining native message boxes (Settings, workspace, regex workbench) to the alert dialog.
- [x] Adopt the new input components in Settings, the setup wizard and the regex workbench; add a Material slider, label, data table and lists.
- [ ] Convert the legacy upstream dialogs (Options tabs, history, plugins, skins, colors, display items, connections, item order, About, icon select, auto-adapt, Windows 11 taskbar) onto the Material components and retire the legacy Options fallback.
- [ ] Add Material tabs, tooltip, snackbar, progress indicator, chips, and color, font and calendar pickers.
- [ ] Fix capture findings: clipped labels in legacy Options tabs and Change Skin; Settings content stops near x=1068 with an empty right gutter; setup step 3 repeats summary lines; "Apply interface preferences" wraps inside its button; white button backgrounds in the dark regex workbench; untranslated Cantonese menu items; misaligned menu icon baselines and a green focus outline in dark menus; localized accessible name for the banner dismiss button.
- [ ] Retake every README capture after the title bar and component adoption land.
- [ ] Let Rename edit a saved expression's description.

## Incomplete source preservation

- [x] Preserve unfinished native implementation and corrected design references on main with explicit incomplete status.
- [x] Hold automatic product publication while .release-hold exists; retain the publication-language check.
- [ ] Verify the preserved source through the native build, causal responsiveness diagnosis and full acceptance.
- [x] Reconcile the inventory to 111 required identities, including action sounds; 155 validator regressions pass while 1544 product-acceptance gaps remain.

## Workspace responsiveness continuation

- [x] Independently review opt-in numeric instrumentation without claiming a runtime repair.
- [x] Build diagnostic 9044, observe long synchronous measurement work and independently identify the quadratic insertion path. The first run's profile isolation was invalid and remains disclosed separately.
- [x] Repair shared list measurement and verify the actual native control: 120 appends use 240 measurements; a disposable restored quadratic loop uses 7380 and is rejected.
- [x] Build/package local candidate 9045 from c5d0f2a through the exact root entrypoints, with two warnings and zero errors.
- [x] Verify bilingual/dark Apply at 120 history rows in corrected portable 9045, bounded liveness, unchanged normal profile/startup and normal exit. Broader display/accessibility acceptance remains unfinished.
- [x] Repair and verify the demonstrated list-measurement cause with bounded native interaction evidence; full supported display-state acceptance remains separate.
- [ ] Preserve the selected native frame, integrate accepted work and verify release and installed state.

## Native image and preservation closeout

- [x] Retain six inspected, unchanged release1.0.48 native images with exact source/hash/native-state inventory.
- [x] Verify normal fixture exit and restoration of the pre-existing portable installation.
- [ ] Read back the published README and exact image/gallery links for the final preservation revision.
- [x] Finish the bounded workspace causal repair and independently re-review the 122 retained design references; full implementation remains unfinished.
- [x] Recover supported compiler/MFC/SDK22621 acquisition and verify exact native build/package entrypoints.
- [ ] Finish the remaining required-feature runtime acceptance.

## Full native visual remake

- [x] Implement compact shared Interface rows and transient Details; verify 12 language/theme/client-size tuples at 96 DPI, mouse/Enter/Space, minimum scrolling and unchanged normal-profile files in portable candidate 9046.
- [x] Verify Details UIA-client idempotent Expand/Collapse and transition events with the committed native MTA observer on source-bound candidate 9048.
- [x] Implement notification grouping and one setup step viewport; verify save/resume, rail/focus behavior and language/theme layouts at actual 96 DPI in 9047, retaining that revision's two discovered defects.
- [x] Repair Preview Tab handling and disabled-label painting; verify selection, forward focus, all six dark notification tuples, exact threshold/unit persistence and normal exit in 9048.
- [x] Verify actual 960 × 760 setup client size in six language/theme tuples, plus extended forward Tab through both style ranges and the fixed footer.
- [ ] Complete reverse Shift+Tab, legacy accessibility, higher physical DPI, restart persistence and native renderer-specific promotion evidence.
- [ ] Finish the remaining shared component roles/anatomy and owner layouts; the current palette and geometry are not the completed remake.
- [x] Integrate shared color roles and bounded button/selector/check/range/list anatomy; retain focused native checks and the limited 9049 runtime observations.
- [x] Verify the 320 × 400 menu resize repair in 9050: contained Regex, retained filter/clear/submenu/Back behavior, owned Regex cancellation and observed default opening. Original narrow creation and DPI transitions remain unverified.
- [x] Build and independently review the native structural observer with twelve native checks and protected-output/incomplete-receipt assertions.
- [x] Verify one live 9050 structural observation with matching identities, seven controls, no raw text and protected output, while retaining all incomplete-receipt flags.
- [ ] Obtain real renderer-owned font/text/scroll snapshots before claiming native layout or image-promotion acceptance.
- [x] Independently approve the initial five-surface design direction and86 authored references through final offset correction0c93dd2. These are proposals, not runtime images.
- [ ] Complete design references and built conversion for the remaining owned legacy surfaces.
- [x] Independently review bounded Settings/shared-control corrections, actual retained-provider behavior, localized draft preservation and native caption at9032/9033.
- [ ] Verify the exact combined-source checks, package, productive consumers, release and installed update after integration.
- [ ] Replace generic control and scrollbar presentation with shared native components, preserving editing, IME, keyboard and accessibility behavior.
- [ ] Remake workspace, Settings, setup, searchable menus and regex editor with every existing action mapped to a real destination.
- [ ] Convert remaining application-owned dialogs, history/details/plugin/About surfaces and floating/taskbar presentation; the first five owners do not complete the full request.
- [ ] Verify actual built normal/minimum language/theme/DPI states, persistence, recovery and interaction before claiming delivery.
- [ ] Finish documentation-website/wiki/reader work only after the main native application and operational delivery.

## Native-first continuation, release 1.0.43

- [x] Integrate bounded Settings discovery/Notifications and verify exact root checks and installer production at source575.
- [x] Verify normal release43 and one natural matching installed42-to43 update with a prepublication baseline.
- [ ] Complete shared native language/appearance preferences, conflict-aware setup resume and five consumers; source is integrated, complete consumer/runtime acceptance remains pending.
- [ ] Verify real shared-preference interaction, persistence, recovery, accessibility and supported layout matrix.
- [ ] Finish remaining native conversion and applicable required features before resuming documentation-website implementation.

## Native Settings discovery and Notifications slice

- [x] Implement stable setting identities, independent group discovery, temporary result reveal, a real scrolling child viewport and per-row draft actions.
- [x] Retain existing notification consumers, explicit ranges/defaults and unavailable temperature values.
- [x] Observe 46 production validation/binding checks, five deliberate ownership-mutation failures and restored46-pass output through the root entrypoint.
- [x] Package local1.0.9026 at30e3009 and verify bounded real repeated-open, invalid-draft, persistence, row-menu and corrected Chinese behavior.
- [x] Obtain independent source/refutation and actual-pixel review approving bounded partial integration.
- [ ] Verify the exact integrated native build, normal release and matching installed update.
- [ ] Finish all remaining native settings, full regex parity, global languages/themes, UIA/tab order, actual higher-DPI tuples and strict image promotion.


## Required-feature acceptance validator

- [x] Add the independent 110-ID, eight-surface and eight-dimension local inventory validator.
- [x] Separate diagnostic incomplete output from strict acceptance; preserve 1499 baseline acceptance gaps.
- [x] Reject malformed PNG bytes and reversed optional palette/transparency order; preserve baseline red and final 153-check green evidence.
- [x] Complete independent post-repair review, including six final focused cases at e647d839d30887fb786cb8fd53068348b8f774f0.
- [x] Verify integrated validator source publication at c79d49dbee6467d591676b575fb6b2dd615224a6 and153 local integrated regressions.
- [ ] Complete full product implementation, genuine execution/provenance and complete interaction evidence.


## Build acquisition recovery, 2026-10-24

- [x] Reconcile all 111 required identities, add the unfinished action-sound obligation and verify 155 inventory regressions; 1544 acceptance gaps remain.
- [x] Complete independent representative review of corrected design references: 122 structural entries and ten inspected design-only rasters; native implementation and full runtime acceptance remain pending.

- [x] Separate strict readiness from incomplete-instance recovery and preserve missing-directory registrations through an explicit project-owned installation destination; focused acquisition regression passed.
- [x] Verify native consent, component acquisition and exact root build completion for recovery source bee8cb6 on the current host; 114 warnings and zero errors.
- [x] Produce local diagnostic candidate 9044 through the exact installer entrypoint and record hashes; package build had two warnings and zero errors.
- [ ] Establish the workspace responsiveness cause and verify its repair; candidate 9044 is build evidence only.

## Current delivery priority, 2026-10-23

- [ ] Complete the main native C++/MFC application first: reusable Material components, productive monitoring workflows, onboarding, complete Settings, context menus, search and independent full regex builders.
- [ ] Verify native consumers, saved choices, recovery, language modes, keyboard/accessibility, themes and supported DPI layouts using source-bound builds and actual interaction.
- [ ] Finish native installer, silent update and accepted installed-version verification, plus the dedicated build companion and operational release work.
- [ ] Only after the main application is complete, finish the hosted documentation, wiki, offline documentation companion, remaining documentation controls and current image matrix.

Existing delivery runs may finish. Do not start more documentation-website implementation while main-application acceptance remains unfinished. Earlier receipts retain their original scope and source identity. Release-grade verification is active; older expedited-mode statements below are historical, not current restrictions.

## Integrated natural35 evidence

- [x] Preserve reviewed source-bound natural 1.0.34 to 1.0.35 acceptance and all four release asset byte/hash records.
- [x] Record integrated source cb0add local root installer production 1.0.9023 separately from normal 1.0.35 release.
- [x] Publish/read back natural35 documentation at main f795 and wiki master07ffa with exact stored article bytes; live wiki interaction remains unverified.

## Preparation reliability candidate

- [x] Separate cancelled/expired preparation and helper creation failure from durable actual-apply attempts.
- [x] Require complete staging trust before readiness and retain post-exit revalidation.
- [x] Observe focused baseline regressions red and repaired local helper/budget checks green.
- [x] Produce exact root unsigned installer candidate1.0.9020.
- [x] Accept installed repair34 through one genuine natural 1.0.34 to 1.0.35 update, exact released bytes and saved-data/startup preservation.
- [ ] Complete production negative paths, full GUI acceptance and broader release-grade verification.
- [ ] Complete separately coordinated preservation/archive and scoped cleanup.

## Readiness release delivery

- [x] Verify normal 1.0.31 tag, source, complete downloaded asset inventory and hashes.
- [x] Manually apply accepted 1.0.31 through supported Squirrel update, preserving saved data and startup; confirm installed hash and hidden-main/taskbar-visible metadata.
- [x] Read back readiness wiki master 0b1263c8b709f420b1e293850a075e09e9342750 and exact prepared article bytes.
- [x] Observe one genuine source-bound natural31-to32 transition, exact released bytes, readiness/launcher/apply results0 and saved data/startup preservation. Broader negative and repeated paths remain unverified.
- [x] Publish and read back installed31 amendment at wiki master9689e37ed3926f512c4752bbda2bd24c89a659e9, exact source3a guide bytes.
- [ ] Publish and read back this natural32 acceptance amendment.

## Installer and release foundation

- [x] Create the organization fork, preserve upstream history, and establish `main`.
- [x] Add reproducible x64 build and installer entrypoints.
- [x] Package a genuine unsigned Squirrel.Windows installer with required runtime dependencies.
- [x] Enable current-user login startup on fresh install through the stable launcher.
- [x] Default installed taskbar display to visible and main window to hidden.
- [x] Preserve explicit choices and data across updates; remove only owned startup registration.
- [x] Verify installer, update, runtime display, and release download behavior.
- [x] Publish and verify the first customized normal release.

Releases 1.0.3 and 1.0.4 passed current-host installation and update checks. The current taskbar
monitor is attached to Explorer with its main window hidden. Subsequent sign-in, Explorer restart,
fresh-host prerequisite acquisition and cancellation remain separately unverified.
Local publication hooks and the hosted publication job pass. The strict `publication-language`
required check is configured; administrators retain bypass permission.

## Native interface and required features, after the first delivery

- [ ] Deliver independent complete regex builders beside every owning search. Local worker and first documentation-search interaction are verified; full parity and remaining owners are unfinished.
- [ ] Expose every applicable Settings control with actual consumers, persistence, reset and recovery. The complete mapping is not implementation proof.
- [ ] Verify modern task-led documentation and capability discovery on the deployed site, including narrow layouts, keyboard behavior and saved state.

- [ ] Design and review a native MFC Material Design 3 component system.
- [ ] Convert the complete owned interface while preserving monitoring and plugins.
- [x] Inventory every required feature and its applicability by delivered surface.
- [ ] Implement and verify all applicable features, documentation, and companions.
- [ ] Publish complete documentation and genuine current build evidence.

Unchecked items are unfinished; source implementation alone does not establish runtime acceptance.

The [110-item inventory](docs/features/required-features/README.md) records implementation and
proof separately. Context menus require the new visual system, action search and a full regex
workbench. Current presentation screenshots must be replaced with genuine rebuilt evidence,
with historical originals retained separately.

## Documentation and current-host milestones

- [x] Deploy the documentation home and verify exact About homepage readback.
- [x] Translate all 1,387 upstream history entries and retain complete fork messages.
- [x] Publish five prepared fork wiki pages and retain their source.
- [x] Verify current-host native build dependency acquisition and accepted 1.0.12 update.
- [x] Repair and repeat the settings page language/theme/scale overflow matrix.
- [ ] Complete keyboard, accessibility and remaining documentation interaction acceptance.
- [ ] Finish native components, live dashboard, setup, context menus and every remaining surface.


## Native network workspace candidate

- [x] Compile and package the observational native workspace locally at 7e560bf; build-installer.bat -Version 1.0.9001 exited 0. This is build proof only.
- [ ] Integrate and publish the native workspace through the normal release route.
- [ ] Verify the new workspace at runtime when testing is enabled again; no new tests, reviews or screenshots run in the current expedited mode.
- [ ] Complete custom UIA table behavior, full language modes, adjacent full regex workbench and practical onboarding. The partial workspace does not close these requirements.


## Native task-oriented settings

- [x] Compile and package the task-oriented settings editor at 93b32abc; build-installer.bat -Version 1.0.9002 exited 0. This is build proof only.
- [ ] Deliver and verify its draft/apply/revert/reset, persistence and recovery behavior when runtime checks resume.
- [ ] Finish remaining legacy settings, full adjacent regex, language modes and accessibility acceptance.


## Silent installed updater candidate, 2026-10-23

- [x] Implement bounded native background checks at five-minute intervals and verified full-package staging.
- [x] Add cooperative UI readiness, identity-bound process handoff and next-launch application.
- [x] Preserve explicit update opt-out and provide persisted automatic-restart preference with save failure reporting.
- [x] Compile and package combined source 8f9812966be9de46b2262df55d44e71517d527bc through the exact root entrypoints, local-only version 1.0.9003, both exit 0.
- [ ] Verify update, failure, cancellation, recovery, draft deferral and data preservation when runtime checks are authorized again.

- [x] Restrict accepted-channel admission to release commits contained in an exact resolved main SHA, retain bounded offline records, and compile/package source 3e6f8c72fe40a363de70c7ee5e27772ca86b9c6b as local-only 1.0.9005, exit 0.

## Native regex staged unit

- [ ] Verify native workspace, Settings and menu pattern Apply/Cancel, invalid input, cancellation and independent state. Source implementation added; runtime verification deferred.
- [ ] Complete every topic in the native RegexBuddy parity ledger, including captures, engine comparison, debugger, saved collections, GREP and automation.
- [ ] Verify anchored modeless presentation, bilingual mode, accessibility, minimum layouts and the complete DPI/theme/language matrix.

## Silent update documentation publication

- [x] Refresh the current 1.0.19 supported-update baseline without claiming automatic timing/restart proof.
- [x] Prepare complete matching silent-update wiki content and navigation while preserving existing guides.
- [x] Publish six wiki guides and navigation at 3d7cfa6917e08d5b387add90c09c187830116cb7; coordinating-task exact stored blob readback matched. Live UI verification remains unverified.
- [x] Build the refreshed inventory through root build.bat --site at 69a6ab51ab1b63d513de27be1efd2cbbfb7b6ef8: 191 articles, 1542 history records, zero unavailable images, exit 0.
- [ ] Publish the refreshed documentation deployment through its existing same-run release metadata binding and verify source-bound live articles.
- [ ] Verify automatic updater runtime behavior when runtime checks are authorized again.

- [ ] Verify the new owned modeless regex editor across workspace, Settings and menu owners. Source implemented; runtime, focus, cancellation and display-matrix acceptance remain unverified.

## Silent updater feed repair

- [x] Compile and package uppercase Squirrel SHA-1 compatibility and bounded local status diagnostics at f63e8a456befcd2f6d726712182f6e2a01e9cc11 as local-only 1.0.9007, exit 0.
- [ ] Observe automatic update behavior, modal/draft deferral and recovery when runtime verification is authorized.

## Named native expression collections

- [ ] Verify explicit save/reuse/rename/delete/recovery, Unicode persistence, concurrency, keyboard access and localization in the built editor. Source implemented; runtime acceptance remains unverified.
- [x] Add bounded collection export and all-or-nothing merge/replace import with `build.bat --verify-regex-collections` (31 checks, mutation-checked).
- [x] Add Export collection and Import collection (Merge or confirmed Replace) to the regex workbench Library page on the existing worker; source and build verified, runtime interaction not yet exercised.
- [ ] Exercise workbench export/import in the built application, give export its own success message, then complete descriptions, parameters and remaining RegexBuddy library topics.

## UTF-8 updater feed marker repair

- [x] Compile/package leading UTF-8 feed marker compatibility and fixed integrity substages at 6464841107c5b2bc43f15040c3108057ed296caf as local-only 1.0.9012, exit 0.
- [ ] Verify automatic update behavior when runtime checks are authorized.

- [x] Produce local 1.0.9013 at 58ff5b8226a9da38307a1b3ec3956e0a4ed35119 with explicit BOM-free UTF-8 RELEASES encoding and production readback, exit 0.

## Updater handoff correction

- [x] Produce the stale-staging correction and bounded helper diagnostics through the supported installer entrypoint (1.0.9015, compile/package only).
- [ ] Verify automatic handoff, helper loading, safe restart and recovery in an authorized runtime acceptance pass.

## Automatic close-message correction

- [x] Correct the observed pulse/default-close message mismatch and produce native package 1.0.9016.
- [x] Observe one real accepted automatic update after the final synchronous correction: 1.0.28 to 1.0.29, exact bytes, helper completion, relaunch and preservation.

## Synchronous automatic-close candidate

- [x] Refine close dispatch to synchronous WM_CLOSE and produce local native 1.0.9017.
- [x] Observe automatic handoff and relaunch for the real accepted 1.0.28 to 1.0.29 transition.

## Remaining automatic update acceptance

- [ ] Verify offline/corrupt input/cancellation/rollback paths in an authorized acceptance pass.
- [ ] Verify busy editor/modal/draft/menu deferral, visual/accessibility/localization and multiple future cadence cycles.

## Validated helper readiness candidate

- [x] Implement complete original-identity acknowledgment before parent exit, nonblocking normal UI polling and bounded early-startup preparation.
- [x] Produce local 1.0.9018 through the exact root installer/build entrypoints, with source-bound receipt.
- [x] Restore matching installed 1.0.30 manually after the natural result13 regression, preserving saved data and startup.
- [ ] Observe the readiness correction in a later actual accepted natural transition; compilation does not discharge this item.
- [ ] Verify cancellation, timeout, busy/modal/draft deferral and other unrun negative modes when authorized.

## Native first-open setup

- [x] Compile and package the native first-open setup source candidate at a500e6c through the exact root entrypoints, local version 1.0.9019, exit 0.
- [ ] Verify first explicit open, resume, save-only close, changed/removed adapters, partial retries and actual startup/taskbar readback.
- [ ] Verify English, Cantonese, bilingual, independent message-style levels, keyboard/accessibility and theme/DPI layouts from the built executable.
- [ ] Complete remaining interface and search-builder obligations; this bounded wizard does not establish broad parity.

## Offline documentation companion

- [x] Build candidate c4231a8 complete article/wiki inventory into a single portable documentation executable and verify production hashes (199 articles, 337 bundled files; later revisions require their own receipt).
- [x] Verify bounded real offline one-file launch, internal routes, search and selected preference persistence for source421/candidate9024.
- [ ] Complete higher scales, full keyboard/accessibility, all images and exact integrated release verification.
- [ ] Publish and read back the companion assets on the native source-bound normal release.

## Remaining generic native controls

- [x] Correct and independently review null-foreground menu regex lifetime with red9034/green9035 bounded observations.
- [x] Verify normal1.0.46 release and matching installed executable, saved settings, startup and history.
- [x] Implement and independently review the workbench visual/native-control replacement, with current9040 consumer observations.
- [ ] Complete accessibility/provider, physical reverse traversal and the full language/theme/DPI acceptance of the replacement workbench.
- [ ] Convert workspace/setup and every remaining owned legacy surface, then verify full language/theme/DPI and keyboard behavior.
