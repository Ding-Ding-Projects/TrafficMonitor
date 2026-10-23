# Menu state and lifecycle inventory

| Consumer | Menu source | Caller preparation | Initialization behavior |
| --- | --- | --- | --- |
| TaskBarDlg | taskbar or plugin root | Plugin mouse callback may consume click; dynamic plugin name/icon and commands | Checked main-window state, disabled update/network commands, default double-click action, plugin checks; notifies main window |
| DisplayTextSettingDlg | submenu 0 | Selected list row | Restore-default disabled without selection |
| HistoryTrafficCalendarDlg | submenu 1 | Anchor beneath menu button | Sunday/Monday radio selection |
| HistoryTrafficListDlg | submenu 0 | Header column hit test | Linear/logarithmic radio selection |
| NetworkInfoDlg | submenu 0 | Selected list row | Existing resource state |
| PluginInfoDlg | submenu 0 | Selected list row | Existing resource state |
| PluginManagerDlg | submenu 0 | Selected plugin, default details command, regenerated dynamic commands | Details/options/disable enabled state and checked disabled-plugin state |
| TaskBarSettingsDlg | default-style root | Anchor beneath default-style button | Existing generated style entries |
| TrafficMonitorDlg (integration owner) | main or plugin root | Plugin callback, dynamic plugin label/icon, regenerated commands | Connection state, main/taskbar visibility, configuration and plugin checks |
| TrafficMonitorDlg tray entry (integration owner) | main submenu 0 | Tray activation point | Same main initialization |

The taskbar and main dialogs both track menu-open state and clear it in WM_EXITMENULOOP. A replacement must send WM_ENTERMENULOOP, WM_INITMENU, each WM_INITMENUPOPUP, WM_UNINITMENUPOPUP and WM_EXITMENULOOP in a balanced lifecycle, including cancel and window-creation failure. Dispatch commands only after exit notification. Plugin menu bitmaps use MIIM_BITMAP and remain owned by the existing icon cache. Never delete those bitmaps.

Plugin command IDs remain ID_PLUGIN_COMMAND_START plus index. The existing resource loader and caller remain responsible for localization. Search normalization removes mnemonic ampersands, preserves escaped ampersands and includes the tab-separated keyboard shortcut. Do not invent accelerators. In-place menu modifications require fresh item state validation before execution.

## Accessibility acceptance inventory

Search: Edit control, accessible name, native Value/Text patterns and IME. Results: selection/focus events, item text, MenuItem role, enabled/disabled, checked/radio and default state. Submenus: ExpandCollapse semantics with a visible Back route; commands: Invoke semantics. Separators: nonselectable. The native list-box provider alone does not prove the custom checked, disabled or submenu semantics. A semantic bridge or dedicated provider plus real UI Automation inspection is required. Platform provider availability is not runtime evidence.

## Review considerations

A popup-local nested message loop must avoid passing its navigation keys through the owner dialog's PreTranslateMessage, which intentionally swallows Escape/Enter. It must continue dispatching unrelated messages, retain WM_QUIT, handle owner destruction, dismiss on outside activation, and not disable unrelated windows. Focus restoration must not steal focus back from another application after outside dismissal.
