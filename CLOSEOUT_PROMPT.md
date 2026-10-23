# Native Settings implementation checkpoint

The native C++/MFC redesign is in progress. The reviewed design is under `design/native-modern-remake/`; its final offset correction is commit `0c93dd2bbc09663d46f669046e0ca3503edec9a4`. The full redesign is not complete.

## Verified baseline

Package 1.0.9029 was produced by the exact root `build-installer.bat -Version 1.0.9029` at source `42ed34298ea58f36cef605035962ad8abb4a3279`, from 2026-10-23T19:23:11.4353693Z to 2026-10-23T19:23:28.9524007Z, exit 0. Executable SHA-256: `2AFE7832C9279C1E40EF609FE2CDA9F47A230B651139F5CF025671B9415E4BCE`. Full package SHA-256: `28CA3E25016F63059F4CEF27A0299022E9E925E093B6CA619C7D2A50CA707734`.

The production INI helper extraction was integrated as `a3241f7b9284329b5ff2091411b892d30da2fa36`. At `a8fc568c6f47a1b2606bc1eb31408bdd615f03cf`, the exact root preference verification passed 23 storage/draft cases and 24 production serialization cases, with the deliberate mutation failing one case and restoration passing all 23. Later changes are presentation-only; these results do not prove UI behavior.

## Runtime evidence and rejected visuals

The bounded hidden-desktop portable 9029 run proved the Interface query `appearance` shows one semantic control and hides language, an empty query shows two, first-open provenance says defaults, bilingual and dark preference application updates Settings, and the custom scrollbar End key reaches the preview. It did not prove the full language/theme/DPI matrix, UI Automation, IME, edit-menu actions, every settings group, or all five consumers.

The 96-DPI bilingual minimum view is rejected: the 24-pixel heading had a fixed 40-pixel box, cutting its second line; the multiline search hint was concatenated in a single-line edit; section grouping and help hierarchy did not match the approved design. The image named `settings-interface-en-normal.png` is rejected as mislabeled because it actually shows Floating monitor. The subsequent `settings-interface-en-normal-actual.png` is the actual Interface view. Raw evidence remains retained separately; no image promotion is claimed.

The fixture closed normally. Existing installation 1.0.45 was restored through its stable launcher at 2026-10-23T19:25:46.560964Z, PID 15824, creation 2026-10-23T19:25:46.547496Z, executable SHA-256 `e7c58493da6cfc6953b1ead55b004b5acccec0f2402c8c85e5ae999e849a3f02`. Data and startup comparisons before restoration both passed. Process identity is historical evidence, never permission to reuse that PID.

## Next candidate

Version 1.0.9030 is reserved. The current repair adds measured heading/search origins, compact localized search hints, correctly localized bilingual group counts, reusable section panels and explanatory text, and an explicit keyboard edit-menu route. Build, package and repeated identical runtime tuples remain pending for this source. Preserve source, index and refs throughout each producer and fixture. Rediscover current installation identity before any next run and restore it normally afterwards.

The next work remains native Settings verification followed by workspace, setup, menus, regex and every inventoried legacy owned surface. Default scrollbar and native combo replacements outside this milestone remain unfinished. Preserve the preference storage, stale-draft, protected-schema and restart contracts. Documentation-site and companion implementation remains deferred. Main integration, publication and cleanup belong to the coordinating task.
