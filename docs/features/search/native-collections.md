# Named native regex collections

The owned native workbench can keep explicit named expressions locally and reuse them from workspace, Settings and searchable-menu editors. Each owning search retains its independent draft and accepted filter. This is a staged collection feature, not full RegexBuddy library compatibility.

## Save and reuse

1. Enter a pattern or literal query, choose case behavior, and choose Search, Replace preview or Split preview. The last selected operation and literal replacement are included in the saved expression.
2. Enter a unique collection name and choose **Save new**. Syntax is validated asynchronously before storage. An existing name is rejected instead of overwritten. Saving does not apply the owner filter.
3. Select a saved name with the native dropdown and choose **Load draft**. Merely selecting a name does nothing to the pattern. The complete saved record and supported syntax are validated before draft replacement. The loaded operation is shown and previewed against the current local sample, which was never saved.
4. Choose **Apply to search** separately. The existing asynchronous validation uses the current owner corpus and rechecks it before accepting the filter. Cancel or Escape leaves the accepted filter unchanged.

Rename uses the selected record and the name field. Delete requires a second explicit press and leaves the editor draft unchanged. Refresh reads the latest shared list. Another editor changing a selected record causes rename/delete to fail without overwriting that change; refresh before retrying. A confirmed storage transaction may finish after the editor closes because Save was explicit; closing does not undo an already requested durable save.

## Local storage and recovery

Only name, pattern, literal/regex mode, supported case flag, operation, replacement and exact engine/version binding are persisted. Subjects, owner corpus, match output and implicit search history are never stored. There is no networking or diagnostic output containing expression data.

The store follows the application's existing configuration directory: installed mode uses its persistent application-data directory outside versioned installation folders; portable mode follows its selected portable data directory. Files are `regex-collections.dat`, its previous valid `.bak`, an empty coordination `.lock`, and a temporary transaction file while writing. Storage runs off the UI thread. A nonblocking exclusive file handle serializes cooperating editors and processes. Contention reports a retryable message rather than waiting on the UI thread.

A successful save flushes a temporary file and atomically replaces the primary, retaining the previous file as backup. Interrupted temporary files are not read as collections. Missing primary means an empty collection; malformed or inaccessible primary is reported and never silently replaced. **Recover backup** requires a second explicit press, validates the complete backup and atomically restores it without applying any search. It preserves the valid backup. A missing or invalid backup produces an honest failure and leaves the draft unchanged. Filesystem failures may require checking access and retrying or explicit recovery.

## Format and limits

This native format is not an interchange format. All integers are unsigned 32-bit little-endian. Header words are `0x58524d54`, `0x314c4f43`, schema version `1`, engine version `1` (TrafficMonitor bounded NFA v1), and record count. Each record contains length-prefixed UTF-16LE name, pattern and replacement, flags (bit 0 regex, bit 1 ASCII ignore-case), and operation (`1` Search, `2` Replace, `3` Split). Unknown versions, flags, operations, duplicates, trailing data and truncated fields reject the complete file. No partial list is accepted from a malformed file.

Limits are 64 records, 96 UTF-16 code units per nonblank name, 256 pattern units, 4096 replacement units and 1 MiB per file. Names compare ordinally without case; spelling and normalization are otherwise preserved. Valid Unicode, including supplementary characters, round-trips without encoding substitution. Embedded NUL and malformed surrogate pairs are rejected because native text controls cannot represent those strings faithfully. There is no Unicode normalization or implicit trimming.

## Remaining work and evidence

Import/export compatibility, descriptions, parameters, stored test subjects, multi-collection management, additional engines and full RegexBuddy parity remain unfinished. The supported engine remains bounded NFA v1 with its existing limitations. Native controls provide keyboard navigation and bilingual action labels; diagnostics currently include English-only storage and engine messages. Full localization, screen-reader behavior, display matrices, actual Unicode persistence/recovery and concurrent-editor acceptance have not been exercised in the current expedited mode. No tests, independent review, audits, runtime interaction or screenshots were performed. Build receipts prove only their exact production sources.
