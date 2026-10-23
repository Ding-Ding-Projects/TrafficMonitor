# Shared status-service hardware displays

Requirement ID: `tidbyt-displays`. Status: **NOT_APPLICABLE**. Phase: **P5**.

## Required outcome

This requirement describes the shared status-service implementation. Client products must register and show truthful status; they do not duplicate the service bridge, owner-managed incident delivery or registered hardware-display infrastructure.

## Current evidence and limits

Registered hardware delivery is owned by the shared status service; client status integration remains required.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| documentation-companion | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| build-companion | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| repository | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | Registered hardware delivery is owned by the shared status service; client status integration remains required. |

## Integration and ordering

Proposed destination: `TrafficMonitor/TrafficMonitor.cpp; TrafficMonitor/TrafficMonitorDlg.cpp; feature-owned native module to be designed`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| documentation | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| localization | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| persistence | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| focusedTests | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| negativeRegression | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| builtInteraction | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |
| screenshots | NOT_APPLICABLE | Not applicable under the documented scope: Registered hardware delivery is owned by the shared status service; client status integration remains required. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
