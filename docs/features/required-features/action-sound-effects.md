# Action sound effects

Requirement ID: `action-sound-effects`. Status: **UNFINISHED**. Phase: **P2**.

## Required outcome

Provide short coherent sound effects for selected meaningful actions and actual outcomes through one non-blocking playback service. Each user-facing surface has its own visible localized keyboard-accessible Settings switch, applied immediately and persisted across restart. Honor existing mute, volume, reduced-sound and quiet-hours choices without changing host audio settings. Prevent repetitive or overlapping cues, preserve complete visible results and assistive announcements, and isolate unavailable audio from the underlying action result. Success cues occur only after success. Personal-wording import outcome cues use the same controls.

## Current evidence and limits

The native application now has a source-bound action-to-cue map, a shared non-blocking playback service and a persisted Settings control (see "Native application implementation" below). Built audible playback has not been observed, and the hosted-documentation, documentation-companion and build-companion surfaces remain unimplemented. This article records implemented source behavior, not runtime acceptance. Shipped default: action sounds are Off.

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

## Native application implementation

Source: `TrafficMonitor/NativeMaterial/ActionSounds.h` and `ActionSounds.cpp`, with the preference in `TrafficMonitor/NativePreferences/`. The `native-application` surface is the only surface touched; its status remains UNFINISHED until a built audible observation is recorded.

| Action | Where it fires | Cue | Windows alias |
|---|---|---|---|
| Settings applied successfully | Interface page Apply and every other Settings page Apply, after the value is stored | Success | `SystemAsterisk` |
| Setup completed | Setup wizard, after every step is applied and saved | Success | `SystemAsterisk` |
| Action refused or failed | Apply rejected (invalid number, conflict, protected file, write failure, uncertain write, automatic restart choice not saved) | Failure | `SystemHand` |
| Search produced no results | Searchable menu, once when a typed query first yields an empty list (not on every keystroke) | Notice | `SystemExclamation` |

Control: Settings, Interface page, "Action sounds" (English, Cantonese or bilingual label), with three levels: Off, Failures and refusals only, All actions. The row is keyboard reachable like the language and appearance selectors, takes part in Settings search, and is covered by the page Reset and "Use saved" commands. The choice is applied with the page Apply command, which is the same moment it is persisted.

Storage: `native-interface.ini` in the shared preference store. Off writes the original schema 1 document, so older builds and existing files keep working and an existing file loads with sounds Off. A non-Off level writes schema 2 with exactly one `actionSounds=1` (Failures and refusals only) or `actionSounds=2` (All actions) line. Out-of-range, malformed, duplicated or non-canonical values leave the file protected and are never partially applied.

Behavior and limits:

- Default is Off. Nothing plays until the user chooses a level.
- Only Windows-provided system sounds are used, played asynchronously with `PlaySound` and `SND_ALIAS_ID`. No audio file is bundled and no network is used.
- Windows volume, mute and sound-scheme settings decide what is audible; the application never changes them. There is no separate volume, mute, quiet-hours or reduced-sound control in the application: "Failures and refusals only" is the reduced level.
- Cues closer than 600 ms apart are dropped, so repeated actions never stack or overlap.
- A missing audio device or playback error is swallowed and never changes the action result, blocks input or removes the visible status text.
- Not implemented: personal-wording import cues (not part of this change), quiet hours, per-action switches, and the other three runtime surfaces.

Verification: `build.bat --verify-preferences` covers the default, bounds, storage round trips for all 27 combinations, schema 1 compatibility, malformed-value recovery, service persistence and reset, and the dispatcher decisions with an injected player (Off plays nothing, the mapped cue per action, Important filtering, non-overlap, unavailable device). The negative regression receipt is `evidence/features/action-sound-effects/native-application/negative-regression.json`. Audible playback in the running application has not been observed.

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
