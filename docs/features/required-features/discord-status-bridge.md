# Shared status-service Discord bridge

Requirement ID: `discord-status-bridge`. Status: **NOT_APPLICABLE**. Phase: **P5**.

## Required outcome

This requirement describes the shared status-service implementation. Client products must register and show truthful status; they do not duplicate the service bridge, owner-managed incident delivery or registered hardware-display infrastructure.

## Current evidence and limits

Shared status-service implementation responsibility, not the registering client product.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |
| documentation-companion | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |
| build-companion | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |
| repository | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | Shared status-service implementation responsibility, not the registering client product. |

## Integration and ordering

Proposed destination: `TrafficMonitor/TrafficMonitor.cpp; TrafficMonitor/TrafficMonitorDlg.cpp; feature-owned native module to be designed`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |
| documentation | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |
| localization | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |
| persistence | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |
| focusedTests | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |
| negativeRegression | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |
| builtInteraction | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |
| screenshots | NOT_APPLICABLE | Not applicable under the documented scope: Shared status-service implementation responsibility, not the registering client product. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
