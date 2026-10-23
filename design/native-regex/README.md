# Native regular-expression workbench

Source baseline: `77435775e46ca78de13859a9e4f2a15732e53415`.

The native workspace, Settings search and each context-menu level own independent search state. A persistent adjacent Regex button opens a native editor anchored to that search. The editor has pattern, literal/regex mode, ASCII case option, subject, literal replacement, construction tokens, search/replace/split previews, match selection and an explicit Apply/Cancel footer. Apply commits only a successfully evaluated draft; Cancel and Escape preserve the accepted owner filter and return focus.

Use the existing native Material palette and button component, native edit/list controls, visible keyboard labels, logical tab order and per-monitor DPI layout. Intended client minimum: 640 by 400 logical pixels, with a vertically scrollable 700-pixel content area. English and Traditional Chinese use the existing language selection. Bilingual mode and the full appearance/customization contract remain unfinished.

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

## Modeless presentation continuation

The editor is now an owned modeless native tool window. One caller-owned `NativeRegex::Editor`
controls its lifetime, duplicate open focuses the existing window, and owner destruction
cancels and destroys it. The owner remains usable. Moving the owning search repositions the
editor below or above it, clamped to the monitor work area; a reduced-height viewport retains
vertical scrolling. This is implementation design, not measured visual evidence.

Apply fetches the current owner corpus, validates it asynchronously and rechecks that corpus
before transferring accepted options. A changed corpus requires a new Apply. Owner query edits,
settings-group changes and menu-level navigation cancel the previous editor. Cancel, Escape and
close leave accepted options alone and return focus only when the editor was active. The menu
message loop includes modeless keyboard translation and treats the editor as part of its owned
popup group; HMENU identity checks remain unchanged. Visible owned windows continue to trigger
the existing automatic-update restart deferral, without updater backend changes.

Runtime proof, focus/DPI/theme/language matrices and full RegexBuddy parity remain unfinished.
No tests, independent review, audits, GUI interaction or screenshots are authorized in this pass.
## Named local expressions continuation

Add a compact saved-expression section above the pattern editor: a labelled native dropdown, explicit Load, a name field, Save new, Rename, Delete and Recover backup. Keyboard tab order follows these actions. Selecting a record alone never changes the draft. Load validates the whole selected record and engine syntax before replacing draft pattern/mode/flags/operation/replacement; Apply still validates the current owner corpus separately. Each owner keeps its own accepted filter and draft.

Save records only an explicitly named snapshot, never sample subjects, corpus, results or implicit history. Names are unique under ordinal case-insensitive comparison. Save never silently overwrites. Rename/delete require an unchanged selected record; concurrent changes require reload. All storage operations run asynchronously, serialized across processes through an exclusive local file handle, with bounded complete reads and atomic replace. Existing configuration-directory selection preserves installed AppData and portable semantics. A strict versioned UTF-16LE format binds schema and bounded-NFA engine version; malformed, unsupported and duplicate records reject the whole file. The previous valid file remains a recovery backup. Recovery is explicit and never applies a search.

Bounds: 64 records, 96 UTF-16 units per nonblank name, 256 pattern units, 4096 replacement units and 1 MiB per file. Supplementary Unicode is lossless; malformed surrogate pairs and embedded NUL are rejected. Import/export compatibility, descriptions, parameters, sample storage, other engines and full collection parity remain unfinished. No tests, independent review, runtime interaction or screenshots are authorized; native and static production will be source-bound separately.
