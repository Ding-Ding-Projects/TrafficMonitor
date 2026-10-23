# A real release on every push

Requirement ID: `release-workflow`. Status: **PARTIAL**. Phase: **P0**.

## Required outcome

Every GitHub project has a GitHub Actions workflow on every `push` and `workflow_dispatch` that builds, packages, and publishes exactly one new, uniquely tagged, non-draft GitHub release carrying the genuinely built installer, and by user's standing decision it runs no tests, lint, or other gating gate. Release notes state which checks actually ran locally and their real results, never describe an ungated release as passing, and publishing creates no automation loop. The active delivery scope is Microsoft Windows only, and GitHub API calls resolve the `RELEASE_TOKEN`, `ORG_TOKEN`, `GITHUB_TOKEN` chain without ever printing it. A private repository keeps `.github/workflows` absent and reaches the same release through `encrypted-public-builder` instead.

## Current evidence and limits

Normal source-bound Stage1 installer releases and successful hosted runs exist. Complete companion delivery and every required release trigger remain unfinished.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | NOT_APPLICABLE | NOT_APPLICABLE | Repository-owned requirement; each companion still needs its own real runtime actions and evidence. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | Repository-owned requirement; each companion still needs its own real runtime actions and evidence. |
| documentation-companion | NOT_APPLICABLE | NOT_APPLICABLE | Repository-owned requirement; each companion still needs its own real runtime actions and evidence. |
| build-companion | NOT_APPLICABLE | NOT_APPLICABLE | Repository-owned requirement; each companion still needs its own real runtime actions and evidence. |
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
| implementation | PARTIAL | Every GitHub project has a GitHub Actions workflow on every `push` and `workflow_dispatch` that builds, packages, and publishes exactly one new, uniquely tagged, non-draft GitHub release carrying the genuinely built installer, and by user's standing decision it runs no tests, lint, or other gating gate. Release notes state which checks actually ran locally and their real results, never describe an ungated release as passing, and publishing creates no automation loop. The active delivery scope is Microsoft Windows only, and GitHub API calls resolve the `RELEASE_TOKEN`, `ORG_TOKEN`, `GITHUB_TOKEN` chain without ever printing it. A private repository keeps `.github/workflows` absent and reaches the same release through `encrypted-public-builder` instead. |
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
