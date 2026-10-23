# Drop a personal-vocabulary JSON onto the application

Requirement ID: `vocabulary-drop-apply`. Status: **UNFINISHED**. Phase: **P2**.

## Required outcome

Every application, desktop apps and browser extensions included, accepts a personal-vocabulary JSON dropped anywhere on its window, validates it with the same rules as the Settings upload, applies the private wording immediately and persists it locally; an invalid file is refused with a clear message and changes nothing. The Settings upload remains as the fallback route. A successful apply shows a non-blocking full-window level-up celebration whose headline animates in, holds for about 3 seconds and animates out, with the headline taken from the private name inside the imported JSON, or the neutral `Personal wording applied` when the file gives none. Successful saved drops play tada once, invalid drops play the invalid-input sound once and preserve existing data, existing mute and volume controls apply, and reduced motion gets a fade or static notice, screen readers get a polite announcement, and nothing of the payload beyond that headline is shown. Local-only, no-network and original-wording-on-clear boundaries are unchanged, and GitHub Pages sites are not covered.

## Current evidence and limits

No complete source, localization, semantic test, built-interaction and evidence set has been established.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | Explicit whole-window drop celebration excludes hosted documentation; the independent Settings upload remains applicable. |
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
| implementation | UNFINISHED | Every application, desktop apps and browser extensions included, accepts a personal-vocabulary JSON dropped anywhere on its window, validates it with the same rules as the Settings upload, applies the private wording immediately and persists it locally; an invalid file is refused with a clear message and changes nothing. The Settings upload remains as the fallback route. A successful apply shows a non-blocking full-window level-up celebration whose headline animates in, holds for about 3 seconds and animates out, with the headline taken from the private name inside the imported JSON, or the neutral `Personal wording applied` when the file gives none. Successful saved drops play tada once, invalid drops play the invalid-input sound once and preserve existing data, existing mute and volume controls apply, and reduced motion gets a fade or static notice, screen readers get a polite announcement, and nothing of the payload beyond that headline is shown. Local-only, no-network and original-wording-on-clear boundaries are unchanged, and GitHub Pages sites are not covered. |
| documentation | UNFINISHED | This article specifies applicability only. Complete behavior, actual defaults/configuration, errors, security and verification plus a Cantonese counterpart and internal navigation remain required. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Declare state schemas, storage, migration, reset and privacy ownership; verify restart/reload, interrupted writes and invalid stored state. Individually explain genuinely stateless boundaries. |
| focusedTests | UNFINISHED | Feature-specific semantic success, invalid-input, cancellation, offline, permission and privacy cases for the real consumer; repository-only requirements verify source and stored provider readback. |
| negativeRegression | UNFINISHED | Exercise valid, invalid, repeated, muted and reduced-motion drops; prove atomic saved application, one corresponding sound, polite announcement, Settings parity, restart persistence and clear restoration. |
| builtInteraction | UNFINISHED | Exercise the actual source/version/hash-bound build on each applicable surface, including actions, results and persistent state. A design preview, static example or source scan supplies no runtime proof. |
| screenshots | UNFINISHED | Retain genuine inspected privacy-reviewed action states with exact source/build hashes, dimensions, scale, theme, language, screen and route. Nonvisual repository rows use exact source/provider receipts, never invented images. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
