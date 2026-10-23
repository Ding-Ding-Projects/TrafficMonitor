# Native preference reference geometry

These are textual layout references, not screenshots and not a runnable reference application. No visual parity or implementation claim is made. A native deterministic reference fixture and built comparison routes must be added in the implementation/evidence lane before visual parity can pass.

## Reference selector

A reference tuple is `(surface, state, language, appearance, dpi, clientWidth, clientHeight)`. Stable surfaces: `interface-settings`, `setup-language`, `workspace`, `menu`, `regex`. Stable states are explicitly listed in states.md. Inputs freeze sample values and timestamps in the reference fixture only; production monitoring must not be replaced by a fixture. Language values are `english`, `cantonese`, `bilingual`; appearance is resolved `light`, `dark`, plus `high-contrast` accessibility cases. The future executable fixture accepts an equivalent bounded selector and records its exact invocation. No fixture currently exists for this unit.

## Interface settings page

Retain the current Settings shell, persistent search and adjacent regex entry, group navigation and contained scrolling body. Existing baseline normal client is 860 x 720 logical pixels; minimum is 760 x 640. This proposal does not increase the minimum.

Body content uses a 20-unit left/right inset, 16-unit inter-field gap and an 8-unit label-to-control gap. Title is 24 units, body and control text 14, secondary labels 12 with a minimum 16-unit line box. Use actual text measurements, not these nominal sizes, to calculate height. Two stacked dropdowns each fill available content width and have at least 40-unit interactive height. Their labels and explanation can wrap above/below without increasing the dropdown's collapsed text into an unreadable line. The bilingual value uses a concise stable bilingual label; long explanatory text wraps in the body.

The sample is a neutral bounded container with one status label and a sample value, clearly labelled Preview. It follows the draft language/appearance while surrounding chrome follows accepted preferences. It has no fake live graph. The page footer belongs to the contained layout: 40-unit minimum actions with wrapping rows when measured text exceeds available width; status occupies its own measured block. Apply is filled primary, Revert outlined, Reset text-style. State is conveyed in words and accessible properties, never by color alone.

Layout reading order: page title; language label, selector, explanation; appearance label, selector, effective-state explanation; preview; save status; Apply, Revert, Reset. Header search remains available while body scrolls. When content is taller than the minimum viewport, scroll the body and retain reachable footer rather than overlap or hide the preview. The existing monitoring group footer is not simultaneously shown as another ambiguous Apply destination.

## Other consumers

| Surface | Source baseline dimensions (logical client units) | Proposed treatment |
| --- | --- | --- |
| Setup | normal 760 x 640; minimum 720 x 600 | Language and appearance occupy stacked fields on step 1. Style controls remain below. Measure multiline heading/info and use contained scroll if required. Keep Back/Next/Save reachable. |
| Workspace | minimum 800 x 600; normal fixture 1040 x 760 (proposed reference, not existing launch measurement) | Replace local theme action with direct preference navigation. Measure toolbar actions; wrap into rows. Preserve chart/data area and selection. Bilingual labels use primary/secondary runs. |
| Menu | existing height capped at 560 and monitor work area; fixture 420 x 560, narrow 320 x 480 proposed | Measure row primary/secondary and shortcut column separately; variable row height or a measured uniform maximum. Preserve work-area clamping and keyboard scroll. Native command text remains untouched for unknown IDs. |
| Regex | minimum 640 x 400; fixture normal 840 x 720 proposed | Long translated labels wrap; controls and editor regions use contained scrolling at minimum. Existing bottom collection actions must not overlap the editor. All changes preserve draft and caret. |

Proposed reference dimensions for workspace normal, menu and regex normal are not assertions about current launch geometry. Implementation must resolve actual supported geometry and record any deliberate adjustment before accepting evidence.

## Material and text audit

Reuse `NativeMaterial::Palette`, high-contrast resolution, button focus treatment, native dropdown semantics and existing font fallback. Do not introduce an independent theme palette or web renderer. Audit contrast for primary/secondary text, outlines, disabled controls, edit backgrounds, selected list rows and focus indicators in both appearances. Maintain visible focus at every DPI. Use native system message fonts with validated Traditional Chinese fallback; preserve UTF-8 BOM for source strings. No decorative emoji is added to controls or accessible names.

Reference inventory: this geometry document defines five surface references, and states.md defines their explicit behavioral states. All raw references, production screenshots, layout receipts, comparison images, visual differences, runtime accessibility records and independent review verdict are currently unavailable. They must remain pending, not replaced with source inspection.
