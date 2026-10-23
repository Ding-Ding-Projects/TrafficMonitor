# TrafficMonitor commit history

This directory is being populated as an English-language index of the commits reachable from the source repository's `main` branch at the recorded snapshot. Current data covers all 26 fork commits after the upstream baseline, stored verbatim, and 483 upstream commits through 2021-02-21; 904 upstream entries remain to be added. Each entry links to its exact immutable commit and retains the source subject where translation helps readers audit the summary. The list describes recorded changes; it does not claim that a commit was built, tested, or exercised at runtime.

## Data format

`manifest.json` records the source commit, exact entry count, and data-file inventory. Each data file contains an `entries` array. Entries use these fields:

- `sha`: full commit identifier.
- `date`: author date in ISO 8601 format, including its original timezone offset.
- `commitUrl`: direct source commit link.
- `summary`: concise English description for upstream commits. Fork entries instead use `originalMessage` to show the exact bilingual source message.
- `category`: one of `feature`, `fix`, `maintenance`, `documentation`, `translation`, `release`, `merge`, or `revert`.
- `lineage`: `upstream` for commits at or before the recorded upstream baseline, and `fork` for commits after it.
- `originalSubject`: original commit subject. `originalMessage`: exact complete commit message for already-bilingual fork commits.
- `summaryMethod`: `translated-message`, `patch-reviewed`, `ambiguous`, or `verbatim-bilingual`.
- `verificationStatus`: `not-asserted`; history text is not runtime verification evidence.

The renderer can split and filter entries by date, category, and lineage. Keep every reachable source commit exactly once. When refreshing the history, update the source commit and coverage inventory together, then compare the rendered entry SHA set with `git rev-list` for that source revision.

## Snapshot provenance

Initial snapshot: `04a9a136ae800c4b1c547cf09c1bf3688ac0265a` on `main`. The upstream lineage baseline is `930f17533d6098989ebad62f210aa97d75ef174b`; commits descended from later fork work are labeled `fork`. All links point to immutable commit pages in the source project. Coverage is incomplete until the manifest reports 1,413 represented entries and zero remaining entries.
