# Native dashboard implementation unit for independent review

Status: proposed implementation boundary, not implemented or runtime verified. This supplements `p1-amendments.md`; it does not replace its setup, identity, menu, accessibility or evidence requirements.

## Entry and preservation

Add an explicit Open dashboard command to the existing tray and taskbar entry paths. Login launch and ordinary monitoring retain the taskbar-visible/main-hidden defaults. The existing floating monitor remains available. Opening the dashboard reuses its existing modeless window; closing it stops only its display worker and window, never the monitoring application or accounting. No marketing home or generic command catalogue is introduced.

The first explicit dashboard open runs the already-designed resumable setup flow. Existing preferences initialize the draft. Keep current remains the default for taskbar and login startup. No setup action runs during login launch. Completion opens the productive Network view. The exact existing `GetAutoRun`/`SetAutoRun` and managed installer lifecycle path remain authoritative.

## Productive surface

Network contains current-monitor-scope metrics, a bounded live rate chart, a searchable adapter table, pause/resume display, refresh, connection chooser and adapter details. The table renders actual snapshot alias, status, receive/transmit rates and observation interval. Details show identity, addresses, link speeds and sampling state. Empty, collecting, unavailable, stale, disconnected, removed-adapter and paused states are distinct. No synthetic observations enter production.

The existing aggregate metrics remain labelled Current monitor scope. `NativeMonitoring::NetworkSampler` supplies only per-adapter observations and bounded display history. An owned worker samples at the existing cadence and coalesces immutable `shared_ptr<const Snapshot>` delivery to the UI thread. No worker owns a window or draws. Closing signals cancellation, joins the worker and invalidates pending dispatch through an instance generation. No raw pointer is posted through a window message.

Display pause freezes the selected snapshot pointer while collection continues. Adapter details bind to GUID, not row position. Missing persistent identity is explicitly nonpersistent. An unavailable snapshot retains stale rows with no invented rates. Counter epochs break chart segments. Reordering cannot change the selected adapter. Removed identities display Adapter no longer present.

## Native Material component boundary

Implement reusable C++/MFC controls, not a browser shell and not recolored stock dialogs. The shared layer owns semantic colors, typography, shape, focus rings, state layers, spacing, density and DPI-aware metrics. Native text-input behavior remains backed by a real edit control with explicit Material container, label, supporting/error text and focus treatment. Fonts derive from Windows UI font metrics with a Chinese-capable fallback, not a missing web font.

Initial registered components: filled/outlined/text button; outlined text field; choice control; switch; tab/navigation item; metric tile; chart with textual equivalent; virtualized adapter table; details sheet; status notice; searchable contextual menu. Registration records class, accessible role, supported keyboard actions and visual states. A bespoke control is not accepted solely because its pixels resemble a reference.

Use UI Automation providers for custom interactive surfaces. Buttons expose Invoke; switches Toggle; text fields Value/Text through the backing edit; choices Selection; the adapter table Grid/Table, Selection and row items; details and notices readable names and state; the chart a labelled textual series summary. Focus and selection events follow actual state changes. No invisible duplicate interactive elements. High contrast uses system colors and visible boundaries; focus never relies on color alone.

Tab order follows reading order. Escape returns from a sheet or menu to its origin. Arrow keys navigate rows/options. Enter activates the selected enabled action exactly once. F6 moves between navigation, primary data and details. All accessible names and feedback support English, Cantonese and bilingual modes. System/light/dark themes and 100/125/150/200 percent DPI use the same logical layout metrics.

## Commands and contextual menus

Use typed command descriptors and current target validation for dashboard actions. Extract a typed adapter-selection consumer only after examining the existing `ID_SELETE_CONNECTION` and select-all bodies. Automatic, All and one-GUID choices revalidate discovery epoch and resolve the current MIB interface index at execution. Preserve aggregate counters, history and saved legacy selection fields.

The new dashboard's row, chart and blank-area contexts, plus its tray/taskbar entry menus, require registered Material menu presentation and independent plain-text command search. Nested results retain parent paths, enabled state and origin focus. Search never invokes a command by an old numeric array position. Existing plugin command routing remains intact and plugin-owned external windows remain explicitly external.

The adjacent full regex workbench remains a separate implementation and acceptance obligation. Do not label a simple regex toggle or tester as full parity. Until that work is implemented and verified, report contextual-menu regex parity as unfinished in every scope verdict. This staged unit must not erase that requirement or claim all contextual surfaces converted.

## Proposed file ownership

New implementation files owned by the GUI lane after approval:

- `TrafficMonitor/NativeMaterial/Theme.h/.cpp`: palette, typography and DPI metrics.
- `TrafficMonitor/NativeMaterial/Controls.h/.cpp`: registered control anatomy and interactions.
- `TrafficMonitor/NativeMaterial/Automation.h/.cpp`: custom UI Automation providers.
- `TrafficMonitor/NativeMaterial/CommandMenu.h/.cpp`: descriptors, search and nested menu focus.
- `TrafficMonitor/NativeDashboardDlg.h/.cpp`: modeless productive surface and snapshot presentation.
- `TrafficMonitor/NativeDashboardController.h/.cpp`: owned sampling worker and UI delivery lifecycle.
- `TrafficMonitor/NativeSetupFlow.h/.cpp`: resumable setup draft and verified operation states.

Narrow integration edits need parent-assigned exclusivity before mutation: `TrafficMonitorDlg.h/.cpp`, `TrafficMonitor.vcxproj`, `TrafficMonitor.vcxproj.filters`, `resource.h`, required resource/menu definitions and localization resources. The sampler implementation and installer lifecycle stay outside GUI ownership. Any newly needed shared consumer change is reviewed before widening these paths.

## Order and independent-release boundary

1. Register shared typography/theme/control and accessibility infrastructure; compile through the exact root build entrypoint.
2. Wire the modeless dashboard, safe snapshot delivery and real table/details states. Verify teardown and identity transitions before selection writes.
3. Implement resumable first-open setup and typed selection consumer with exact readback. Preserve disabled startup, taskbar visibility, accounting and existing configuration.
4. Convert the entry and dashboard contextual menus to registered Material search menus; retain the full-workbench gap explicitly.
5. Complete normal/minimum reference parity, keyboard/UIA and source-bound capture coverage. Replace current presentation images only with inspected genuine captures and retain historical originals separately.

The independently releasable product boundary is the productive dashboard plus actual first-open setup and its entry/context interactions, with the existing monitor preserved. Foundation code alone is a recovery checkpoint, not a finished GUI release. Broader Settings/tools, full regex parity and both documentation/build companions remain separate pending milestones.

## Review and verification questions

Independent review must settle the exact snapshot-worker lifetime, owner for typed selection extraction, UI Automation provider interfaces and native rendering technology before implementation starts. Validate the existing P1 reference geometry at normal and minimum sizes, including a real table body and details layout. No source-only result establishes rendered parity.

Decisive checks cover no-accounting-mutation, first-sample/reset/reorder/removal, selection revalidation, close/reopen and queued-delivery safety, setup resume without duplicate effects, disabled-startup preservation, keyboard focus return and each accessible pattern. Parent owns the exact built capture matrix, installer/release regression and public delivery. Any unavailable result remains unverified.

## Independent review decisions accepted for implementation

The rendering backend is HWND-backed Direct2D with DirectWrite. Recreate device-dependent resources after target loss and attempt the Direct2D software target when hardware creation fails. Text input retains native edit behavior for IME and text accessibility.

Custom accessibility uses an explicit UI Automation fragment root and children. Button actions expose Invoke or Toggle. The eventual custom table exposes Grid, Table and Selection; rows expose GridItem, TableItem, SelectionItem and ScrollItem, with Realize when virtualized. The chart must expose its actual time/rate/state samples in an accessible table, not only a decorative summary. COM providers hold invalidatable shared state and never dereference a destroyed HWND or C++ window object.

Snapshot delivery uses a shared latest-const-snapshot mailbox, at most one queued wake, a window-instance generation and monotonic sequence checks. Posted messages carry no pointers. Closing hides immediately and requests stop, then completes an owned asynchronous join. GetAdaptersAddresses is not cancellable, so the UI must not block joining it and must never use TerminateThread. Aggregate values are copied only on their existing owner thread.

Typed selection extraction belongs exclusively to TrafficMonitorDlg. Both legacy and new entry paths use mode, GUID and expected discovery epoch, with fresh GetIfTable2 resolution on the UI thread. Sampling and installer lifecycle implementations are unchanged by this GUI lane.

The first bounded implementation may preserve foundation and observational dashboard work before the entire P1 boundary is finished. Such a checkpoint must list missing setup, custom providers, menu/workbench and parity work explicitly and cannot be labelled a completed P1 release.
