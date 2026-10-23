# Built-in authenticator

Requirement ID: `authenticator`. Status: **UNFINISHED**. Phase: **P3**.

## Required outcome

Every user-facing application ships its own authenticator as an ordinary tabbed, searchable, palette-reachable destination for arbitrary TOTP secrets, registered by `otpauth://` URI, by QR from an image file or the clipboard, by camera where available, or by manual base32. Codes are standards-exact RFC 6238 over RFC 4226 with SHA-1, SHA-256, and SHA-512, 6 to 8 digits, and arbitrary periods verified against the published vectors, shown large with copy, a readable countdown, and a next-code peek, and a skewed clock is reported in plain words. Everything stays local in the operating-system credential vault, ordinary exports omit secrets and say so, and a secrets export sits behind super confirmation.

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

Proposed destination: `Isolated credential, history and record services; shared native dialogs and command dispatch`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: `language`, `accessibility-sizing`, `progress-recovery`, `super-confirmation`, `local-history`

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | UNFINISHED | Every user-facing application ships its own authenticator as an ordinary tabbed, searchable, palette-reachable destination for arbitrary TOTP secrets, registered by `otpauth://` URI, by QR from an image file or the clipboard, by camera where available, or by manual base32. Codes are standards-exact RFC 6238 over RFC 4226 with SHA-1, SHA-256, and SHA-512, 6 to 8 digits, and arbitrary periods verified against the published vectors, shown large with copy, a readable countdown, and a next-code peek, and a skewed clock is reported in plain words. Everything stays local in the operating-system credential vault, ordinary exports omit secrets and say so, and a secrets export sits behind super confirmation. |
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
