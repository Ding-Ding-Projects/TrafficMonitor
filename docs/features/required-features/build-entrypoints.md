# Root `build.bat` and `build-installer.bat`

Requirement ID: `build-entrypoints`. Status: **PARTIAL**. Phase: **P0**.

## Required outcome

Every repository carries real working root `build.bat` and `build-installer.bat` entrypoints that take a fresh Microsoft Windows checkout to a built runnable application and to the release-equivalent installer, installing every dependency or prerequisite themselves in user scope, pre-elevating interactively, supporting `/s`, `--silent`, and `SILENT=1`, reporting each phase honestly, staying idempotent, and propagating the real child exit result. Every build and installer production, every manual release included, invokes these exact entrypoints, and the installer script verifies and prints its artifact path and SHA-256 and never publishes, tags, or pushes. A repository with no installable target documents exact nonapplicability with an honest non-success result. Every repository also has a dedicated desktop application build GUI with real Build, Build and run and Build installer actions, automatic trusted pinned/hash-verified dependencies and prerequisites, progress, cancellation and recovery. GitHub Actions publishes its source/version-bound single portable `.exe` in a normal release without an adjacent runtime folder or separate installer.

## Current evidence and limits

Real root scripts produced the native package. Complete fresh-host/silent failure acceptance and the dedicated portable build GUI remain unfinished.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | NOT_APPLICABLE | NOT_APPLICABLE | Repository-owned requirement; each companion still needs its own real runtime actions and evidence. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | Repository-owned requirement; each companion still needs its own real runtime actions and evidence. |
| documentation-companion | NOT_APPLICABLE | NOT_APPLICABLE | Repository-owned requirement; each companion still needs its own real runtime actions and evidence. |
| build-companion | APPLICABLE | UNFINISHED | The dedicated portable build GUI is required independently of repository scripts. Its Build action invokes exact root build.bat, Build and run invokes the supported source-bound build/run route, and Build installer invokes exact root build-installer.bat; verify real child exit status, progress, cancellation and recovery without false success. |
| repository | APPLICABLE | UNFINISHED | Repository delivery, source/evidence or governance responsibility. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension is delivered; adding one requires its independent full surface inventory. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | No game runtime is delivered by this native monitoring product. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | This public consumer product is not the private instruction source. |

## Integration and ordering

Proposed destination: `build.bat; build-installer.bat; scripts/; .github/workflows/; feature documentation and release receipts`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | PARTIAL | Every repository carries real working root `build.bat` and `build-installer.bat` entrypoints that take a fresh Microsoft Windows checkout to a built runnable application and to the release-equivalent installer, installing every dependency or prerequisite themselves in user scope, pre-elevating interactively, supporting `/s`, `--silent`, and `SILENT=1`, reporting each phase honestly, staying idempotent, and propagating the real child exit result. Every build and installer production, every manual release included, invokes these exact entrypoints, and the installer script verifies and prints its artifact path and SHA-256 and never publishes, tags, or pushes. A repository with no installable target documents exact nonapplicability with an honest non-success result. Every repository also has a dedicated desktop application build GUI with real Build, Build and run and Build installer actions, automatic trusted pinned/hash-verified dependencies and prerequisites, progress, cancellation and recovery. GitHub Actions publishes its source/version-bound single portable `.exe` in a normal release without an adjacent runtime folder or separate installer. |
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
