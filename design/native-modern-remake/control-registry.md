# Native component replacement registry

The latest acceptance criterion rejects unstyled generic controls and default scrollbar appearance across the owned native surfaces. Merely recoloring a standard border or making every action a pill does not meet it. This registry names the actual classes and styles found in the source baseline. Native editing and accessibility behavior remain mandatory.

## Component anatomy and source mapping

| Current source/control | Replacement presentation and retained behavior |
| --- | --- |
| NativeMaterial Button | Shared Primary, Tonal, Outlined, Text and Icon variants, desktop radius 12, measured label, hover/pressed/disabled/focus states; same HWND and provider, Space/Enter/BM_CLICK behavior |
| Workspace m_search; Settings m_search/SettingEdit; menu EDIT; regex pattern/replacement/savedName | Shared filled or outlined TextField frame with persistent label, input inset, supporting/error text, trailing clear/regex actions where applicable. Native EDIT child has no WS_EX_CLIENTEDGE/default border; retain caret, selection, clipboard, undo, IME, limits and accessible label association |
| Regex subject/output; workspace m_details; ReadOnlyText | Shared multiline editor frame, line-height and padding, custom scroll adapter; native EDIT retains text semantics and selection. Read-only state is named and selectable, not disabled |
| NativeMaterial Choice; setup m_choices; regex savedList | Shared outlined selector/button with selected value and chevron, searchable owned popup where required. Retain stable item identity and selection notifications. No exposed default combo dropdown border or scrollbar. Real button/menu role if implemented through a popup, never false combobox semantics |
| NativeMaterial SettingsCheck | Styled checkbox for independent booleans; switch only for immediate enabled/disabled settings where semantics match. Use a custom-drawn 18-unit indicator and measured full label; native checked, mixed, disabled and keyboard state retained |
| NativeMaterial RangeValue | Styled track, thumb, current numeric value and focus state, native range semantics/arrow/Page/Home/End keys. Never hide the value solely in a tooltip |
| Workspace CListCtrl adapters/samples | Shared table row/header styles, alternating background only if useful, selected/hover/focus states, measured bilingual headers, native selection and accessible columns. Skin header and both scroll axes |
| Settings m_results; menu LISTBOX; regex results | Shared result row with primary/secondary text, icon/check column and optional shortcut. Real list/menu role, keyboard selection, visible focus, empty state, stable identity |
| CStatic titles/notes/status | Shared measured text blocks with explicit typography, semantic status identity and full accessible name; no fixed-height text that hides a second language |
| Workspace WS_VSCROLL; SettingsViewport WS_VSCROLL; setup m_preferenceBody; regex dialog WS_VSCROLL | Remove default nonclient scroll chrome. Use shared ScrollViewport with custom ScrollRail and real range/page/position, contained body, focused-child reveal and persistent anchor |
| Native EDIT/LISTBOX/CListCtrl internal scrollbars | Shared ScrollAdapter drives the native control's real scroll messages/positions while custom rails render its actual range. Do not emulate text layout, lose accessibility range or hide overflow. If a control cannot expose reliable ranges, replace it with an accessible equivalent before acceptance |

## ScrollRail specification

Scrollbar styling is an explicit native adaptation, not a claim of an official one-to-one Material component. The body reserves 12 logical pixels for the rail. A 6-pixel rounded thumb rests inside an 8-pixel track; hover/drag expands the visible thumb to 8 pixels without changing content geometry. Thumb length is proportional to viewport/content, minimum 32. No track/thumb when content fits. Track uses the navigation/container role; thumb uses secondary text with verified contrast. Hover, active drag and keyboard focus have distinct visuals. High contrast uses system foreground/background and a visible outline; system accessibility settings may increase dimensions.

Wheel/precision-wheel accumulation, PageUp/PageDown, Home/End, arrow increments, track page clicks, thumb dragging, capture loss, disabled states and DPI changes must be implemented. Preserve native scroll positions and notifications. A rail exposes an accessible scrollbar with orientation, minimum, maximum, current value and supported actions, linked to its viewport. Keyboard focus does not disappear when the rail becomes unnecessary. Never overlay the thumb on editable text or hide standard bars without a functioning replacement.

## TextField specification

Filled fields use a quiet container and bottom emphasis; outlined fields use a 1-pixel outline, 2-pixel focused accent, persistent floating label and 12-pixel horizontal inset. Validation uses an error outline plus explicit supporting text and accessible status, never color alone. Disabled, read-only, hovered, focused, invalid, composing and empty states are distinct. Trailing controls have independent names and keyboard targets. Supporting text wraps and increases the field group's measured height. Do not paint a fake caret or intercept IME composition with Enter/Escape shortcuts.

## Provenance and negative acceptance

Material component reference URLs: https://m3.material.io/components/text-fields/overview and https://m3.material.io/components/navigation-rail/overview. The parent review reported these pages were JavaScript-only through its text route. Author-maintained implementation references supplied by that review: https://github.com/material-components/material-web/blob/main/docs/components/menu.md and https://github.com/material-components/material-components-android/blob/master/docs/components/TextField.md. These are provenance pointers, not a claim that their complete contents were independently retrieved in this lane. No web or Android runtime is imported. The exact native adaptation remains this checked-in specification and requires review.

- [ ] No default nonclient scrollbar is visible on any owned surface or popup in either appearance.
- [ ] No raw EDIT client edge, unstyled combo popup, checkbox, slider, list header or list selection remains in an owned surface.
- [ ] Every scrollable viewport is reachable by wheel, keys and accessible actions, including nested multiline editors.
- [ ] Native caret/IME/selection/undo and UIA/MSAA state remain functional after skinning.
- [ ] High contrast remains readable and operable, with an explicitly reviewed accessible rendering rather than a silent generic fallback.
- [ ] All deviations and unsupported controls are listed with exact source/state; no blanket exemption for legacy behavior inside an owned surface.

Original dialogs outside the five-owner scope remain explicitly unfinished under the full-application request, as recorded in handoff.md. Their existence prevents a full-application completion claim.
