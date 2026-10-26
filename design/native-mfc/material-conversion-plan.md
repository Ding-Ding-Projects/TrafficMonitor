# Material Design 3 conversion plan

Goal: every owned TrafficMonitor window uses Material Design 3 components and a custom title bar.
No stock Windows control, system caption, system scrollbar, system message box or system tooltip
remains on an owned surface. This plan records the inventory taken on 2026-10-26 at source
`fe1804f` and the agreed conversion order. Line numbers refer to that source.

## Inventory summary

| Area | Count at fe1804f | Status |
| --- | --- | --- |
| Newer native top-level windows (Settings editor, setup wizard, workspace, regex workbench, update banner, searchable menu popup) | 6 | All use the system caption or system border |
| Legacy upstream dialog templates in `TrafficMonitor.rc` | 22 (17 top-level, 5 tab pages) | None converted |
| Stock controls in legacy templates | 62 push buttons, 48 checkboxes, 16 radio buttons, 20 edits, 22 combo boxes, 7 list views, 3 list boxes, 2 tab controls, 18 group boxes, 15 up-down controls, 85 static labels | Pending |
| System message box call sites | 39 (35 legacy, 4 native) | Pending |
| Stock tooltip controls | 8 | Pending |
| Common dialogs | `CFontDialog` x2, `CMFCColorDialogEx` x6, `IFileDialog` x2, `CFileDialog` x1 | Pending |
| Popup menus | 7 `IDR_*` menus, all routed through `NativeMenus::Track` except two `TrackPopupMenu` fallbacks | Mostly converted |

Surfaces outside the application's control, recorded as boundaries rather than gaps:

- Plugin option dialogs (`ShowOptionsDialog`) are drawn by plugin DLLs.
- The notification-area balloon (`NIF_INFO`) is drawn by the Windows shell.

The skin-drawn floating monitor and taskbar windows contain no stock controls; their stock
tooltips are covered by the tooltip step below.

## Existing Material components

`Button`, `TextField`, `ScrollableTextField`, `ReadOnlyText`, `SearchSelector`, `ScrollRail`,
`ScrollViewport`, `Panel`, `ResultList`, `SettingsCheck`, `SettingsNumber`, `Choice`,
`RangeValue`, `NativeMenus::Track`. `Choice`, `RangeValue`, `SettingsCheck`, `TextField` and
`ResultList` still sit on stock window classes and are replaced by the components below.

## Conversion order

1. Window shell: custom Material title bar and a dialog base for all top-level windows.
2. Material alert dialog replacing every system message box.
3. Core inputs: switch, checkbox, radio group, segmented button, number stepper, searchable
   dropdown, labelled text field with clear button.
4. Native Settings, setup wizard and regex workbench moved fully onto those components.
5. Data table, checkable reorder list and single-select list (history, plugins, colors, display
   items, connections, item order, skins, menu list).
6. Pickers: color, font, calendar, image preview; decision on file pickers.
7. Tabs and a Material tooltip, which retire the legacy Options and History tab dialogs.
8. Snackbar, progress indicator and status chips.
9. Remove the legacy Options fallback once steps 3 to 7 cover every setting.

Each step is complete only with built-application interaction evidence and current captures.
