# Universal local file converter

Requirement ID: `file-converter`. Status: **UNFINISHED**. Phase: **P4**.

## Required outcome

Every user-facing application and GitHub Pages ships a real local converter that detects type by bounded byte inspection and offers a categorized, searchable adapter catalogue covering at least Documents/PDF, Images, Audio, Video, Archives, Structured Data/Spreadsheets, Code/Text, and Binary Encodings, with unavailable formats listed as visible disabled rows naming the exact missing dependency or prerequisite. An adapter is enabled only when every dependency or prerequisite is bundled and proven offline, runs sandboxed within bounded resources, and validates its output, and the PDF inspect, split, merge, extract, reorder, rotate, and metadata tools write atomically and reopen to verify. Lossy changes are disclosed first, sources stay untouched, overwrites need super confirmation, and the queue has no artificial file cap yet never loads every path or byte into memory, with pause, resume, cancel, crash recovery, and a storage preflight.

## Current evidence and limits

No complete source, localization, semantic test, built-interaction and evidence set has been established.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| hosted-documentation | APPLICABLE | UNFINISHED | Independent visitor-facing surface. Native execution needs an explicitly authorized local bridge and a truthful unavailable state. |
| documentation-companion | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| build-companion | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| repository | NOT_APPLICABLE | NOT_APPLICABLE | Runtime behavior belongs to individual product surfaces; repository supplies their source, documentation and verification. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension is delivered; adding one requires its independent full surface inventory. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | No game runtime is delivered by this native monitoring product. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | This public consumer product is not the private instruction source. |

## Integration and ordering

Proposed destination: `Dedicated native tools modules with versioned descriptors, sandboxed adapters and exact process ownership, separate from monitoring core`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: `language`, `accessibility-sizing`, `progress-recovery`, `rich-controls`, `guided-forms`, `bundled-dependencies`, `super-confirmation`, `local-history`

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | UNFINISHED | Every user-facing application and GitHub Pages ships a real local converter that detects type by bounded byte inspection and offers a categorized, searchable adapter catalogue covering at least Documents/PDF, Images, Audio, Video, Archives, Structured Data/Spreadsheets, Code/Text, and Binary Encodings, with unavailable formats listed as visible disabled rows naming the exact missing dependency or prerequisite. An adapter is enabled only when every dependency or prerequisite is bundled and proven offline, runs sandboxed within bounded resources, and validates its output, and the PDF inspect, split, merge, extract, reorder, rotate, and metadata tools write atomically and reopen to verify. Lossy changes are disclosed first, sources stay untouched, overwrites need super confirmation, and the queue has no artificial file cap yet never loads every path or byte into memory, with pause, resume, cancel, crash recovery, and a storage preflight. |
| documentation | UNFINISHED | This article specifies applicability only. Complete behavior, actual defaults/configuration, errors, security and verification plus a Cantonese counterpart and internal navigation remain required. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Declare state schemas, storage, migration, reset and privacy ownership; verify restart/reload, interrupted writes and invalid stored state. Individually explain genuinely stateless boundaries. |
| focusedTests | UNFINISHED | Feature-specific semantic success, invalid-input, cancellation, offline, permission and privacy cases for the real consumer; repository-only requirements verify source and stored provider readback. |
| negativeRegression | UNFINISHED | Remove each asserted implementation registration, article, localized resource, persistence path, semantic test, built interaction or evidence item independently and require rejection; restore and require success. An independent fixed expected list must catch total disappearance. |
| builtInteraction | UNFINISHED | Exercise the actual source/version/hash-bound build on each applicable surface, including actions, results and persistent state. A design preview, static example or source scan supplies no runtime proof. |
| screenshots | UNFINISHED | Retain genuine inspected privacy-reviewed action states with exact source/build hashes, dimensions, scale, theme, language, screen and route. Nonvisual repository rows use exact source/provider receipts, never invented images. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
