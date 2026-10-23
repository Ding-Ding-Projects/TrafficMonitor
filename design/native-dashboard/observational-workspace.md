# Observational monitoring workspace: bounded implementation amendment

Status: design-only amendment for independent review. Source baseline: `0656856ffc12699bfa177fc438d29e8943019e89`. No dashboard runtime is implemented by this amendment. The governing P1 design remains `../native-mfc/implementation-unit.md` and `../native-mfc/p1-amendments.md`.

## Productive scope and explicit limits

Implement one real modeless Network workspace, using actual `NativeMonitoring::Snapshot` data and the existing `NativeDashboardController`. It shows adapter discovery, per-adapter receive/send rates, observed sampling state, a bounded history chart and its textual sample equivalent, search, selected-adapter details, display pause/resume and existing History, Plugins and Settings actions. It is not a marketing surface or a generic command form.

This is an observational milestone permitted by the reviewed implementation-unit amendment. It is not completed P1. First-open onboarding, typed adapter selection that changes the monitor's accounting scope, full regex workbenches, the full required-feature inventory, custom Grid/Table/Selection providers, broad localization and complete visual acceptance remain unfinished. Do not ship placeholder buttons claiming those functions exist.

## Entry behavior and compatibility

The existing `ID_SHOW_MAIN_WND` explicit user action opens or focuses the one monitoring workspace owned by the existing main dialog. No initialization, timer, login launch or background sampling path opens it. Existing hidden-main/taskbar-visible/tray defaults and startup registration are unchanged.

The old `OnShowMainWnd` floating-monitor toggle body is preserved as one callback passed to the workspace, not copied into a second implementation. A real **Toggle floating monitor** action executes that callback. Closing the workspace does not toggle or close the floating monitor, change taskbar visibility, stop accounting or exit the application.

The command's visible name must become **Open monitoring workspace**, and its old checked state must no longer represent floating-monitor visibility. This requires coordinated integration beyond the wrapper: the integrating owner updates the exact menu wording/localization and the `ID_SHOW_MAIN_WND` check-state line. Existing resource command ID and all searchable menu call sites stay unchanged. The legacy floating-monitor action remains reachable in the workspace even when its window is hidden.

Creation failure reports a real error and leaves existing monitoring/configuration unchanged. Repeated Open focuses the existing workspace, preserving its selection and pause state. Existing History, Plugins and Settings actions dispatch their actual owner command IDs; no replacement settings implementation is implied.

## Lifetime and threading

- The modeless workspace is owned by the existing main HWND and runs on the same UI thread. It owns its controller and child controls. One UI-thread registry maps that owner to the workspace; destruction removes the exact entry before memory is released.
- The floating-monitor callback is invoked only while the exact owner HWND remains valid. It is destroyed with the workspace. Owner destruction destroys its owned workspace and detaches its controller before any callback can be reused.
- Controller Start uses the existing validated monitor interval, clamped only to the controller's documented 100..60000 ms range. Sampling and address discovery remain exclusively on the existing controller worker.
- Notifications carry its generation value, never a snapshot or C++ pointer. Take validates generation and increasing sequence. The UI retains immutable shared snapshots.
- Close hides immediately and calls Stop. The controller already detaches delivery and releases its outstanding thread-pool work asynchronously; the workspace never waits for discovery, joins on the UI thread or terminates a worker.
- WorkerError and snapshot error are displayed distinctly. No exception becomes a successful empty adapter inventory.

## Data semantics and useful controls

The top rates are labelled **Selected adapter**, not Current monitor scope. Source review shows the legacy aggregate fields are written by the legacy monitoring thread. This unit does not add an unsynchronized read, copy aggregate rates into adapter rows or claim a safe aggregate-copy integration that does not exist.

The table uses each snapshot's alias, description, operational/rate state, receive/send values and observation interval. Missing rates show their state, not zero. The chart uses that selected adapter's actual bounded RatePoint history. Invalid samples and epoch changes break segments. Its textual equivalent lists actual timestamp/relative time, receive, send and state records, with no synthetic interpolation.

Selection binds to persistent GUID identity. A nonpersistent adapter may be viewed for the current snapshot but is never persisted or reselected by a stale row index. A selected adapter that disappears retains the details identity and displays **Adapter no longer present**. Table reorder cannot select another adapter. Filter changes do not change the selected adapter silently.

Plain search matches current alias/description/state and has a real Clear action. Each new dashboard context menu uses the existing shared native searchable menu and current enabled-state validation. The adjacent full regex workbench remains absent and explicitly unfinished, not a false full-parity claim.

Pause freezes the displayed snapshot pointer and visible acquisition time while the controller continues collecting. Resume presents the newest retained snapshot. Refresh restarts only the observational controller, visibly announces that a new first sample is collecting and clears obsolete display history; it never resets traffic accounting or daily records. The distinction is stated next to that action.

Details include actual stable identity, link speeds, addresses and address-acquisition error. Sensitive adapter/address text is excluded from public screenshots unless explicitly reviewed. Existing History opens the actual historical-traffic window, while the new chart's samples are labelled display history rather than accounting history.

## Rendering, layout and accessibility boundary

Use the existing Direct2D/DirectWrite Canvas and Palette for the workspace, metric panels and chart. Use the existing NativeMaterial::Button implementation for working actions. Native Unicode edit behavior preserves IME and text accessibility. A native report list supplies actual adapter rows and a second native report list supplies the textual history equivalent.

The report-list choice is an explicit staged limitation. Stock accessibility is not proof of the reviewed custom Grid/Table/Selection pattern set, and a styled native list is not full component conformance. Do not claim custom providers, full accessibility or full Material Design acceptance until the actual pattern and interaction checks pass. Each control has a meaningful accessible label and keyboard route; the chart has its actual data equivalent rather than inaccessible pixels alone.

Use the existing normal reference size of 1120 by 760 and a minimum client area of 800 by 600 logical pixels. At minimum size, details move below the adapter list and panes scroll where necessary. No labels or action rows disappear to fit. Theme follows the operating-system preference initially, with actual Light/Dark controls; high contrast uses the existing system palette. DPI changes recreate fonts/layout and resize drawing targets. Device-loss redraw uses the existing Canvas result and bounded invalidation.

The first unit may contain English and Traditional Chinese chrome based on the actual language setting. Cantonese/bilingual mode completeness, other translations, broad appearance customization and the full DPI/theme/language matrix remain acceptance gaps. No private wording is bundled.

## Owned files and integration

New runtime files are confined to `TrafficMonitor/NativeDashboard/`:

- `Workspace.h/.cpp`: modeless lifetime, working actions, controller delivery and child ownership.
- `Presentation.h/.cpp`: typed formatting, sampling-state text and chart rendering.
- `ViewModel.h/.cpp`: snapshot selection identity and plain filtering without accounting mutation.

The only proposed main-dialog integration is the include and the `OnShowMainWnd` wrapper around its retained toggle body. The integrating owner separately owns the precise command label/check-state changes and project registration. Shared Theme, Controls, Controller, Snapshot, installers and other main-dialog logic remain untouched by this lane. Existing menu conversion is preserved.

## Decisive verification and pending acceptance

Build through the exact root entrypoint from an immutable candidate. Run on the named off-screen desktop with the same supported portable behavior and process/hash/source binding used for menus. Preserve and restore the matching installed monitor if singleton exclusion requires its temporary graceful close.

Verify actual first sample, subsequent valid rate, paused sequence stability while collection advances, resume, refresh-first-sample state, filtering, stable selection, details/history data correspondence, repeated open, close/reopen with old notifications rejected and owner shutdown. Verify existing History/Plugins/Settings and floating toggle actions and unchanged startup/accounting settings. Injected data or a mock cannot substitute for actual collection evidence; unavailable disconnection/removal/error cases remain unverified.

Inspect normal/minimum rendered surfaces, keyboard access, accessible names/patterns, both themes, high contrast and DPI. Retain every unavailable tuple and the full onboarding/regex/custom-provider requirements in the handoff. This milestone cannot be labelled a completed interface replacement or P1 release.
