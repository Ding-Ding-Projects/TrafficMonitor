# Native searchable context menus

## Scope and architecture

Replace popup presentation at the eight assigned MFC consumers with one shared native popup. Keep each caller's existing HMENU as the command/state source. Send WM_INITMENU before reading it and WM_INITMENUPOPUP for each opened level. Do not change command IDs, plugin callbacks, tray entry points or persisted preferences. Dispatch the selected command only after the popup is destroyed and focus restored.

The popup is an owned, transient native window with a real Unicode edit field, a scrollable owner-drawn native list box and a Back button. This retains native text selection, clipboard and IME composition. Each menu level has its own query and selection. Search is case-insensitive plain-text substring search over that level's labels and shortcuts. Opening a submenu navigates inside the same bounded surface, and Back/Left restores its parent's independent query. Separators are non-actionable. Disabled commands remain visible but cannot execute. Checked, radio and default states are distinct. Menu bitmap icons are retained where provided.

## Material presentation

Use a filled elevated surface, rounded outer corners, 12 px surface padding, 48 px search field, 40 px minimum rows, a dedicated leading check/icon column, right-aligned shortcuts and a trailing submenu chevron. Use Segoe UI, tonal selected rows, contrasting focus outline and subdued disabled text. The normal width is 400 logical pixels, bounded to the monitor work area; height never exceeds its work area. Long lists scroll. Dark mode follows the existing Windows application preference; high contrast uses system colors. Per-monitor DPI determines every metric. No animation is necessary, preserving reduced-motion behavior.

## Input and accessibility

Initial focus is the search field. Up/Down select actionable rows, Enter activates, Right enters a submenu, Left returns, and Escape clears a nonempty query before returning a level or dismissing. Tab moves among search, results and Back. Native text entry retains IME handling. Pointer selection and activation use the same command validation as keyboard. Outside activation dismisses. Restore the prior valid focus window after closing. UI Automation must expose search, results, labels and current selection through native control providers; custom disabled/check/submenu semantics must additionally be exposed before full accessibility is claimed. Runtime automation evidence is required, source inspection is insufficient.

## State inventory and verification tuples

Consumers: TaskBarDlg, DisplayTextSettingDlg, HistoryTrafficCalendarDlg, HistoryTrafficListDlg, NetworkInfoDlg, PluginInfoDlg, PluginManagerDlg, TaskBarSettingsDlg.

States: root, nested submenu, filtered matches, no matches, checked command, radio selection, disabled command, default command, plugin command and empty plugin submenu, long label/shortcut, tall scrolling menu, focus return, outside dismiss, IME composition.

Reference tuples: English, Cantonese and bilingual where provided by the product; light, dark and high contrast; 100%, 125%, 150%, 200%; normal and minimum supported viewport. Anchor at all four work-area corners. Native popup layout and built screenshots remain pending until source-bound runtime verification.

## Explicit unfinished requirements

Full RegexBuddy engine/version parity and the adjacent complete regex workbench are not delivered by plain search. Universal appearance/history/bulk/export requirements are not claimed by this bounded menu change. Full custom UIA semantic proof, runtime keyboard/IME and layout verification remain open until demonstrated. Existing owner-drawn third-party menu callbacks require a compatibility fallback if their labels cannot be read safely.

## Integration handoff

Implementation owns NativeMenus and the eight consumer cpp files only. The integrating owner registers new compilation/header files in the vcxproj and filters, updates shared documentation, and connects broader theme/language preferences. No resource ID or shared NativeMaterial edits are required. A separate reviewer must approve this handoff before implementation.

## Independent review amendments

Independent native-interface reviewer approved this bounded implementation on 2026-10-23 with the following conditions: caller ownership of HMENU; TrackPopupMenu return versus notification semantics and cancel zero; balanced enter/init/uninit/exit notifications; fresh level initialization and command identity/enabled/submenu validation immediately before dispatch; mutation cancels or rebuilds instead of executing a stale row. Focus returns only within the original valid owner without taking foreground from another application. Preserve native edit caret and IME composition keys. Use LBS_HASSTRINGS with explicit accessible names, states and default actions, invalidated with HWND lifetime. Bound width and height to the monitor work area, scale/wrap rows, preserve separate check/icon columns, and respond to DPI changes. Unreadable owner-drawn labels use the original native menu route and remain an explicit visual acceptance gap. Plain search does not satisfy the adjacent complete regex-workbench requirement.
