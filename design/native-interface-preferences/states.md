# States and acceptance inventory

Design-only inventory at source `575c871e0b6a52667bce1b77f69d62634e457455`. Every row is pending implementation and genuine built verification.

| State ID | Action and required result | Preservation assertion |
| --- | --- | --- |
| preferences/default | Open Settings, Language and appearance without saved native file | Read migration/defaults; do not write configuration |
| preferences/draft | Change both selectors | Only sample previews; other windows and accepted state unchanged |
| preferences/applied | Explicit Apply succeeds | Every open native consumer refreshes; restart reads identical pair |
| preferences/revert | Change selectors then Revert | Restore accepted pair; unrelated monitoring draft remains |
| preferences/reset | Reset then Apply | Disclosed default pair only; setup receipts and legacy LANGID unchanged |
| preferences/write-failed | Make selected configuration destination unwritable through a controlled fixture | Accepted pair unchanged, draft retained, no saved claim; Retry works after recovery |
| preferences/readback-uncertain | Replace succeeds but verification fails in storage seam | Explicit uncertain-save state; reload/reconciliation required |
| preferences/malformed | Invalid enum, oversized file, malformed syntax or unknown schema | No partial apply; preserve original; explicit recovery |
| preferences/external-conflict | External file changes between load and apply | No silent overwrite; reload/retry action preserves draft |
| workspace/refresh | Open adapter details, enter search, then apply preferences in Settings | Stable identity, caret, selection, sample sequence and query preserved |
| settings/dirty | Invalid numeric draft plus pending interface draft | Neither domain discards or applies the other; close explains both |
| setup/migration | Valid legacy setup language 0, 1 or 2, no native file | New defaults honor saved wizard choice without changing legacy LANGID |
| setup/resume-conflict | Dirty saved wizard draft and newer accepted preference revision | Choose Use saved or Keep draft; preserve progress and operation receipts |
| setup/apply | Apply pair from step 1 | Shared update only, no adapter/startup/taskbar operation |
| menu/live | Open submenu with filtered rows during preference update | Correct command IDs, shortcut text, selection, focus, state and submenu ownership |
| menu/unknown | Plugin-provided or opaque menu item | Preserve text and native fallback; do not invent translation |
| regex/live | Owned query/replacement and pending preview during refresh | Draft remains; no implicit owner Apply; stale label-dependent result rejected |
| appearance/system | System changes Light to Dark and back | System tracks; explicit Light/Dark do not change; high contrast overrides every mode |
| lifetime/close | Close owner, child editor and menu while notification queued | No dangling callback, use-after-free, wrong-HWND delivery or post-shutdown service access |
| locale/fallback | Missing translation in controlled fixture | Explicit English fallback; no blank accessible name or missing action |
| scope/portable | Release portable directory and installed application-data directory | Use each actual selected config root; no forced portable destination or cross-process broadcast |

## Measurement matrix

For every normal state render all three language modes, Light and Dark at 96, 120, 144 and 192 DPI, at normal and minimum client sizes from references.md. Exercise System as both resolved appearances and high contrast as an additional accessibility state. Record actual client pixel rectangle and DPI; do not confuse logical units with physical pixels or force a minimum larger than the usable work area without reporting it.

Measure each label, secondary line, button, dropdown, result row, footer and ancestor in native client coordinates. Record text extent using the actual selected font/DirectWrite layout, control bounds, visible intersection, scroll range and focused-item visibility. Native rectangles and text metrics are the appropriate evidence here, not browser DOM metrics. Require visible text, no sibling overlap, bounded scrolling, reachable footer and independent search/regex access. Minimum-size failures remain explicit and require layout changes, not a higher undocumented minimum.

Keyboard paths: Tab and Shift+Tab traverse visible enabled controls in reading order; dropdown arrows select without committing; Enter in dropdown/IME must not trigger Apply; explicit Apply button activates with Space/Enter; Escape closes dropdown before dialog; unsaved-close protections persist. Existing Ctrl+L, menu key, Shift+F10 and regex owner shortcuts retain meaning. No new global shortcut or focus stealing. After relocalization keep focus on the same stable control, and scroll it into view only if necessary.

Accessibility: native dropdown role/value, labels and descriptions are exposed; custom action accessible names include the bilingual secondary text once; selected/check/disabled states remain correct; language and save status updates notify access technology without reading editable user content. Focus ring, high contrast, keyboard menu access, per-monitor DPI and reduced-motion behavior are required. No animation is introduced.

## Focused verification plan

Unit seams cover typed enum validation, explicit setup migration precedence, unknown schema, unchanged-load no-write, atomic failure/recovery, reset, external conflict and same-process revision behavior. Use an independently enumerated consumer list, with a negative test that removes one consumer or changes one notification generation and actually fails. Localization completeness includes owned menu commands and every surface's status strings, rather than counting only the new preference labels.

Runtime checks open workspace, Settings and an independent regex editor together; change every language/appearance pair; inspect actual text, colors and accessibility, close/reopen each surface and restart the application. Repeat with pending numeric edits, regex work, disconnected adapter, submenu search and owner destruction. Verify no monitoring resampling/restart and no change to legacy language_id, plugin labels or unrelated saved preferences. Run exact root build entrypoint after independent design approval and implementation. This design pass runs no build and starts no UI.

Evidence must identify candidate commit, executable hash, version, screen/state, language, theme, DPI, actual client dimensions, controlled inputs, route and privacy review. Preserve raw screenshots, layout receipts, same-tuple reference comparison and reviewed deviations. Existing 96-DPI evidence from earlier settings work is historical baseline, not proof of this design or the whole matrix.
