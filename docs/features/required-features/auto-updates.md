# Chrome-style automatic updates

Requirement ID: `auto-updates`. Status: **PARTIAL**. Phase: **P0**.

## Required outcome

Every installed user-facing application checks for updates at startup and on a bounded background schedule, validates the unsigned Squirrel feed metadata and package hashes over HTTPS without interrupting work, stages the update, and shows a persistent non-blocking ready banner with the exact version, a release-note link, an unsigned-artifact warning, **Restart to install update**, and **Later**. Restart happens only on the person's choice with unsaved-work protection. The updater is on by default with a visible state and a manual **Check for updates** command, and an update bug or failure is never hidden behind a spinner.

## Current evidence and limits

A supported Squirrel update preserved configuration and disabled startup. Automatic schedule, staged-ready banner, invalid feed/hash, cancellation, rollback and unsaved-work interaction remain unfinished.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | Installed-product-specific behavior; hosted documentation must explain it without pretending a demonstration executed it. |
| documentation-companion | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| build-companion | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| repository | NOT_APPLICABLE | NOT_APPLICABLE | Runtime behavior belongs to individual product surfaces; repository supplies their source, documentation and verification. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension is delivered; adding one requires its independent full surface inventory. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | No game runtime is delivered by this native monitoring product. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | This public consumer product is not the private instruction source. |

## Integration and ordering

Proposed destination: `build.bat; build-installer.bat; scripts/; .github/workflows/; feature documentation and release receipts`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | PARTIAL | Every installed user-facing application checks for updates at startup and on a bounded background schedule, validates the unsigned Squirrel feed metadata and package hashes over HTTPS without interrupting work, stages the update, and shows a persistent non-blocking ready banner with the exact version, a release-note link, an unsigned-artifact warning, **Restart to install update**, and **Later**. Restart happens only on the person's choice with unsaved-work protection. The updater is on by default with a visible state and a manual **Check for updates** command, and an update bug or failure is never hidden behind a spinner. |
| documentation | UNFINISHED | This article specifies applicability only. Complete behavior, actual defaults/configuration, errors, security and verification plus a Cantonese counterpart and internal navigation remain required. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Declare state schemas, storage, migration, reset and privacy ownership; verify restart/reload, interrupted writes and invalid stored state. Individually explain genuinely stateless boundaries. |
| focusedTests | UNFINISHED | Feature-specific semantic success, invalid-input, cancellation, offline, permission and privacy cases for the real consumer; repository-only requirements verify source and stored provider readback. |
| negativeRegression | UNFINISHED | Remove each asserted implementation registration, article, localized resource, persistence path, semantic test, built interaction or evidence item independently and require rejection; restore and require success. An independent fixed expected list must catch total disappearance. |
| builtInteraction | PARTIAL | Exercise the actual source/version/hash-bound build on each applicable surface, including actions, results and persistent state. A design preview, static example or source scan supplies no runtime proof. |
| screenshots | UNFINISHED | Retain genuine inspected privacy-reviewed action states with exact source/build hashes, dimensions, scale, theme, language, screen and route. Nonvisual repository rows use exact source/provider receipts, never invented images. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
