# Browser-extension download hand-off surfaces

Requirement ID: `download-handoff`. Status: **NOT_APPLICABLE**. Phase: **P5**.

## Required outcome

A browser-extension download capture opens a real **Start download** dialog naming the file, source, destination, and starting action before any transfer, then a distinct IDM-style **Downloading** surface whose controls operate the real transfer, then a non-blocking **Download complete** surface. Start and completion stay always on top until resolved, and none of them claims success before the transfer ends.

## Current evidence and limits

No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required.

Listing is not implementation or acceptance proof. PARTIAL still means unfinished.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| hosted-documentation | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| documentation-companion | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| build-companion | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| repository | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |

## Integration and ordering

Proposed destination: `TrafficMonitor/TrafficMonitor.cpp; TrafficMonitor/TrafficMonitorDlg.cpp; feature-owned native module to be designed`. This is a module proposal, not a statement that these paths implement the feature.

Dependencies: Establish the phase prerequisites in the implementation plan.

## Evidence checklist

| Dimension | Status | Acceptance evidence |
|---|---|---|
| implementation | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| documentation | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| localization | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| persistence | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| focusedTests | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| negativeRegression | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| builtInteraction | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |
| screenshots | NOT_APPLICABLE | Not applicable under the documented scope: No browser extension or extension-origin transfer is delivered. Ordinary download progress and recovery remain required. |

## Configuration, failures and privacy

No unfinished setup action or runtime success is promised. Preserve existing monitoring, plugins, portable behavior, startup choices and saved data. Validate before mutation, disclose exact unavailable capabilities and provide bounded recovery. Credentials and user personal wording never enter logs, ordinary exports or public evidence. Future implementation must replace this planning paragraph with actual configuration, defaults, limitations and verified recovery behavior.

[Inventory](README.md) · [Implementation phases](implementation-plan.md)
