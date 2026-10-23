# Native search pattern workbench

Choose **Regex** immediately beside the native workspace, Settings or context-menu search. Each dialog or menu level owns its own query, literal/regex mode, ASCII case option, validation and accepted rows. Plain text is the default. The editor is positioned beside the originating search and returns focus there. It is an owned modeless native tool window: the originating search remains usable, reopening focuses the existing editor, and moving the search reanchors the editor within the monitor work area.

Edit the query, choose literal or regex mode, use construction buttons to insert supported syntax at the selection, and edit the local example subject. Search lists indexed UTF-16 spans; selecting a result highlights that span. Replace previews literal replacement text, with no capture expansion. Split previews fields separated by CRLF. No file is read or changed. Run a successful preview, then Apply to validate every current owner row before transferring the draft. Apply rechecks the corpus before accepting results; a changed corpus requires another Apply. Cancel or Escape keeps the previous owner state. Editing the owner query, changing the Settings group or navigating menu levels cancels that owner's stale editor. Changing input cancels pending results and disables Apply until another preview succeeds.

## Actual engine and bounds

`TrafficMonitor bounded NFA v1` is an independent UTF-16-code-unit engine, not a named third-party flavor. It implements leftmost-longest nonoverlapping matches, literal text, escaped punctuation, `\n`, `\r`, `\t`, dot excluding CR/LF, positive/negative character classes and ranges, ASCII `\d`, `\w`, `\s`, whole-subject `^`/`$`, alternation, noncapturing `(?:...)` and greedy `?`, `*`, `+`. Optional case folding affects ASCII A-Z only. There is no Unicode-property database/version. Replacement is literal. Empty matches advance one UTF-16 unit; matching never splits or rewrites source files.

Each operation permits at most 256 pattern units, 512 automaton states, 8,192 units per subject, 65,536 total subject units, 512 owner rows, 256 preview matches, 4,096 replacement units and 65,536 output units. A shared budget across the complete operation caps execution at 4,000,000 steps and checks cancellation and a 500 ms elapsed limit every 256 steps. This is a cooperative bound, not a hard real-time scheduling guarantee. Parsing is bounded by the pattern and state limits. There is no backtracking or external process.

Every owner has at most one asynchronous worker and one replaceable pending request. Generation checks discard stale results; workers own their input and never hold HWNDs. Destruction cancels and joins that bounded worker. The UI polls completed results every 40 ms. Invalid, canceled, oversized and timed-out results retain the previous accepted rows. Live adapter rows are associated with their searched text rather than stale numeric positions. All input stays in memory, with no network, history, logs or persistence.

## Search ownership

| Surface | Owner | Scope |
| --- | --- | --- |
| Native workspace | `NativeDashboard::ViewModel::search` | Adapter alias and description |
| Native Settings | `NativeSettings::Editor::m_filter` | Current group, matching the full stable label corpus |
| Native context menu | Each `NativeMenus::Level::filter` | Current HMENU labels and shortcuts |

The menu keeps its item order, owner notification lifecycle and identity revalidation. The popup and editor form one owned modeless window group. Its message loop routes keyboard translation to the editor before menu navigation, allows clicks inside either window and retains dismissal when the group loses activation. Item invocation remains invalidated by the menu generation. Settings retains draft changes and its existing restart-deferral lifetime. No updater backend behavior is changed.

## Unfinished acceptance and capabilities

Full RegexBuddy parity is **unfinished**. [The native ledger](native-regex-parity.json) retains all 93 official online topics and the existing complete PDF bookmark inventory, including its original inventory limitations. No row is complete. Captures and capture navigation, lookaround, backreferences, lazy/count/possessive quantifiers, Unicode properties, cross-engine comparison/conversion, debugger, benchmark, code/templates, saved collections, file GREP, automation and comprehensive preferences remain unavailable. Unsupported syntax produces a diagnostic rather than silently claiming another flavor. Detailed parsed explanations, match highlighting beyond selection, result exports, reusable cases, per-field persisted history/snippets, bilingual mode and complete localization remain unfinished.

The native layout uses DPI-scaled controls, existing light/dark palettes and vertical scrolling at reduced height. Intended minimum client area is 640 by 400 logical pixels. Full keyboard focus-scroll, accessibility, high contrast, all language/theme/scale combinations and visual acceptance have not been exercised. This expedited pass runs no tests, independent review, audit, runtime interaction or screenshots. Source, compilation and packaging do not establish runtime correctness.

## 廣東話摘要

原生工作區、設定同每層選單都有自己嘅搜尋狀態。搜尋旁邊嘅 Regex 按鈕會開啟工作台；預覽成功後再套用，取消會保留原有篩選。搜尋、純文字取代同分割只喺記憶體處理，唔會改檔案。背景工作有輸入、步數同時間上限，舊結果唔會套用到新請求。現時只係有明確限制嘅 NFA v1，完整 RegexBuddy 功能、三語模式、無障礙同實機驗證仍未完成。

## Source-bound build result

Exact root `build-installer.bat -Version 1.0.9006` (including `build.bat`) passed at `265f048da7cb7534cb151519d003c67218a73048` with exit 0, 114 warnings and zero errors. [The build receipt](native-build-receipt.json) records SHA-256 values and sizes. The package is local only, not installed or published. The preceding source candidate failed compilation on an unqualified window-rectangle lookup, corrected in this producer commit. No runtime or full-parity claim follows from compilation.

## Modeless source continuation

The modeless lifetime is caller-owned through `NativeRegex::Editor`. The editor owns its draft
and async session, never a pointer to mutable owner options. Owner closure cancels and destroys
it before callbacks may run. Successful Apply destroys the editor before invoking its copied
callback, preventing owner edit notifications from recursively destroying a live callback target.
The current corpus is fetched at Apply and compared again on completion. No engine syntax or
execution bounds changed. The existing updater visible-window check continues to defer restart
while this owned tool window is present; the updater backend was not modified.

Version 1.0.9008 compilation/package evidence is recorded separately. Earlier 1.0.9006
receipts remain bound to their original source and do not verify the modeless continuation.
No tests, reviews, audits, runtime interaction or screenshots ran. Modeless runtime acceptance,
all focus/keyboard/accessibility/DPI/theme/language combinations and full parity remain unfinished.

## Modeless package production result

Exact root `build-installer.bat -Version 1.0.9008`, including `build.bat`, passed at `97a3cd2fb1866adb9bb412c8a7a421acf466ac1b` with exit 0, two linker warnings, zero errors and 9.18 seconds native compilation. Hashes and sizes are in `docs/features/search/native-modeless-build-receipt.json`. This is a local candidate, not an installed or published release. Later documentation commits do not relabel the producer.

## Named local expressions

The modeless editor now includes explicit local Save new, select/Load draft, Rename, Delete, Refresh and Recover backup controls. See [collection behavior, storage and limits](native-collections.md). Loading affects only the draft; Apply retains current-corpus validation. Full collection compatibility and runtime acceptance remain unfinished.
