# Native Settings implementation checkpoint

The native C++/MFC redesign is in progress. The reviewed design is under `design/native-modern-remake/`; its final offset correction is commit `0c93dd2bbc09663d46f669046e0ca3503edec9a4`. The full redesign is not complete.

## Verified baseline

Package 1.0.9029 was produced by the exact root `build-installer.bat -Version 1.0.9029` at source `42ed34298ea58f36cef605035962ad8abb4a3279`, from 2026-10-23T19:23:11.4353693Z to 2026-10-23T19:23:28.9524007Z, exit 0. Executable SHA-256: `2AFE7832C9279C1E40EF609FE2CDA9F47A230B651139F5CF025671B9415E4BCE`. Full package SHA-256: `28CA3E25016F63059F4CEF27A0299022E9E925E093B6CA619C7D2A50CA707734`.

The production INI helper extraction was integrated as `a3241f7b9284329b5ff2091411b892d30da2fa36`. At `a8fc568c6f47a1b2606bc1eb31408bdd615f03cf`, the exact root preference verification passed 23 storage/draft cases and 24 production serialization cases, with the deliberate mutation failing one case and restoration passing all 23. Later changes are presentation-only; these results do not prove UI behavior.

## Runtime evidence and rejected visuals

The bounded hidden-desktop portable 9029 run proved the Interface query `appearance` shows one semantic control and hides language, an empty query shows two, first-open provenance says defaults, bilingual and dark preference application updates Settings, and the custom scrollbar End key reaches the preview. It did not prove the full language/theme/DPI matrix, UI Automation, IME, edit-menu actions, every settings group, or all five consumers.

The 96-DPI bilingual minimum view is rejected: the 24-pixel heading had a fixed 40-pixel box, cutting its second line; the multiline search hint was concatenated in a single-line edit; section grouping and help hierarchy did not match the approved design. The image named `settings-interface-en-normal.png` is rejected as mislabeled because it actually shows Floating monitor. The subsequent `settings-interface-en-normal-actual.png` is the actual Interface view. Raw evidence remains retained separately; no image promotion is claimed.

The fixture closed normally. Existing installation 1.0.45 was restored through its stable launcher at 2026-10-23T19:25:46.560964Z, executable SHA-256 `e7c58493da6cfc6953b1ead55b004b5acccec0f2402c8c85e5ae999e849a3f02`. Data and startup comparisons before restoration both passed. Detailed process identity remains in the private runtime receipt and must be rediscovered before another run.

## Next candidate

Version 1.0.9030 is reserved. The current repair adds measured heading/search origins, compact localized search hints, correctly localized bilingual group counts, reusable section panels and explanatory text, and an explicit keyboard edit-menu route. Build, package and repeated identical runtime tuples remain pending for this source. Preserve source, index and refs throughout each producer and fixture. Rediscover current installation identity before any next run and restore it normally afterwards.

The next work remains native Settings verification followed by workspace, setup, menus, regex and every inventoried legacy owned surface. Default scrollbar and native combo replacements outside this milestone remain unfinished. Preserve the preference storage, stale-draft, protected-schema and restart contracts. Documentation-site and companion implementation remains deferred. Main integration, publication and cleanup belong to the coordinating task.

## 9030 bounded result

The exact root installer entrypoint produced version 1.0.9030 at source `3971e1886c40cb157998d1be6a67f05508a94190`, from 2026-10-23T19:27:28.4087417Z to 2026-10-23T19:27:47.6894061Z, exit 0. Executable SHA-256: `1AE2352A1124D33403703ED6A244D0B2465B14E35CBFB1A81EE985BF1D21A80A`. Package SHA-256: `E46EFD93E1DA51A92A1212FD22AB3A723ED02C56A0EA630839385DEC8A21499D`.

Real 96-DPI captures show the complete bilingual heading, compact search hint, translated group count and grouped preference sections. Home, Page Down, Page Up and End moved the body through bounded offsets 0, 236, 0 and 446; one downward wheel detent moved 96 units. End exposed the preview. These are narrow interaction results, not full accessibility or visual approval.

The focused language selector remains partially outside the minimum viewport after language application; focus visibility after layout changes requires repair and an explicit settled-focus measurement. The edit-action popup still uses the older menu field/button rendering and remains part of the unfinished Settings workflow. NativeMenus needs the bounded shared-component update before this milestone can be accepted. The private drag driver reused its deadline variable and ended the fixture early after sending drag input; drag geometry and the all-groups sweep are unverified, not passed.

The fixture closed normally and restored installation 1.0.45 at 2026-10-23T19:29:04.122401Z through the stable launcher. Data and startup comparisons passed, restoration returned zero, and the installed executable hash remained unchanged. Full process identity is retained only in the private receipt. No fixture remains active. Continue with focus visibility, menu component styling and readable formatting of touched component logic, then produce a newly reserved exact package and repeat the relevant tuples. Do not claim the full redesign complete.

## 9031 source checkpoint

The next bounded candidate replaces the Settings-owned menu search field and Back/Regex buttons with shared native components, retains menu-specific accessibility and checked/disabled semantics, and adds a proportional custom rail to the native variable-height list. Popup height follows measured content so three fitting options do not show an overflow rail. Keyboard traversal includes a visible rail. Settings preference, size and DPI changes request a fresh focused-control visibility check. Version 1.0.9031 is reserved; production and runtime results are pending. Provider lifetime and enabled-state review, UI Automation, IME, drag capture loss, every settings group, and the complete language/theme/scale matrix remain unverified.
