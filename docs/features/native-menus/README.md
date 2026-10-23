# Native searchable context menus

The shared native popup reads each caller-owned menu without changing its command IDs, plugin interfaces or saved preferences. Its search is case-insensitive plain text, scoped independently to the current menu level. Back returns to the parent level with its previous query. The original native renderer remains the compatibility route for owner-drawn plugin entries whose labels cannot be retrieved.

The eight integrated consumers are taskbar, display-text settings, calendar history, list history, network details, plugin details, plugin manager, and taskbar style settings. Main-window and tray popup registration belongs to the integrating interface owner.

## Operation

Type into Search menu to narrow the visible commands. Down moves from search into the results. Up and Down select enabled commands; Enter executes; Right opens a submenu; Left from results returns to the parent. Escape first clears search, then returns to the parent, then closes the menu. Edit caret keys and active IME composition remain native. Tab and Shift+Tab move among search, results and Back. Clicking outside dismisses without taking foreground focus back from another application.

The popup sends the existing menu initialization and exit notifications, re-reads item identity and state before execution, and rechecks identity after closing. A stale, replaced or disabled command does not execute. Menu and bitmap ownership remain with the original caller and icon cache.

## Presentation and accessibility

Rows wrap and grow with text. The surface is bounded to the selected monitor work area and long results scroll. Font, spacing and rows scale with DPI. Appearance follows Windows light/dark preference and system high-contrast colors. Search and results use native edit/list controls. A lifetime-aware MSAA provider supplies menu item roles, checked, disabled, default and submenu states and accessible default actions to the Windows UI Automation legacy bridge. Radio items have an explicit description. No matches is a real static text control.

## Verification and limitations

Implementation is preserved, but compilation, runtime keyboard/IME behavior, screen-reader/UI Automation inspection and the complete layout matrix remain pending integration verification. A source-level provider is not proof of a passing UI Automation workflow. The adjacent full regex workbench and RegexBuddy engine/version parity are unfinished. General appearance customization, bilingual mode, and translation of new chrome beyond English/Chinese remain broader product integration work. Native compatibility menus remain a visual conformance gap. No network endpoint is added, so an API/Postman collection is not applicable.
