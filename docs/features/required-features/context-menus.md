# A context menu on every element

Requirement ID: `context-menus`. Status: **UNFINISHED**. Phase: **P2**.

## Required outcome

Every user-facing application and GitHub Pages, and every rendered element within it, provides a target-specific right-click context menu plus accessible keyboard and touch equivalents, including **Edit appearance…** and **Lock this element…**. Every context menu and every dropdown opens with a keyboard-focusable filter field and its own anchored regex builder, however short the menu, filtering locally without reordering meaning, hiding a destructive item while its shortcut stays live, or changing any action. Focus lands in the filter on open where the platform allows, arrows move, Enter activates, Escape clears and then closes, focus returns to the opener, and screen readers hear the result count.

Every right-click/context menu is an independently inventoried Material Design 3 surface with its own search bar, adjacent complete regex builder/workbench, keyboard path and actual action results. Search must preserve target identity, permitted actions, focus return and query state. Inventory each contextual surface explicitly; no small-menu exemption is accepted.

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
| implementation | UNFINISHED | Every user-facing application and GitHub Pages, and every rendered element within it, provides a target-specific right-click context menu plus accessible keyboard and touch equivalents, including **Edit appearance…** and **Lock this element…**. Every context menu and every dropdown opens with a keyboard-focusable filter field and its own anchored regex builder, however short the menu, filtering locally without reordering meaning, hiding a destructive item while its shortcut stays live, or changing any action. Focus lands in the filter on open where the platform allows, arrows move, Enter activates, Escape clears and then closes, focus returns to the opener, and screen readers hear the result count. |
| documentation | UNFINISHED | This article specifies applicability only. Complete behavior, actual defaults/configuration, errors, security and verification plus a Cantonese counterpart and internal navigation remain required. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Declare state schemas, storage, migration, reset and privacy ownership; verify restart/reload, interrupted writes and invalid stored state. Individually explain genuinely stateless boundaries. |
| focusedTests | UNFINISHED | Feature-specific semantic success, invalid-input, cancellation, offline, permission and privacy cases for the real consumer; repository-only requirements verify source and stored provider readback. |
| negativeRegression | UNFINISHED | Inventory every context menu and every current presentation reference; remove one required search/workbench route or rebuilt screenshot as applicable and require rejection.  Remove each asserted implementation registration, article, localized resource, persistence path, semantic test, built interaction or evidence item independently and require rejection; restore and require success. An independent fixed expected list must catch total disappearance. |
| builtInteraction | UNFINISHED | Exercise the actual source/version/hash-bound build on each applicable surface, including actions, results and persistent state. A design preview, static example or source scan supplies no runtime proof. |
| screenshots | UNFINISHED | Retain genuine inspected privacy-reviewed action states with exact source/build hashes, dimensions, scale, theme, language, screen and route. Nonvisual repository rows use exact source/provider receipts, never invented images. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
