# State and interaction inventory

Current visual refinement: [release 48 refinement](release48-refinement.md), based on source `eae4d77243e53cc25df57ccb9ef6f9a665a33562`. Its visual decisions supersede conflicting earlier dimensions below; historical evidence and behavior contracts remain unchanged. Independent review and native runtime acceptance are pending.

| Owner | Required states | Acceptance |
| --- | --- | --- |
| Workspace | live, sampling paused/unavailable, adapter selected/missing, empty history, query match/empty/invalid regex | Real values and stable adapter identity survive layout and preferences refresh; no sample data in production |
| Settings | first-open defaults, migrated, loaded, dirty, saved/read-back, stale conflict, protected, uncertain write, filter match/empty | Provenance is truthful; global filter applies to semantic rows; no hidden discard; footer belongs to selected domain |
| Setup | new, resumed clean, resumed dirty A against accepted B, schema1 migration, protected unknown/malformed, action pending/success/failure, complete | Draft/base pair persist; no incidental accepted overwrite; close works without protected-file writes |
| Selector menu | all three options, filtered one/none, selected checked, keyboard focus, IME composing, nested regex editor, canceled | Stable enum IDs; language and appearance states independent; selection changes only draft; Escape returns focus |
| Regex | plain/regex owner state, valid/invalid pattern, running/canceled/timeout, match/no match, collection save failure, unsupported engine feature | Real engine/version and errors preserved; stale worker results ignored; owner changes only on explicit acceptance |

Every principal state applies in English, Cantonese and bilingual presentation, resolved light/dark, normal/minimum size. High contrast, reduced motion and keyboard-only states are additional accessibility cases. References are proposals, not evidence that this matrix ran.

## Keyboard and accessibility

Navigation is one named native list/group with selected state; Tab reaches the selected destination, arrows move within it, Enter activates. No navigation action creates an unintended new workspace tab. Search uses Ctrl+L where already supported and its adjacent regex action has its own labelled focus target. Body order follows visual reading order. Tab and Shift+Tab reveal offscreen controls before focus indication. Escape first ends IME composition, then closes the innermost popup/editor, then invokes the parent's existing draft-safe close behavior. F6 moves between navigation, search, body and footer without applying changes.

Buttons remain buttons, selector menus remain menus with checked/radio state. Do not claim a combobox role for a button. Include context in selector accessible names, for example Interface language: English. Custom drawing stays on the same HWND that receives input and exposes accessible state. Labels, descriptions, validation and selected states must be available through existing MSAA/UIA bridges. A visual focus ring alone is insufficient. Announce status once per actual operation and do not steal focus for background sampling or successful save.

## Explicit negative checks

Search for language shows one Interface row; appearance shows one; unrelated text shows none. Clearing restores both. Regex filter follows the same semantic subjects. Opening each selector retains its own last query and flags, without copying the global Settings query or the other selector state. Choosing an option and canceling the parent never claims persistence. Dirty A based on revision A cannot silently replace accepted B; explicit Use saved/reapply is required. Missing file never displays successful-save wording. Unknown/malformed setup bytes stay identical after Escape/window-close. Schema1 explicit Cantonese followed by schema2 navigation/restart remains Cantonese without pretending a global Apply occurred. Missing schema1 language never becomes an explicit English choice. NativePreferences writes never alter legacy language_id or send a process-wide broadcast.

## Release 48 reference additions

Current generator inventory is 120 surface proposals plus two component sheets. Notifications initial and end-scroll cover every normal/minimum, English/Cantonese/bilingual, light/dark tuple. Interface also has initial and end-scroll references. Setup retains its paired initial/end-scroll references with one step-body scroll owner. Notifications show six stable catalog subjects; daily traffic retains the MB/GB selector action. Each production state in the table above still requires native verification. A design-only drawing never satisfies an unchecked runtime state.
