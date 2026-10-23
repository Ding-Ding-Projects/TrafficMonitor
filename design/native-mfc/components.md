# Native component contract

Design specification only. Registrations below are proposed stable identifiers, not declarations of implemented controls. A registry entry is accepted only after its renderer, input behavior, accessibility provider and reference-state tests exist. Material Design is a specification provenance, not an MFC component library or a certification granted by using these names.

## Provenance and acceptance

Review against the official [Material 3 component catalog](https://m3.material.io/components) and [color roles](https://m3.material.io/styles/color/roles). These pages required JavaScript during design discovery; their rendered specification must be inspected and its reviewed revision recorded before implementation conformance can be accepted. The project owns this native implementation and must not describe it as Google's shipped component code. The native accessibility contract follows [Microsoft UI Automation providers](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-providersoverview): custom elements need providers and patterns, while retained standard controls must preserve their existing providers. Store reviewed specification source URL, date/revision, anatomical checklist, implementation class, accessible patterns and state evidence per registration. Do not satisfy registration by a class-name substring or by registering one giant painted canvas.

## Registration inventory

| Stable registration | Anatomy and visible states | Native semantics and acceptance |
| --- | --- | --- |
| `surface.window` | Filled root, custom title region, measured content bounds, native resize edges; normal/maximized/inactive | Window provider, title buttons Invoke; hit-testing excludes controls from drag regions; system menu and snap remain functional |
| `surface.panel` | Tonal container, heading, content, optional border/elevation; normal/loading/empty/error | Pane/group provider with explicit name; every text child independently exposed; no opaque bitmap-only content |
| `text.label` | Tokenized type role, fallback shaping, wrapping and measured baselines | Text provider or appropriate static control; accessible full text; selectable read-only data when useful |
| `button.filled` / `button.tonal` / `button.text` | Container, text, optional leading icon, state layer, focus indicator; hover/focus/pressed/disabled/busy | Invoke, Enter/Space, capture-loss cancellation, enabled guard in handler; focus cannot be indicated only by fill change |
| `button.icon` | 24-DIP glyph inside 48-DIP interaction region, tooltip, state/focus layers | Localized name, Invoke and actual shortcut; glyph is not the name; dense taskbar exception described below |
| `input.outlined` | Container, floating/persistent label, editable content, clear action, trailing actions, supporting/error line | Native Unicode edit internals for IME/selection; Value/Text, read-only/password flags, validation relation; clear changes bound value and restores focus |
| `input.switch` / `input.checkbox` | Track/thumb or box/check, label, focus and disabled states | Toggle with announced state; Space changes one value; label target also works; persisted state is read back |
| `input.slider` / `input.stepper` | Track, thumb, bounded numeric value, units and numeric alternative | RangeValue with min/max/step; arrows/Home/End; no precision loss on typed entry |
| `navigation.tabs` | Active indicator, label/icon, close/pin/group and overflow regions | Selection/SelectionItem; correct orientation; arrows change focus, Enter selects; protected tabs excluded from bulk close |
| `selection.list` / `selection.table` | Header, rows, selected/focused states, explicit empty and no-match states, bounded scroll | Grid/Table/Selection and virtualized item identity; stable IDs; shift selection, inverse selection and scoped select-all |
| `selection.menu` / `selection.picker` | Opaque rounded elevated surface, local filter plus anchored builder, rows and shortcut column | Menu/List patterns; type/filter, arrows, Enter; Escape clears then closes; focus returns to exact origin |
| `status.chip` | Compact tonal status with text and optional symbol | Read-only Text unless actually actionable; no false button role; color never sole status |
| `data.metric` | Label, value, unit, freshness/source and optional trend | Group containing independently named text; announce on demand or bounded summary, never every sample |
| `data.chart` | Labelled axes, data series with line patterns, range/legend, empty/stale/paused states | Chart group plus equivalent accessible table; keyboard range/navigation and summary; chart is functional data, surrounding controls remain Material |
| `overlay.sheet` / `overlay.dialog` | Own background, title, content, actions, scroll and collision constraints | Window/Pane with modal flag only for a genuine decision; initial focus, Escape, focus return, keyboard move/resize |
| `feedback.snackbar` / `feedback.center` | Persistent warning/error or timed information, action and dismiss | Polite notification events, no stolen focus; history has search, selection, bulk dismiss/export |
| `guidance.info` | Target-specific icon directly above control, explanation, worked example and isolated playground | 44-DIP minimum entry; expanded state; shared pure validator but no live side effects; focus returns |
| `progress.operation` | Current phase, measured amount or explicit indeterminate state, cancel/retry/recovery | RangeValue when measured, status text otherwise; reentry blocked in command handler; success follows real completion |

The palette, regex workbench, appearance editor, onboarding, security prompts and tools compose these registrations. They are not exempt from their own component, search, guidance, accessibility and customization coverage.

## Common interaction state machine

Every actionable component implements idle, pointer-over, keyboard-focus, pressed-inside, pressed-outside, disabled and busy. Disabled/busy never execute through accelerator or automation. Mouse release invokes only after a valid press on the same target; capture loss cancels. Pointer-over is not keyboard focus. All states have deterministic reference fixtures. Selectable components add selected/unselected and mixed where meaningful. Fields add empty, filled, invalid, read-only, composing and unavailable. Lock wrappers intercept all activation routes before the command, and expose an unlock action without executing the protected action.

Use one command descriptor per real operation: stable ID, label keys, context, enabled reason, shortcut, validation, executor and help target. Menus, palette, dashboard and automation invoke the same descriptor. Every new search, including menu/picker filters, requires its own adjacent anchored full regex builder with independent query, pattern, flags, mode, validation, history and snippets. A shared service may evaluate; it must not share field state. Plain text is the default. A regex toggle alone is not accepted.

## Rendering, layout and accessibility

All geometry is in 96-DPI logical units, converted once per HWND using its current DPI. Recompute text, layout, cached bitmaps and hit rectangles on `WM_DPICHANGED`; honor the suggested outer rectangle. Never mix resource dialog units and physical pixels in new controls. Layout is measure/arrange with minimum/preferred bounds and explicit overflow owners. Long text expands height; tables scroll inside their own region; the window does not acquire horizontal overflow to hide a layout defect.

Expose `WM_GETOBJECT` providers with stable runtime IDs and correct fragment navigation/hit testing. Return UIA host providers for HWND roots and implement Invoke, Toggle, SelectionItem, Value, RangeValue, ExpandCollapse, Scroll and Grid/Table only where the actual behavior exists. Provide control type, name, enabled/focusable/offscreen state, bounding rectangle, help text and keyboard shortcut. Marshal all control mutation onto the UI thread; provider lifetime must survive outstanding calls without dereferencing destroyed windows. Raise focus/property/structure notifications only on relevant change. Reading throughput must not flood a screen reader.

Native text fields preserve IME composition, Unicode selection, clipboard semantics, password concealment and accessible clear controls. An externally hidden helper control is not sufficient accessibility for a custom painted counterpart; bounds, focus and values must match the visible component. Use actual screen-reader/Inspect evidence, not only provider unit tests.

Motion uses one application clock and bounded invalidation, never a timer per control. Hover and press transitions are brief and interruptible. Reduced motion removes spatial transitions and continuous rainbow cycling while retaining authored settings. The native equivalent of stylesheet animation is a shared compositor/animation service with one global rainbow speed mapping and a stable reduced-motion hue. This platform adaptation must be documented and tested; it is not permission to omit motion editing.

## Taskbar boundary

The embedded taskbar meter is an ambient display constrained by Explorer geometry. Preserve its compact height, taskbar insertion/recovery and plugin rendering. Do not expand Explorer's taskbar to fit dashboard controls. Its accessible activation surface opens a native flyout with 48-DIP actions, readable values, provenance and Accessibility entry. A tooltip alone does not satisfy this flyout. The compact meter's small target is an explicit host-geometry limitation with the tray and flyout keyboard equivalents; it is not a blanket exemption for dashboard controls. Record raw taskbar bounds, host class, DPI and actual attachment separately from dashboard evidence.

## First implementation boundary

Implement the root, typography, button, panel, metric, status and disclosure registrations before the dashboard. Establish token, layout and UIA tests, including a deliberately unregistered visible child that must fail the inventory check. Then bind the dashboard to immutable real snapshots. Do not begin broad Options conversion by replacing `CBaseDialog::OnCtlColor`; that path cannot supply new anatomy, input semantics, layout or accessible patterns. Convert dialogs one cohesive workflow at a time through adapters around existing consumers.
