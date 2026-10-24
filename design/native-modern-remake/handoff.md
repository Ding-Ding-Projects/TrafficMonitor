# Implementation handoff

Current visual refinement: [release 48 refinement](release48-refinement.md), based on source `eae4d77243e53cc25df57ccb9ef6f9a665a33562`. Its visual decisions supersede conflicting earlier dimensions below; historical evidence and behavior contracts remain unchanged. Independent review and native runtime acceptance are pending.

Status: design only, independent review pending. Preserve source `f1c6cc59f254ebf37503115636765619b3b35914`, existing storage/migration corrections and raw 1.0.9028 evidence. No new visual implementation is authorized by this document itself.

## Production boundaries

1. `NativeMaterial/Theme.*` and `Controls.*`: extend shared palette/typography, button variants, measured labels, focus and accessibility. Separate layout primitives from painting and native input. Preserve same-HWND input/provider binding and rendering diagnostics. Add reusable navigation, field section and domain action bar only where at least two owners actually share behavior.
2. `NativePreferences/Editor.*`: explicit font/layout context, compact task sections and preview; support semantic row visibility controlled by the owning search. Retain stable IDs, independent NativeMenus options, TryApply expected base, subscriptions and persistence. Do not move storage into controls.
3. `NativeSettings/Settings.cpp`, `SettingsViewport.h`, `SettingCatalog.*`: navigation column and measured header/body/footer, truthful filtered result count and actual row filtering, correct focus reveal, localized stateful footer. Retain all current setting consumers, reset semantics, validation and restart deferral.
4. `NativeDashboard/Workspace.*`, `Presentation.*`: productive monitoring composition, existing actions/data destinations, adaptive chart/list arrangement and readable status. Preserve sampler, chart history, adapter identity, monitoring, tray/taskbar, plugins and lifecycle.
5. `NativeDashboard/SetupFlow.cpp`, `SetupFlowText.h`: shared shell/step list and field sections, existing progress/action controls. Preserve exact SetupState serialization and protected-close contract.
6. `NativeMenus/SearchableMenu.cpp`: compact search/results, measured bilingual rows, native checked and focus states, retained independent option states. Preserve opaque/plugin labels and command dispatch identity.
7. `NativeRegex/Workbench.*`: task-oriented editor layout, real stateful tabs and responsive panels. Map every current control into the new organization before removing its previous placement. Preserve engine, worker generations, collection persistence and owner-search lifecycle.

Recommended coherent milestones: shared visual components and Settings; workspace and setup; menu and regex; bounded runtime corrections and evidence. Each milestone needs an exact source commit and root-entrypoint build. No producer's source/index/refs change while it runs. The broader required-feature inventory and documentation delivery remain separate work.

## Remaining legacy surface inventory

Original resource-based Options tabs, traffic-history dialogs, plugin manager and plugin-owned dialogs, adapter/resource pickers, notification/message dialogs, tray/taskbar and floating monitoring presentation remain real existing surfaces. The workspace may route to them but that does not modernize them. Preserve all commands and data while recording each route's current visual state. A claim that the entire native application was remade requires a separately reviewed extension covering these surfaces, not merely the five shared owners. Startup/updater/native OS consent surfaces retain their supported lifecycle and identity.

The user's full-remake request makes the remaining application-owned surfaces mandatory subsequent milestones, not optional exemptions: original Options and all its tabs; traffic history and details; plugin manager; About/version and diagnostic details; adapter/resource pickers; application-owned notifications, confirmations and validation dialogs; and compact floating/taskbar presentation. Inventory actual reachable dialogs and every existing control before each milestone. Plugin-owned third-party windows and OS-owned consent are distinct ownership boundaries: preserve their identity and behavior, disclose them explicitly and do not claim their appearance was changed. The first five-owner milestone cannot close the full request.

## Window frame decision

Keep the real resizable Win32 window frame and native caption buttons, system menu, snapping, drag/resize hit testing and accessibility. Set supported DWM light/dark caption attributes from effective appearance; align caption color with the shared canvas where the host supports it. The design SVGs depict client areas only, so they are not evidence of titlebar styling. Do not paint duplicate fake caption buttons or remove standard window semantics. Record the actual supported DWM behavior per host; an unsupported caption-color attribute is a named OS boundary for review, not a claim of full custom chrome. All application-owned client controls and scrollbars remain subject to the replacement registry. Owner-drawn nonclient scrollbars are replaced with accessible client rails, not merely recolored through DWM.

## Verification and evidence

Build and package only through root build.bat and build-installer.bat. The existing standalone storage checks have 23 passing cases; the additional production serialization console target remains unverified because its real Common.cpp linkage has unresolved symbols. No invented stubs or repeated unchanged linker attempts. Actual wizard initialization, serialization and restart must be exercised in the built application.

The expanded serialization target never executed: its build route exited 2 during linking. Having calls to production functions in its source is not execution evidence. Preserve that exact limitation until a real target or the actual application proves the behavior.

First reproduce the rejected Interface count/filter/font defects against the old package if further evidence is necessary, then compare the new exact tuple. Obtain native child rectangles, font/DPI information, scroll ranges, focus/provider state and genuine rendered screenshots. Generic browser computed-style counters cannot be claimed for MFC. Retain raw receipts and explicitly report any unsupported generic promotion schema.

Run normal/minimum, English/Cantonese/bilingual, light/dark at 96/120/144/192 DPI for every owner and the decisive states in states.md. Separate design references from built images and preserve old evidence as rejected history. Bind screenshots to exact package/executable hash, source, state, client dimensions, DPI, route and privacy verdict. Check text/button/label/status bounds, reading order, keyboard reachability, selected semantics, contrast and high contrast. Do not call a 96-DPI image proof of higher scales.

Use only the authorized hidden desktop route. Rediscover the installed baseline before each run. Record state, close normally only when safe, use genuine portable mode without environment redirection, close only owned fixtures normally and restore through the installed stable launcher. Verify data/startup and exact restored process identity. Do not force-close, change visible focus, change host power/login or alter sound settings.

## Review checklist

- [ ] Independent reviewer accepts all five compositions and normal/minimum bilingual references.
- [ ] Existing command/control inventory has no dropped destination or silently unsupported behavior.
- [ ] Component API and explicit font lifetime remove the observed tiny-text cause.
- [ ] Global Interface filter actually filters both semantic settings and the count agrees.
- [ ] Storage, migration, conflict, protected close and subscription contracts remain unchanged or receive separate review.
- [ ] Built matrix and keyboard/accessibility evidence pass, or every missing tuple is explicitly unverified.
- [ ] Full-application claims wait for remaining legacy-surface scope and evidence.
