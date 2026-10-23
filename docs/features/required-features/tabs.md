# Browser-style tabbed navigation

Requirement ID: `tabs`. Status: **UNFINISHED**. Phase: **P1**.

## Required outcome

Each repository has one top-level workspace tab, reselect focuses it, and ordinary menu visits never add tabs. Utility tabs require an explicit New tab catalog choice from every menu option, setting, tool and destination, with available official icons, independent plain-text search and an adjacent full regex builder, preserving workflows and drafts.

Every user-facing application and GitHub Pages presents content as browser-style tabs whose strip docks to any edge, left by default, with axis-correct accessibility, an overflow surface, reordering, first-class pinning and grouping, decoratable groups, and persistence of order, pins, groups, and collapsed state across restarts. Settings, properties, and appearance surfaces are tabbed too, with the whole tab feature set, in addition to their own search. Every application ships the four tab-discovery searches, a **Move… into group…** picker instead of inline move lists, and the **Close tabs containing text** and **Close tabs not containing text** actions with previews, pinned exclusion, and regex builders.

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

Proposed destination: `TrafficMonitor/TrafficMonitor.cpp; TrafficMonitor/TrafficMonitorDlg.cpp; feature-owned native module to be designed`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | UNFINISHED | Each repository has one top-level workspace tab, reselect focuses it, and ordinary menu visits never add tabs. Utility tabs require an explicit New tab catalog choice from every menu option, setting, tool and destination, with available official icons, independent plain-text search and an adjacent full regex builder, preserving workflows and drafts.  Every user-facing application and GitHub Pages presents content as browser-style tabs whose strip docks to any edge, left by default, with axis-correct accessibility, an overflow surface, reordering, first-class pinning and grouping, decoratable groups, and persistence of order, pins, groups, and collapsed state across restarts. Settings, properties, and appearance surfaces are tabbed too, with the whole tab feature set, in addition to their own search. Every application ships the four tab-discovery searches, a **Move… into group…** picker instead of inline move lists, and the **Close tabs containing text** and **Close tabs not containing text** actions with previews, pinned exclusion, and regex builders. |
| documentation | UNFINISHED | This article specifies applicability only. Complete behavior, actual defaults/configuration, errors, security and verification plus a Cantonese counterpart and internal navigation remain required. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Declare state schemas, storage, migration, reset and privacy ownership; verify restart/reload, interrupted writes and invalid stored state. Individually explain genuinely stateless boundaries. |
| focusedTests | UNFINISHED | Feature-specific semantic success, invalid-input, cancellation, offline, permission and privacy cases for the real consumer; repository-only requirements verify source and stored provider readback. |
| negativeRegression | UNFINISHED | Remove each of four independent searches, New tab catalog destination, overflow, group picker, pinned/locked exclusion or persisted group state. Verify explicit creation, stable workspace identity, reorder, grouping and unsaved-work protections. |
| builtInteraction | UNFINISHED | Exercise the actual source/version/hash-bound build on each applicable surface, including actions, results and persistent state. A design preview, static example or source scan supplies no runtime proof. |
| screenshots | UNFINISHED | Retain genuine inspected privacy-reviewed action states with exact source/build hashes, dimensions, scale, theme, language, screen and route. Nonvisual repository rows use exact source/provider receipts, never invented images. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
