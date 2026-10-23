# Toy locks on every element

Requirement ID: `element-locks`. Status: **UNFINISHED**. Phase: **P3**.

## Required outcome

Every rendered element, tab, tab group, and appearance value can be locked from its own **Lock this element…** command through a non-modal anchored wizard choosing exactly one of six credential mixes: PIN, password, PIN plus password, password plus TOTP, PIN plus TOTP, or password plus PIN plus TOTP. Each lock has its own credential set in the operating-system credential vault with no master credential or implicit inheritance, and a locked element is truly disabled yet opens its own anchored unlock prompt, with no shortcut, palette teleport, or automation hook walking around it. Every lock says it is just for fun, never security, and names deleting the local application-data folder as the recovery.

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
| implementation | UNFINISHED | Every rendered element, tab, tab group, and appearance value can be locked from its own **Lock this element…** command through a non-modal anchored wizard choosing exactly one of six credential mixes: PIN, password, PIN plus password, password plus TOTP, PIN plus TOTP, or password plus PIN plus TOTP. Each lock has its own credential set in the operating-system credential vault with no master credential or implicit inheritance, and a locked element is truly disabled yet opens its own anchored unlock prompt, with no shortcut, palette teleport, or automation hook walking around it. Every lock says it is just for fun, never security, and names deleting the local application-data folder as the recovery. |
| documentation | UNFINISHED | This article specifies applicability only. Complete behavior, actual defaults/configuration, errors, security and verification plus a Cantonese counterpart and internal navigation remain required. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Declare state schemas, storage, migration, reset and privacy ownership; verify restart/reload, interrupted writes and invalid stored state. Individually explain genuinely stateless boundaries. |
| focusedTests | UNFINISHED | Feature-specific semantic success, invalid-input, cancellation, offline, permission and privacy cases for the real consumer; repository-only requirements verify source and stored provider readback. |
| negativeRegression | UNFINISHED | Remove each of six policies and pointer, keyboard, touch, assistive or programmatic activation interception independently. Verify credential isolation, keypad/manual parity, expiry, searches, history omission and recovery. |
| builtInteraction | UNFINISHED | Exercise the actual source/version/hash-bound build on each applicable surface, including actions, results and persistent state. A design preview, static example or source scan supplies no runtime proof. |
| screenshots | UNFINISHED | Retain genuine inspected privacy-reviewed action states with exact source/build hashes, dimensions, scale, theme, language, screen and route. Nonvisual repository rows use exact source/provider receipts, never invented images. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
