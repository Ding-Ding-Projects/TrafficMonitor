# Signal GUI redesign: handoff

Design source for a full visual redesign of TrafficMonitor, created on 2026-10-30. The canvas can be read back from this folder alone: `canvas.json` is the index and each `.dc.html` file is one artboard. The artboards load a `support.js` runtime supplied by the design tool; it is not part of this repository.

These files are design proposals. They are not product screenshots and do not show verified runtime behavior.

## Artboards

| File | Surface |
| --- | --- |
| `Main.dc.html` | Taskbar strip with the redesigned right-click menu open above it |
| `FloatingMonitor.dc.html` | Floating monitor: compact light, expanded light, expanded dark |
| `Workspace.dc.html` | Network workspace: navigation, metrics, five-minute chart, adapter table |
| `Settings.dc.html` | Settings, Taskbar section, with live preview and action bar |
| `Tokens.dc.html` | Color, type and shape tokens for the native renderer |

## Direction

Signal is a calm instrument-panel look: white or near-black surfaces, one blue accent, tabular figures for every number, and a fixed color pairing for traffic direction. Download is blue and upload is orange on every surface; the two also differ in arrow direction and lightness so color is never the only cue. High contrast mode keeps system colors.

## Tokens

| Role | Light | Dark |
| --- | --- | --- |
| Canvas (`surface`) | #F4F6F8 | #0E1116 |
| Card / menu (`surfaceContainerLow`) | #FFFFFF | #161B22 |
| Navigation (`surfaceContainer`) | #EBEFF3 | #1F2630 |
| Text | #111820 | #E6EDF3 |
| Secondary text | #46525F | #9AA7B4 |
| Control outline | #7A8693 | #7C8896 |
| Divider / menu edge | #C9D2DC | #3A4552 |
| Accent and download | #1D5FD1 | #8EC0FF |
| Selected background | #D6E3FA | #22324A |
| Selected text | #0F2F66 | #DCE9FF |
| Upload | #B4490F | #FDBA74 |
| Positive / error | #2C6E49 / #A3262F | #7DD3A8 / #FF9AA2 |

The control outline is darker than the divider shown on the canvas so input boundaries keep at least 3:1 contrast against white.

Type: Segoe UI Variable; body 14/20, secondary 12/16, section 16/24 semibold, page 24/32 semibold, metric 28/36 semibold. Shape: menu and floating monitor 12, menu rows and fields 8, cards 16, pill buttons 18, navigation items 20.

## Implemented in this change

- Shared `NativeMaterial::Palette` light and dark roles use the Signal tokens, so every component drawn through the shared palette picks them up.
- Snackbar inverse colors follow the new palette.
- Searchable menu: Signal colors, 380 px width, 40 px rows (48 px kept for bilingual labels), rounded selected rows, lighter dividers, selected text color.
- Taskbar right-click menu: the popup is owned by an in-process window when the taskbar strip is embedded in the shell, with a system-menu fallback, so the menu appears.

## Not yet implemented

- Section headings, icons, the tool grid and the keyboard hint footer in the menu. The menu resources have no section metadata yet.
- Floating monitor and taskbar strip defaults (upload and download colors, usage bars, sparkline). Existing saved colors and skins must be preserved, so new defaults need a migration rule first.
- Workspace and Settings layout changes beyond the shared palette.

## Verification status

Not compiled or run on Windows as part of this change. No runtime screenshots exist for the new look. Contrast values above were chosen against WCAG targets but have not been measured on built output.

Earlier design records in `design/native-modern-remake/` stay in place; where their palette or menu dimensions conflict with this folder, this folder is the newer direction.
