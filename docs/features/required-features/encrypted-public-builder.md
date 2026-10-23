# Encrypted public builder for private repositories

Requirement ID: `encrypted-public-builder`. Status: **NOT_APPLICABLE**. Phase: **P5**.

## Required outcome

Private repositories build and release through the `encrypted-public-builder` skill: a gibberish-named public builder holds only an encrypted, name-free payload behind opaque per-project ids, releases publish only back to the private repository, and the private source keeps `.github/workflows` absent. No public location ever reveals the private repository's name, products, build details, or release target. An unavailable builder is an exact blocker, never a fall back to an unencrypted installer.

## Current evidence and limits

This source repository is public; private-source encrypted transport is not its build route.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |
| documentation-companion | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |
| build-companion | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |
| repository | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | This source repository is public; private-source encrypted transport is not its build route. |

## Integration and ordering

Proposed destination: `build.bat; build-installer.bat; scripts/; .github/workflows/; feature documentation and release receipts`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |
| documentation | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |
| localization | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |
| persistence | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |
| focusedTests | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |
| negativeRegression | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |
| builtInteraction | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |
| screenshots | NOT_APPLICABLE | Not applicable under the documented scope: This source repository is public; private-source encrypted transport is not its build route. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
