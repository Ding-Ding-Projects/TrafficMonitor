# Native regular-expression workbench

Source baseline: `77435775e46ca78de13859a9e4f2a15732e53415`.

The native workspace, Settings search and each context-menu level own independent search state. A persistent adjacent Regex button opens a native editor anchored to that search. The editor has pattern, literal/regex mode, ASCII case option, subject, literal replacement, construction tokens, search/replace/split previews, match selection and an explicit Apply/Cancel footer. Apply commits only a successfully evaluated draft; Cancel and Escape preserve the accepted owner filter and return focus.

Use the existing native Material palette and button component, native edit/list controls, visible keyboard labels, logical tab order and per-monitor DPI layout. Intended client minimum: 720 by 600 logical pixels. English and Traditional Chinese use the existing language selection. Bilingual mode and the full appearance/customization contract remain unfinished.

## Execution boundary

TrafficMonitor bounded NFA v1 uses UTF-16 code units and ASCII-only optional case folding. Supported syntax is explicit: literals, escaped punctuation, dot, character ranges/classes, ASCII digit/word/space classes, anchors, alternation, noncapturing groups and greedy `?`, `*`, `+`. Unsupported constructs are rejected. Limits cover pattern length, automaton states, subject/corpus length, rows, output count and total operation steps. Background workers check cancellation and generations; UI consumers never run the matcher. No network, external process, logs, file replacement or persisted subjects.

## States and actions

- `owner.literal`: live plain-text search, independent accepted state.
- `editor.draft`: editable pattern/options/example, accepted owner unchanged.
- `editor.running`: worker active, Apply disabled, cancel available.
- `editor.invalid`: syntax/bounds message, old owner retained.
- `editor.results`: bounded completed preview, indexed match navigation and exact spans.
- `editor.apply`: accepted draft transferred only to the originating owner.
- `owner.pending`: previous rows remain visible until the newest generation completes.
- `owner.error`: previous accepted rows remain, explicit status reports the incomplete request.

Context-menu identity and HMENU notification lifecycle stay intact. Opening the editor must suppress only the popup's ordinary deactivation dismissal for the duration of the owned modal decision. Queued invocation generation still invalidates on a new accepted filter.

## Verification state

This is a design and implementation handoff, not runtime evidence. Independent review, tests, GUI interaction, screenshots and the DPI/theme/language matrix are deferred by the explicitly selected speed pass. Exact root build/package entrypoints may provide compilation and package evidence only. Full RegexBuddy parity remains unfinished and is tracked separately.
