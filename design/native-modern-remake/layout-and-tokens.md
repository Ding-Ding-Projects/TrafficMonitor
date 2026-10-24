# Layout and component specification

Current visual refinement: [release 48 refinement](release48-refinement.md), based on source `eae4d77243e53cc25df57ccb9ef6f9a665a33562`. Its visual decisions supersede conflicting earlier dimensions below; historical evidence and behavior contracts remain unchanged. Independent review and native runtime acceptance are pending.

All sizes below are device-independent pixels. Scale once from the window's actual DPI, recreate owned fonts on DPI change and measure actual selected fonts with the same renderer used for painting. Never assume CWnd::GetFont returns the font sent through WM_SETFONT to a custom viewport. Each reusable component receives an explicit typography context with a lifetime owned by its top-level window.

## Shared visual tokens

| Role | Light | Dark |
| --- | --- | --- |
| Canvas | #F5F7FA | #101419 |
| Surface | #FFFFFF | #1B2129 |
| Navigation | #EDF1F6 | #151B23 |
| Text | #17212F | #E6EDF5 |
| Secondary text | #526174 | #A8B7C8 |
| Divider | #D5DEE8 | #394758 |
| Accent | #245FC8 | #91B8FF |
| Selected background | #DCE8FF | #243D63 |
| Positive status | #17674B | #79D7B1 |
| Error | #A62A35 | #FFADB5 |

These are reference values for the existing Palette boundary, not a second unrelated palette. High contrast uses system colors and native focus cues rather than these values. Contrast must be measured on the real built output before acceptance.

Typeface: Segoe UI with validated Traditional Chinese fallback. Body/input 14/20, secondary 12/18, navigation 14/20 medium, section heading 18/24 semibold, page heading 24/32 semibold, metric value 28/36 semibold. Never use a secondary font for primary controls. Bilingual text has two measured lines; secondary language remains at least 12/18. Accessible names include both languages in full.

Spacing uses 4, 8, 12, 16, 24 and 32. Content inset 24, minimum 16. Control height 36 in one-language modes, minimum 48 in bilingual mode when two lines are needed. Icon-only target at least 32, with a full accessible name and tooltip. Ordinary buttons have radius 12, input fields 8, grouped surfaces 16. These are deliberate desktop-density adaptations of Material shape and containment, not a claim that the desktop must duplicate a mobile control's proportions. Only status badges may be pill shaped. Prefer dividers and whitespace to borders around every label. One filled primary action per action domain; secondary actions use tonal, outline or text treatment. Focus is a distinct 2-pixel inset/outset ring, not hover color. Navigation icons are authored vectors with a different semantic shape per destination and never replace labels.

## Surface geometry

| Surface | Normal client | Minimum client | Structure |
| --- | --- | --- | --- |
| Workspace | 1280 × 840 | 800 × 600 | 176 navigation, flexible main pane, compact toolbar, metric strip, chart, adapter table |
| Settings | 1280 × 840 | 760 × 640 | 176 navigation, persistent search, scrollable task form, fixed domain action bar |
| Setup | 960 × 760 | 760 × 640 | 176 step list, scrollable step body, fixed Back/Next/save footer |
| Menu | 420 × 480 | 320 × 400 | search plus adjacent regex, checked result rows, keyboard hint |
| Regex | 1280 × 840 | 640 × 400 | search, task tabs, expression/flags, editor/results split, action bar |

Settings navigation contains six destinations, in order: Monitoring, Floating monitor, Taskbar, Notifications, Interface, All groups. The last destination is the existing discovery surface. Captions and translations come from SettingCatalog. A vertical list replaces equal-width header buttons. Label height is measured; bilingual lines wrap within the navigation column. Content and search origins derive from the measured header bottom. Minimum dimensions do not grow to hide layout failures.

Workspace uses exactly three existing data destinations inside its main pane: Adapters, Sample history and Details. Top-level navigation reaches Monitor, History, Plugins, Settings and Setup through existing actions without inventing extra workspace tabs. Pause/Resume display, Restart samples and the floating-monitor toggle remain visible in the contextual toolbar. Metrics are a single strip divided into three readings, not three giant call-to-action cards. Chart follows, then the selected data tab. At minimum width the primary toolbar keeps search and its adjacent regex together; remaining actions wrap to a second row. At minimum height the main content scrolls only when its actual content bounds exceed the viewport, while navigation and search remain reachable. Tab focus reveals the actual focused child, including editor actions.

Settings language and appearance appear as two compact task sections with label/help on the left and selector on the right when content width is at least 600. Below that, each label/help stacks above its full-width selector. Both selectors open independent searchable menus with separate query/pattern/flags. A small preview follows. Accepted provenance and conflict messages occupy a measured status region above a fixed action bar. Apply, Use saved and Reset wrap by measured width. An overflow of translated actions increases footer height and reduces the scroll body, never overlaps it. Reset belongs to a less prominent secondary action. Global search filters the two semantic settings, yields a correct count and a real empty state. It never removes the action/status region needed to recover a retained draft.

Setup's step list includes progress state in text and a numbered marker. Step 1 uses the same two preference sections, followed by existing style controls in the scroll body. No controls disappear because the reference only depicts the first visible portion. Next, Back and Save and close remain pinned. Unknown schema changes Save and close to Close without saving, leaves navigation/protected writes disabled as appropriate, and never traps Escape or window close.

Regex normal layout places test text and results side by side. Under 880 logical pixels they stack in a scroll body. Expression and flags stay above the body. Library, Explain, Replace and Test are task tabs tied to actual existing behavior; unsupported capabilities remain explicit. Every existing control needs a destination before the old long form is replaced. Native multiline edit controls retain caret, selection, IME, undo and keyboard behavior. The owner search accepts changes only through the explicit Use pattern action.

## Scale adaptation and text measurement

The 122 references cover logical geometry, not physical display scaling. Required built tuples add 96, 120, 144 and 192 DPI. At 200%, the minimum 760 × 640 logical Settings client requires 1520 × 1280 physical pixels; when the monitor work area cannot supply that, report the physical limitation and exercise reachable scrolling at the actual available size. Do not claim that resizing at 96 DPI proves 200% text rendering. Recompute every font, line box, navigation row, action width and scroll range after DPI/language changes. Preserve focus ID, selected stable identity, caret and scroll anchor. Each text block is measured before placing the next block; only data columns may deliberately ellipsize with accessible full text and tooltip.
