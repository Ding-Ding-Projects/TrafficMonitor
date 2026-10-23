# Native settings discovery and notifications

Source baseline: `c79d49dbee6467d591676b575fb6b2dd615224a6`. This handoff extends the existing native settings and menu designs, not the documentation website. Independent architecture review conditionally approved the scope with the constraints below. Runtime evidence remains pending.

## Workflow

Keep Monitoring, Floating monitor and Taskbar drafts, and add Notifications backed by existing `GeneralSettingData` notification fields. Preserve the `ApplySettings` carrier and `notify_tip` configuration keys. Daily traffic accepts 1..32767 MB/GB, memory 1..100 percent and temperatures 1..120 Celsius. Defaults are explicit: 200 MB, memory and temperature thresholds 80, notifications disabled. Never read uninitialized default numeric members. A build without temperature monitoring displays its unavailable state and preserves the existing temperature preferences unchanged.

Each group owns query, regex mode, flags and validation independently. A separate All groups discovery view searches labels, explanations and formatted draft values, returning named group destinations. Activating a result navigates without applying or deleting drafts. Async callbacks bind the selected group identity, not a mutable current-page pointer. Keep existing bounded regex execution and modeless owner lifetime. Full RegexBuddy parity remains unfinished.

Use a scrollable content body beneath fixed group/search controls and above fixed Apply/Revert actions. Setting rows provide an explanation and explicit default value; provenance is reported as unknown unless the loader provides real source evidence. Right-click and Shift+F10 open target-specific searchable menus for explanation, staging a default and reverting that field to its applied value. Disabled temperature fields do not expose mutating actions. No placeholder appearance/lock action is added.

## Components and states

Use native edit/check/selection accessibility with existing shared Material controls. Keep catalog labels/descriptions separate from editor logic. Preserve invalid numeric drafts until correction or explicit reset/revert. Page reset affects filtered-out entries too. Group switching restores each group's query and scroll position. Search never changes setting values.

States: applied values, modified draft, invalid value, disabled notification with retained threshold, unavailable temperature consumer, local matches, matches in another group, no matches, pending regex, invalid regex, explanation, row default, row revert, group reset, successful apply, discard confirmation, legacy handoff and restart deferral while the editor is open.

## Verification

Compile through root `build.bat` from a fixed source revision. Exercise bounds and no-partial-apply behavior, independent group/search state, keyboard/pointer row identity, actual configuration persistence and reopen/restart, and the legacy consumer. Keep alert firing evidence separate from preference persistence. Verify normal/minimum layout, English/Cantonese/bilingual, light/dark/high contrast and 100/125/150/200 percent scale on the real native build. Record unsupported localization or unavailable runtime tuples honestly. No source inspection or design reference substitutes for runtime accessibility or screenshots.

## Explicit remaining work

This is a bounded notifications and discovery slice, not complete Settings or full required-feature acceptance. Remaining legacy groups, persistent cross-surface language/theme settings, complete regex parity, element appearance/locks and full visual/accessibility acceptance remain unfinished. Existing regex workbenches are already wired to Settings, dashboard and menus despite older handoff prose describing them as absent.
