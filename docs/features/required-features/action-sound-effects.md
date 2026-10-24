# Action sound effects

Requirement ID: `action-sound-effects`. Status: **UNFINISHED**. Phase: **P2**.

## Required outcome

Provide short coherent sound effects for selected meaningful actions and actual outcomes through one non-blocking playback service. Each user-facing surface has its own visible localized keyboard-accessible Settings switch, applied immediately and persisted across restart. Honor existing mute, volume, reduced-sound and quiet-hours choices without changing host audio settings. Prevent repetitive or overlapping cues, preserve complete visible results and assistive announcements, and isolate unavailable audio from the underlying action result. Success cues occur only after success. Personal-wording import outcome cues use the same controls.

## Current evidence and limits

No source-bound action-to-cue map, shared playback service, persisted Settings switch or built sound verification has been established. This article records required behavior, not implementation or runtime acceptance. No default enabled state is claimed.

## Independent surface applicability

| Surface | Applicability | Status | Exact reason |
|---|---|---|---|
| native-application | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| hosted-documentation | APPLICABLE | UNFINISHED | Interactive documentation is an independent user-facing surface; browser playback restrictions require user-gesture handling and truthful unavailable audio states, not host audio control. |
| documentation-companion | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| build-companion | APPLICABLE | UNFINISHED | Independent user-facing installed surface; universal controls cannot be delegated to another product. |
| repository | NOT_APPLICABLE | NOT_APPLICABLE | Runtime behavior belongs to individual product surfaces; repository supplies their source, documentation and verification. |
| browser-extension | NOT_APPLICABLE | NOT_APPLICABLE | No browser extension is delivered; adding one requires its independent full surface inventory. |
| game | NOT_APPLICABLE | NOT_APPLICABLE | No game runtime is delivered by this native monitoring product. |
| instruction-source | NOT_APPLICABLE | NOT_APPLICABLE | This public consumer product is not the private instruction source. |

## Configuration, recovery and privacy

Keep monitoring, plugins, portable behavior and saved preferences intact. Playback errors must never change action outcomes, block input or suppress visible and assistive feedback. Browser autoplay restrictions and absent devices require truthful unavailable-audio handling. Never change host volume or log personal wording. Valid personal-wording drops play the platform success cue once after validation, apply and persistence; invalid drops play the invalid-input cue once without changing stored wording or starting a success celebration. Both obey the same controls.

## Evidence checklist

| Dimension | Status | Required evidence |
|---|---|---|
| implementation | UNFINISHED | Provide short coherent sound effects for selected meaningful actions and actual outcomes through one non-blocking playback service. Each user-facing surface has its own visible localized keyboard-accessible Settings switch, applied immediately and persisted across restart. Honor existing mute, volume, reduced-sound and quiet-hours choices without changing host audio settings. Prevent repetitive or overlapping cues, preserve complete visible results and assistive announcements, and isolate unavailable audio from the underlying action result. Success cues occur only after success. Personal-wording import outcome cues use the same controls. |
| documentation | UNFINISHED | Document actual action-to-cue mapping, audio asset or synthesis provenance, shipped defaults, platform playback restrictions, controls and failure behavior, with complete translated guidance. |
| localization | UNFINISHED | English, Hong Kong Cantonese and bilingual UI, both independent tone extremes, keyboard and screen-reader text on every applicable surface. Repository-only rows use appropriate source/record evidence rather than invented localized controls. |
| persistence | UNFINISHED | Verify immediate switch changes, restart/reload persistence, upgrade preservation, reset, malformed storage recovery and independent ownership per surface; retain existing mute/volume/quiet-hours choices. |
| focusedTests | UNFINISHED | Verify successful and refused outcomes, disabled and muted playback, reduced-sound and quiet-hours settings, repeated actions, non-overlap, non-blocking execution and unavailable audio without changing action results. |
| negativeRegression | UNFINISHED | Omit the complete action-sound-effects identity from both inventory and expected file and require independent-list rejection; separately remove playback routing, Settings switch, storage, localization, outcome timing or runtime receipts and require rejection before restoring. |
| builtInteraction | UNFINISHED | Exercise selected successful and refused actions, disabled playback, immediate changes, restart persistence, repeated actions and unavailable audio on each actual source/build-bound surface. Audio observations require explicit receipts; screenshots alone cannot prove sound. |
| screenshots | UNFINISHED | Inspect privacy-reviewed built Settings and action states with source/build identity, viewport, scale, language and theme; retain separate audio observation receipts because pixels cannot prove playback. |

## Integration and ordering

Proposed: feature-owned native playback service and GeneralSettingsDlg controls; independent companion and hosted-documentation playback adapters. No implementation is claimed. Native application work precedes documentation implementation. Each runtime surface must supply its own eight proof dimensions and evidence; sibling proof is insufficient.

[Inventory](README.md) ? [Acceptance procedure](acceptance.md)
