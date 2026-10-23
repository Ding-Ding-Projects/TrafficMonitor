# The GitHub Pages linked from the repository

Requirement ID: `homepage-link`. Status: **UNFINISHED**. Phase: **P5**.

## Required outcome

Every public repository application sets its GitHub About website field (`homepage`) to the exact verified deployed GitHub Pages home-page URL; private repositories are exempt from this field because their GitHub Pages is a packaged desktop application rather than a hosted site (`documentation-companion`). Set the field with `gh repo edit --homepage <url>` and link it near the top of the README. GitHub Pages publishing is enabled whenever the project publishes through it, and the URL and base path stay configurable and verified in the built output, so a detached fork publishing under a project path does not 404.

## Current evidence and limits

No complete source, localization, semantic test, built-interaction and evidence set has been established.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | NOT_APPLICABLE | NOT_APPLICABLE | Specific hosted/repository requirement rather than an installed action; relevant content and links remain current. |
| hosted-documentation | APPLICABLE | UNFINISHED | Independent visitor-facing surface. Native execution needs an explicitly authorized local bridge and a truthful unavailable state. |
| documentation-companion | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| build-companion | NOT_APPLICABLE | NOT_APPLICABLE | Specific hosted/repository requirement rather than an installed action; relevant content and links remain current. |
| repository | APPLICABLE | UNFINISHED | Repository delivery, source/evidence or governance responsibility. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension is delivered; adding one requires its independent full surface inventory. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | No game runtime is delivered by this native monitoring product. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | This public consumer product is not the private instruction source. |

## Integration and ordering

Proposed destination: `TrafficMonitor/TrafficMonitor.cpp; TrafficMonitor/TrafficMonitorDlg.cpp; feature-owned native module to be designed`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | UNFINISHED | Every public repository application sets its GitHub About website field (`homepage`) to the exact verified deployed GitHub Pages home-page URL; private repositories are exempt from this field because their GitHub Pages is a packaged desktop application rather than a hosted site (`documentation-companion`). Set the field with `gh repo edit --homepage <url>` and link it near the top of the README. GitHub Pages publishing is enabled whenever the project publishes through it, and the URL and base path stay configurable and verified in the built output, so a detached fork publishing under a project path does not 404. |
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
