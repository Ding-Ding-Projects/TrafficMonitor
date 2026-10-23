# Native searchable context menus

The shared native popup reads each caller-owned menu without changing its command IDs, plugin interfaces or saved preferences. Its search is case-insensitive plain text, scoped independently to the current menu level. Back returns to the parent level with its previous query. The original native renderer remains the compatibility route for owner-drawn plugin entries whose labels cannot be retrieved.

The eight integrated consumers are taskbar, display-text settings, calendar history, list history, network details, plugin details, plugin manager, and taskbar style settings. Main-window and tray popup call sites use the same presenter, preserving the original command and menu ownership.

## Operation

Type into Search menu to narrow the visible commands. Down moves from search into the results. Up and Down select enabled commands; Enter executes; Right opens a submenu; Left from results returns to the parent. Escape first clears search, then returns to the parent, then closes the menu. Edit caret keys and active IME composition remain native. Tab and Shift+Tab move among search, results and Back. Clicking outside dismisses without taking foreground focus back from another application.

The popup sends the existing menu initialization and exit notifications, re-reads item identity and state before execution, and rechecks identity after closing. A stale, replaced or disabled command does not execute. Menu and bitmap ownership remain with the original caller and icon cache.

## Presentation and accessibility

Rows wrap and grow with text. The surface is bounded to the selected monitor work area and long results scroll. Font, spacing and rows scale with DPI. Appearance follows Windows light/dark preference and system high-contrast colors. Search and results use native edit/list controls. A lifetime-aware MSAA provider supplies menu item roles, checked, disabled, default and submenu states and accessible default actions to the Windows UI Automation legacy bridge. Radio items have an explicit description. No matches is a real static text control.

## Verification and limitations

Compilation has passed. A bounded real Plugin Manager popup check covers plain search, no-match, Escape, disabled Enter and agreement between direct IAccessible and IDispatch role/state reads. See [runtime verification](runtime-verification.md). Enabled direct and dispatched actions and nested opacity navigation were also exercised on the main popup. Remaining keyboard/IME behavior, screen-reader/UI Automation inspection and the complete layout matrix remain pending integration verification. A source-level provider is not proof of a passing UI Automation workflow. The adjacent full regex workbench and RegexBuddy engine/version parity are unfinished. General appearance customization, bilingual mode, and translation of new chrome beyond English/Chinese remain broader product integration work. Native compatibility menus remain a visual conformance gap. No network endpoint is added, so an API/Postman collection is not applicable.

## Build receipt

The exact root `build.bat --silent` passed for source `a74b441b08acbfd22480653f0a085fcab8ad0b83`, with zero errors and two existing `LNK4199` unused WinRT delay-load warnings. See `build-verification.json` for the executable SHA-256. Subsequent project-filter/encoding normalization changes no compilation item or runtime source. Runtime acceptance remains pending.

## Required adversarial runtime checks

1. Obtain the results provider through both direct `IAccessible` and `IDispatch`. Compare menu roles, checked/default/disabled/submenu state, shortcut and default-action text. Dispatch must execute the same validated command as direct calls.
2. Queue an accessible command, change list selection before delivery, and verify the originally requested command executes, never the new selection. Queue then filter, change level, disable/remove/replace the command, or close the popup: stale actions must execute nothing. Force a failed message post: the accessibility call must return failure.
3. Repeat each check against an intentionally reverted queue or native-dispatch implementation in an isolated test build. The check must fail before the repaired build is accepted. No such negative runtime test has run yet.
4. Exercise plugin dynamic commands, disabled commands, radio/check/default states, Back, Escape, Up/Down/Left/Right, IME composition, outside dismissal and focus return on the genuine built surface. Check work-area corners and the complete DPI/theme/language matrix in the design inventory.

