# Local personal-vocabulary JSON upload

Requirement ID: `personal-vocabulary`. Status: **UNFINISHED**. Phase: **P2**.

## Required outcome

Every user-facing application and GitHub Pages shows a visible local personal-vocabulary JSON upload control in its own settings, even before any file exists, with localized no-file, loaded, invalid, replace, and clear or reset states that settings search and the command palette index. One documented, versioned, bounded schema accepts the dynamic current inventory without a hardcoded entry count and is validated in full before display or caching (256 KiB UTF-8, depth 8, keys up to 160 Unicode code points and values up to 1,000), a rejected file never applies partially, clearing purges the cache, and original shipped wording renders until a valid private file arrives. Handling is local-only, so no private vocabulary value, mapping, payload, or file metadata ever reaches source, logs, exports, history, telemetry, prompts, or any public record.

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

Proposed destination: `TrafficMonitor/OptionsDlg.cpp; TrafficMonitor/GeneralSettingsDlg.cpp; isolated settings and presentation services`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: `language`, `accessibility-sizing`, `progress-recovery`

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | UNFINISHED | Every user-facing application and GitHub Pages shows a visible local personal-vocabulary JSON upload control in its own settings, even before any file exists, with localized no-file, loaded, invalid, replace, and clear or reset states that settings search and the command palette index. One documented, versioned, bounded schema accepts the dynamic current inventory without a hardcoded entry count and is validated in full before display or caching (256 KiB UTF-8, depth 8, keys up to 160 Unicode code points and values up to 1,000), a rejected file never applies partially, clearing purges the cache, and original shipped wording renders until a valid private file arrives. Handling is local-only, so no private vocabulary value, mapping, payload, or file metadata ever reaches source, logs, exports, history, telemetry, prompts, or any public record. |
| documentation | UNFINISHED | This article specifies applicability only. Complete behavior, actual defaults/configuration, errors, security and verification plus a Cantonese counterpart and internal navigation remain required. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Declare state schemas, storage, migration, reset and privacy ownership; verify restart/reload, interrupted writes and invalid stored state. Individually explain genuinely stateless boundaries. |
| focusedTests | UNFINISHED | Feature-specific semantic success, invalid-input, cancellation, offline, permission and privacy cases for the real consumer; repository-only requirements verify source and stored provider readback. |
| negativeRegression | UNFINISHED | Reject malformed JSON, duplicate keys, unsafe keys, unsupported versions and oversized/deep input atomically. Verify 256 KiB UTF-8, depth 8, 160-code-point keys, 1000-code-point values, replace/clear, cache corruption and no network/log/export/history/metadata disclosure. |
| builtInteraction | UNFINISHED | Exercise the actual source/version/hash-bound build on each applicable surface, including actions, results and persistent state. A design preview, static example or source scan supplies no runtime proof. |
| screenshots | UNFINISHED | Retain genuine inspected privacy-reviewed action states with exact source/build hashes, dimensions, scale, theme, language, screen and route. Nonvisual repository rows use exact source/provider receipts, never invented images. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
