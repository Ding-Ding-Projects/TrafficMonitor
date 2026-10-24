# Release 48 native refinement

Status: design proposal, implementation and independent review pending. Source inspected: `eae4d77243e53cc25df57ccb9ef6f9a665a33562`. This document supersedes conflicting visual dimensions in `layout-and-tokens.md`; persistence, validation, action destinations and native frame requirements remain binding.

## Evidence and decisions

The supplied release 48 Interface, Notifications and setup-language images were inspected as before-state evidence. They show oversized per-setting containers, an empty-looking preview container, repetitive implementation explanations, crowded notification value rows and a setup preference viewport isolated from the message-style controls. No before-state image is copied into this design folder. The adapter-filter image is excluded from publication because it contains adapter identity. These observations establish design priorities, not new runtime acceptance.

| Observed composition | Replacement decision | Behavior retained |
| --- | --- | --- |
| Separate large white language and appearance cards | Two compact divider-separated form rows on the main surface | Independent selector search, retained draft and explicit Apply |
| Large mostly empty preview | One 48-unit sample row, 72 when two languages need two lines | Actual draft wording/appearance, selectable native text, no fabricated live result |
| Repeated descriptions of native windows and storage internals | Concise decision guidance; scope and provenance in contextual Details | Protected/conflict/save-failure guidance stays immediately visible |
| Notification label, number, unit and prose compete | One logical setting group: enable row, labelled threshold, unit, optional detail | Every numeric bound, unit action, validation, unsupported capability and stored preference |
| Preference viewport inside setup with style sliders outside | One outer step viewport for preferences, their actions and both style ranges | Same draft/base pairing, progress writes and setup action sequence |
| All footer actions equally heavy | Filled Apply within preference domain, text Use saved/Reset; filled Next in wizard footer | No implicit Apply through Next, no loss of Save and close, conditional Reapply retained |

## Semantic roles and typography

Use the existing `NativeMaterial::Palette` boundary, extended with named roles rather than a second palette per owner. The blue scheme below is a product palette using Material 3 role semantics, not Google's default purple scheme. Generator aliases map to these roles: `bg` surface, `panel` surfaceContainerLow, `nav` surfaceContainer, `text` onSurface, `muted` onSurfaceVariant, `line` outline, `accent` primary, `selected` primaryContainer and `ink` onPrimary. Add separate outlineVariant, onPrimaryContainer, secondaryContainer and onSecondaryContainer fields in production instead of reusing one color for unrelated states.

| Role | Light | Dark |
| --- | --- | --- |
| surface | #F9F9FF | #111318 |
| surfaceContainerLow | #F0F3FA | #1D2026 |
| surfaceContainer | #E9EDF5 | #272A31 |
| onSurface | #191C20 | #E1E2E9 |
| onSurfaceVariant | #43474E | #C3C6CF |
| outline | #73777F | #8D9199 |
| outlineVariant | #C3C6CF | #43474E |
| primary | #345E9C | #A8C8FF |
| onPrimary | #FFFFFF | #003064 |
| primaryContainer | #D6E3FF | #19467F |
| onPrimaryContainer | #001B3E | #D6E3FF |
| error / onError | #BA1A1A / #FFFFFF | #FFB4AB / #690005 |

Use Segoe UI with measured Traditional Chinese fallback as the native platform font adaptation. Headline small 24/32 regular for page title, title large 22/28 regular for a principal section, title medium 16/24 medium for row labels, body large 16/24 for editable values, body medium 14/20 for explanation, label large 14/20 medium for actions. Do not shrink text to fit a translated label. Two-language rows measure both lines independently, using at least body medium for the second language. The reference drawings remain approximate font layout; native DirectWrite/GDI measurement owns final placement.

Material color and type role names were checked against Google's maintained [color tokens](https://github.com/material-components/material-web/blob/main/tokens/_md-sys-color.scss) and [type scale tokens](https://github.com/material-components/material-web/blob/main/tokens/_md-sys-typescale.scss). This is a native adaptation; no web renderer is imported. Earlier attempted version-specific URLs were unavailable and supply no evidence.

## Component anatomy

All geometry is logical pixels and scales once from the actual window DPI. Base spacing is 4, 8, 12, 16, 24 and 32. Normal content inset 24, absolute minimum 16. Constrain form content to 840; leave remaining space as honest canvas rather than stretching controls across the whole window.

- Buttons: Material filled, filled-tonal, outlined and text variants. Minimum visual height 40, two-language minimum 56, full rounded end shape (radius half height). Horizontal label inset 24, icon gap 8. A 40-unit visible control has a nonoverlapping 48-unit interaction allocation where possible. Preserve full accessible names and keyboard targets. Selector triggers are outlined fields, not filled actions.
- Selector: 1-unit outline, 4-unit corner radius, 16-unit value inset, 24-unit chevron allocation. Focus outline 2. Height 56 for a field with persistent floating label; an external measured label may use a compact 40-unit trigger. The two forms must not show duplicate labels. Same stable identity, popup search and cancel behavior.
- Checkbox: 18-unit square indicator, radius 2, 2-unit outline when unchecked, primary fill plus onPrimary check when checked, explicit mixed line. Allocate 48-unit interaction height, label starts 32 units after row origin. Draft booleans remain checkboxes; do not recast them as immediate switches.
- Slider: 4-unit track, 20-unit thumb, visible discrete ticks for the 1..5 range, 40-unit hover/focus state layer within a 48-unit input allocation. Show numeric value persistently beside its label. Existing arrow, Page, Home/End and range provider behavior remains. Support serious/playful endpoints through concise labels or accessible description.
- List/menu: 48-unit minimum single-line row, 64 or measured larger for bilingual/supporting text. Leading icon/check allocation 24 with 12 gap. Selected row uses container/on-container roles; focus has its own outline. A check mark and accessible selected state distinguish selection from hover. Never use a colored fill alone.
- Surface grouping: use no per-row card on Interface. Optional preview uses surfaceContainerLow, radius 12 and padding 16. Dividers use outlineVariant. Dialog container radius is 28 only inside a custom client surface; the real operating-system frame remains unchanged.
- Hover layer: on-content role at 8 percent opacity; pressed/focused state layer at 12 percent. Focus additionally has a 2-unit visible outline with 2-unit separation so it survives both selected and hovered states. Disabled content uses onSurface at 38 percent and container at 12 percent; disabled reason remains full-contrast supporting text. Validate actual contrast, not just token names.
- High contrast: resolve foreground/background/highlight from system colors; use explicit borders/checks/selection and a visible focus outline. Preserve component anatomy and semantics. Do not silently return an unstyled generic widget. Reduced motion suppresses animated state transitions without removing feedback.

## Source and control mapping

| Owner | Required implementation change | Invariants |
| --- | --- | --- |
| `NativeMaterial/Theme.h`, `Theme.cpp` | Extend role vocabulary and typography context; separate outline from subtle divider | DWM native frame, high contrast and Canvas fallback remain |
| `NativeMaterial/ButtonVisual.*`, `Controls.*` | Shared filled/tonal/outline/text/selector anatomy, full shape, range ticks and states | Same HWND/provider, BM_CLICK, Space/Enter, native range keys |
| `NativeMaterial/SettingsControls.*` | Compact checkbox indicator and measured label allocation | Native auto-checkbox checked/mixed/disabled semantics |
| `NativeMaterial/TextField.*`, `ScrollableTextField.*` | Shared field outline, supporting validation and rail inset | Native EDIT caret, IME, selection, undo, clipboard and limits |
| `NativeMaterial/SearchSelector.*`, `Navigation.*` | Selected versus focused row treatment; measured icon/label columns | Stable IDs and real list/menu semantics |
| `NativeMaterial/ScrollViewport.*`, `ScrollRail.*` | One body range, reserved 12-unit rail gutter, focused-child reveal | Wheel accumulation, page/thumb/key actions and provider range |
| `NativePreferences/Editor.cpp` `Layout` | Remove white per-row framing from `m_languagePanel`/`m_appearancePanel`, use measured rows and compact `m_previewPanel` | IDs 56200/56201/56210; no draft/storage changes |
| `NativePreferences/Editor.cpp` `LayoutActions` | Measure state message, wrap actions, show `m_reapply` only on conflict | IDs 56202..56205, expected-base compare, protected state and subscriptions |
| `NativeSettings/Settings.cpp` `Row`, `Layout`, `RefreshRowText` | Group check/label, threshold/unit and conditional Details, move defaults provenance into expanded details | `FirstRow=54100` addressing, validators, unit actions, reset and restart behavior |
| `NativeSettings/SettingsViewport.h` | Content extent derives from measured visible rows | Search filter and focused-row reveal, independent group scroll |
| `NativeDashboard/SetupFlow.cpp` `Wizard::Layout` | Parent all step controls into one step body; include preferences/actions and ranges there | IDs 55002/55003, progress serialization, protected-close route |
| `NativeDashboard/SetupFlowText.h` | One concise description per decision; no implementation narration | Localized truthful scope, unsupported actions and save result |

`Editor` currently owns separate panel HWNDs. Retaining those HWNDs with transparent/surface-matching paint is acceptable if focus and hit testing remain exact; flattening visual containment does not require rewriting persistence or replacing controls. Existing Apply/Use saved/Reset/Reapply commands must remain connected. A conflict requires an explicit message and visible Reapply or Use saved; it must not disappear into Details.

## Settings geometry and information hierarchy

Retain 176 navigation, persistent search plus adjacent Regex and Clear, and the existing six destinations. Main content starts at x=200 at the normal inset; minimum client remains 760 by 640. At content width at least 640 use label 45 percent and selector 55 percent with measured label bounds. Below 640 stack label then selector with 8 gap. Compact single-language rows target 64 units wide or 88 stacked; bilingual targets 80 wide or 112 stacked. These are minimums, not clipping limits. Measure actual wrapped lines first and increase height as needed.

Interface only shows its section title, two setting rows, a real draft preview and state/action region. One concise contextual help destination carries scope details. A missing preference file may read "Using defaults"; Details explains why. Never claim saved until readback. Keep failure, conflict, protected and uncertain-write guidance visible. The preview is labelled Preview and must not assert that monitoring is currently running.

Notifications preserve all six actual settings. Each group has an enable checkbox/title and Details, then a labelled threshold field with its unit. The Details action must not consume threshold width. Single-language minimum target 144 units per group, bilingual 192; expanded explanation adds measured height. Do not force six groups into one screen. Threshold input remains editable or disabled according to existing draft semantics, never an invented immediate action. The unit is text only when immutable; preserve the existing clickable unit-selection action where supported, styled as an outlined selector. Unavailable temperature support keeps its exact saved value and an inline reason; only extended provenance belongs in Details.

Footer height is measured from the state message and wrapped action labels. Layout footer first, then reserve the remaining body height. Normal references reserve 88 single-language or 112 bilingual units; longer real status text increases this allocation. No absolute-position footer may cover the last form row. A group filter retains draft and actionable footer, reports the true count, and shows a real empty state.

## Setup and scrolling

The revised setup references intentionally remove the duplicate settings-style navigation and search shell from step 1. Keep a compact "1 of 4" title and progress indicator, then one body. This is task progress, not a new destination catalog. Step names and existing actions remain represented by the progress model. The body contains language/appearance, draft preview, Apply preferences/Use saved/Reset and conditional Reapply, then both message-style ranges and a short scope sentence. Next, Back and Save and close remain in a fixed footer. Use text treatment for Back/Save and close; only Next is filled in that footer.

Do not put `m_preferences` in a scrolling island with ranges outside. Use one `SettingsViewport`/`ScrollViewport` step body, compute total content bottom after the final style description, and scroll all step children together. A popup menu and a multiline text editor may legitimately own an internal scroll range; the simple preference section does not. Tab/Shift+Tab reveals the entire focused control and supporting validation. On language/theme/DPI refresh preserve focused ID, caret, active popup state, draft and scroll anchor. Clamp the anchor to the new actual range.

At minimum height the lower style controls may begin offscreen. The paired initial/scrolled references explicitly show both positions. Preserve keyboard reachability and visible scroll affordance. Unknown-schema state uses Close without saving and retains window close/Escape without writes. Next does not silently apply interface preferences; final setup Apply still checks the existing dirty preference state.

## Other owners and migration order

Workspace uses the same typography, field, button, table, selection and focus roles. Keep current metrics/data tabs and all existing commands. Do not expand a metric into a decorative card. Menus use the same outlined search, checked selection and focus; each selector keeps independent regex/query/flags. Regex keeps native multiline editing and truthful engine capability; no reference change authorizes new algorithms or hides unsupported capability. See `action-destinations.md` for exact command routing.

1. Extend shared role/type/state primitives and review component sheets. Preserve native frame.
2. Implement compact shared preference rows and Settings Interface, then notification grouping. Verify real draft, validation, conflict and filter behavior.
3. Flatten setup step scrolling while retaining serialized progress and every action. Verify restart and protected-close behavior.
4. Apply the same anatomy to workspace/menu/regex without altering owner-specific data and execution semantics.
5. Compare real built output at identical tuples and correct discrepancies. Original application-owned dialogs remain subsequent mandatory work under the broader remake scope.

## Reference and acceptance inventory

The existing generator remains the only reference producer. It now produces 120 surface SVGs and two component sheets: the prior 84 surface states plus twelve Notifications tuples and twenty-four Settings Interface/Notifications end-scroll tuples. Each proposal contains a design-only description and synthetic values. Client area only, no fabricated caption or runtime image. The revised references cover normal/minimum, English/Cantonese/bilingual and light/dark. Display scale obligations remain 96/120/144/192 DPI on the real native output, plus high contrast, keyboard-only, reduced motion, IME and valid/invalid/protected/conflict states from `states.md`.

- [ ] Independent design review accepts this revision and checks every changed reference.
- [ ] Native measurements show no overlap, cropped text or unreachable control at every required tuple.
- [ ] Notification units retain their actual action, bounds and validation.
- [ ] Setup has one step-body scrollbar and every control remains keyboard reachable.
- [ ] UIA/MSAA name, role, state and range are checked on the same rendered HWND.
- [ ] Before/after images bind exact source, executable hash, dimensions, DPI, language and appearance, with privacy review.
- [ ] No design proposal is promoted to runtime evidence or used to claim delivery.

Status reporting for this design lane could not establish an authenticated current-version service session. No delivery to that service is claimed. Build, release, installed-update and runtime verification belong to the integration owner and remain unrun by this design author.

## Design-author verification

The generator ran successfully. All 122 SVGs parsed as XML, all explicit rectangle dimensions were nonnegative, and a second generation was byte-identical by SHA-256 for every file. `git diff --check` passed. Representative native-independent SVG rasterizations were visually inspected: minimum bilingual Interface initial/end-scroll, minimum bilingual Notifications initial/end-scroll, minimum bilingual setup end-scroll, normal English setup and the dark component sheet. These checks validate design files only, not native font layout, behavior, display scaling or production accessibility. Final search/count refinements remain subject to the independent review. The author did not build, launch, modify production code, commit or publish.
