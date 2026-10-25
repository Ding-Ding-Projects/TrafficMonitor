# First-open native setup

## Start and resume

Choose Open dashboard explicitly. An incomplete setup opens four steps: language and message style, monitoring scope, taskbar/login startup, and review. Ordinary launch and login startup continue monitoring without this wizard. After completion, Open dashboard opens Network. The dashboard context menu contains Resume setup for later changes.

Back retains edits. Next stores progress, while Save and close saves the bounded local draft without applying monitor, taskbar or startup changes. Reopening restores that draft. Completed setup reopened explicitly loads the current monitor choice and initializes both optional changes to Keep current.

## Actual choices

Language and appearance use the shared native preference editor, with English, Cantonese and bilingual choices. Its explicit Apply action persists that preference domain; Next does not silently apply an unsaved preference draft. Independent levels from 1 to 5 change the wizard's supporting message voice while keeping factual results unchanged. Those message-style levels remain scoped to setup. Complete application-wide language and message coverage remains unfinished.

Preferences, their own actions and both message-style ranges now occupy one scrollable step body. Back, Next and Save and close remain in a measured fixed footer. Focus reveals the actual child control, while scrolling the rail itself does not pull the content back toward the rail. The declared reference client is 960 × 760; the supported minimum is 760 × 640. A larger 1280 × 840 observation is additional evidence, not a substitute for the declared reference.

Automatic, All eligible adapters and One adapter use the existing monitor's selection semantics. Discovery uses GetIfTable2 identities and current eligible legacy mappings. A sorted GUID/LUID/index set identifies the discovery epoch. Apply refreshes and validates that epoch, resolves the GUID again and requires one current mapping. A removed identity or changed discovery returns to adapter choice. No persisted numeric row position is used as identity. Traffic counters and stored history are not reset; the existing connection-change flag invalidates the transition interval.

Taskbar and startup default to Keep current. Explicit enable/disable is applied only when needed. Taskbar uses the existing visibility consumer with current window/configuration readback. Startup uses CTrafficMonitorApp::GetAutoRun/SetAutoRun; installed mode retains the managed stable launcher and portable mode retains its existing mechanism. No registry/task-scheduler substitute or host sign-in action is introduced.

## Persistence and incomplete results

`dashboard-setup.ini` lives beside the application's selected configuration. Current schema 2 stores bounded choices, one adapter GUID, step, completion, three operation states and the shared preference draft/base information needed for conflict detection. Schema 1 remains readable through the supported migration route. It stores no credentials, addresses or traffic samples. Files larger than 16 KiB or unsupported schemas are not loaded. Saves replace a temporary file atomically; an unsuccessful save leaves the window open and completion unrecorded.

Apply records each pending operation before execution and its outcome afterward. Retrying reads actual effective state first, so an already effective startup/taskbar choice is not toggled again. Each selected operation must return and read back successfully before completion is saved. Partial effects remain in place; there is no rollback claim. Keep remaining current values replaces only incomplete choices with the current settings. A removed adapter still requires an available choice. Closing never enables startup.

The wizard is modal to the monitor owner. The existing automatic-update safety check sees that owner disabled while setup is active. The dashboard's normal teardown and monitor close path are unchanged.

## Current bounded verification

Candidate 9047 at `fd0a87aa114c83794467de6f5e5b84c82bf12a1d` verified an actual unseeded first open, message-style values 2 and 3 across save/reopen, Back and Save and close, and final Keep-current completion. Next preserved the unapplied shared-preference file hash. Twelve English/Cantonese/bilingual, light/dark and normal/minimum observations ran at actual 96 DPI. Focused rail Down/Up moved content by 32 pixels, End left a 12-pixel bottom gap, and key release did not undo the position. That revision exposed a Preview Tab trap.

Candidate 9048 at `337903e3bf08bf068d7a5dfc6471419150113ce1` repairs the native read-only text dialog-code handling without replacing selection or editing semantics. Actual double-click selection remained available, and forward Tab moved from Preview to Use saved in four bilingual light/dark size combinations. A separate fresh fixture verified the declared 960 × 760 client reference at actual 96 DPI in all six language/theme combinations. English and Cantonese fitted; bilingual content reached both style ranges and the final scope text through the real rail.

Actual forward Tab, with fresh native focus readback after each key, followed Preview, Use saved, Reset, English style, Cantonese style, Next and Save and close in English/light and bilingual/dark. Every focused body control was fully revealed. The source-bound native UIA observer separately verified Details ExpandCollapse readback, idempotence and transition events. Reverse Shift+Tab, full IME/screen-reader/legacy accessibility, higher physical scales and complete renderer-specific image-promotion receipts remain unverified.

All these fixtures were portable and uninstalled. Normal exit preserved five normal-profile file hashes and startup absence, removed the fixture processes and mutex, and released the owned empty desktops. See the [combined source-bound receipt](../../evidence/native-surfaces-9048.json). This is bounded interaction evidence, not complete redesign or release acceptance.

## Historical candidates 9019 through 9022

The following records retain their original revisions, dimensions, defects and limitations. They do not describe the current implementation or substitute for current evidence.

### Corrected production and bounded runtime result

Production source `11ad9cd297d56f4884ae044f521a0c154b6ec971` adds a UTF-8 BOM to the setup text header so the existing MSVC compilation decodes localized strings correctly. Exact `build-installer.bat -Version 1.0.9020` exited 0, native build 6.61 seconds, 2 warnings and zero errors. The executable SHA-256 is `d0447ab5b69863bd319339abaed50227790713c867b2517883405de14b327c94`.

A genuine portable fixture on a named hidden desktop verified explicit first open, Next, Save and close, reopening at step 2, default Automatic with taskbar/startup Keep current, Apply, persisted completion and all three successful results, and opening the productive Network workspace. The package installation marker was excluded before launch; the draft was observed beside the fixture executable and absent from the installed data location. Installed data remained byte-identical to the legitimate post-close baseline throughout the fixture and startup registration remained unchanged. Normal fixture teardown and stable-launcher restoration succeeded; the accepted installed release 1.0.33 executable hash was independently read back.

An earlier 1.0.9019 attempt retained the package installation marker and therefore used installed data. It was stopped before Apply. Its taskbar visibility change was restored through the normal consumer, and its exclusively test-created draft was preserved outside the installed data directory after exact hash verification. Traffic records were retained with nondecreasing counters; no whole settings or history file was restored. That attempt is not portable-isolation evidence.

Raw screenshots remain private and are not accepted presentation evidence. Corrected English punctuation is visible, but the PrintWindow route returns blank native-button pixels and stale paint remnants after transitions. Their cause remains unresolved. No visual, keyboard, UIA, Cantonese/bilingual, theme, DPI, adapter-churn or partial-failure acceptance is claimed. The complete required-feature work remains unfinished. Independent review and coordinating-task integration are still pending.

### First-open repair candidate

The setup operation executor now retains acknowledged successes across save/resume. Retrying an incomplete startup operation does not reapply an earlier adapter or taskbar choice after another workflow changes it. Only editing the corresponding draft choice invalidates its result. Interrupted Applying operations read effective state before invoking the consumer. The standalone SetupFlowProgressTests.cpp exercises retained partial success, intervening legacy settings, choice-specific invalidation, interrupted application, failed consumers and the intent-save barrier. It passed; removing the completed-operation skip in a disposable copy caused the expected assertion failure.

Setup choices now use the reusable NativeMaterial::Choice owner-drawn native combo, style levels use two NativeMaterial::RangeValue trackbars with the actual native range provider, and review uses NativeMaterial::ReadOnlyText with native selection/scroll semantics. Typography separates 22-DIP semibold headings, medium labels and 14-DIP body text. Button hover and capture-loss cancellation are explicit. The native print path uses a DC render target and the same Button::Draw function and state as normal HWND rendering, with numeric render results queryable through WM_APP+0x521. No reconstructed screenshots or alternate screenshot-only drawing are used.

The revised layout intent is a 760 by 640 DIP normal client area and 720 by 600 DIP minimum, retaining 44-DIP actions. This explicitly supersedes the original 720 by 540 design size and the first implementation's 860 by 760 default. Reference images remain historical design references until updated and independently reviewed. Complete language/theme/scale, keyboard, UIA and rendering acceptance remains pending. This source candidate has not yet completed production or runtime verification.

Production repair receipt: exact build-installer.bat -Version 1.0.9021 at 4624978c31bdcd0d40e613339c214f39e6a6c550 exited 0, native 16.30 seconds, 78 warnings, zero errors. See docs/features/native-dashboard/setup-repair-build.json. Runtime and visual acceptance remain pending; the earlier 9020 run is not evidence for this changed candidate.

### Runtime-directed rendering and accessibility correction

The 9021 run confirmed Range Home-key changes and a seeded partial-result resume scenario: actual legacy All-to-Automatic selection remained Automatic after retry, and the current disabled taskbar was not overwritten by the acknowledged old Enable choice. Startup remained Keep current. This is a deliberately seeded persistence scenario, not a spontaneous startup-failure test. Fixture teardown restored accepted release 1.0.34 with unchanged installed data and startup.

The bilingual light 720 by 600 client tuple at actual 96 DPI exposed supporting-text clipping in the fixed 68-pixel information region. Layout now measures heading and information text with the actual control font and available width. Numeric range values have a separate text region above the thumb. Same-hidden-desktop UI Automation returned Invoke for buttons but no required native choice/range patterns. Explicit shared-state providers now expose RangeValue, Selection/SelectionItem and ExpandCollapse; mutations post to the control UI thread, destruction revokes the HWND, and stale selected-item identities are rejected.

Native HWND target creation and EndDraw returned S_OK while PrintWindow left button pixels blank without entering WM_PRINTCLIENT. A direct cross-process WM_PRINT probe entered the handler but returned E_INVALIDARG, so it is not accepted rendering proof. Ordinary button painting now uses the actual CPaintDC with a Direct2D DC render target and the same Draw function as native printing. No image is reconstructed. These latest source corrections require a new production run and real rendering/UIA verification; earlier screenshots are retained only as historical failure evidence.

### Version 1.0.9022 bounded verification

Exact `build-installer.bat -Version 1.0.9022` succeeded at source `7a28ffa042c2e02ec3ae582db373d0e93c6e647b`, exit 0, native build 9.32 seconds, 2 warnings and zero errors. The preceding candidate failed compilation on an int/LONG template deduction; the source now selects LONG explicitly. See [production receipt](setup-accessibility-build.json).

The portable fixture excluded the installation marker before launch and stored its completed draft beside its executable. Genuine named-hidden-desktop PrintWindow images now contain the shared native buttons, labels, disabled Back state, focus outline and range values. English, Cantonese and bilingual setup were exercised at actual 96 DPI in light theme. The bilingual 720 by 600 client image contains the full supporting text after font-based measurement. Save and close reopened the adapter step; Automatic and Keep current taskbar/startup applied successfully, persisted all three Applied results and completion, and opened Network.

Same-desktop UI Automation exposed ComboBox Selection and ExpandCollapse, Slider RangeValue and Button Invoke. Selection returned English. RangeValue changed English style from 5 to 3 through its real consumer; invalid 6 was rejected and left 3 unchanged. Expand returned a Collapsed readback, so popup expansion remains unresolved and is not accepted. Full selection traversal, provider-destruction behavior, screen-reader event delivery, dark theme and 125/150/200 percent scale coverage remain unverified. Raw images remain private pending evidence promotion and independent review. No complete visual or accessibility acceptance is claimed.

Normal fixture shutdown and named-desktop closure succeeded. Installed data remained identical to the legitimate post-close baseline and startup registration was unchanged. Stable-launcher restoration returned accepted release 1.0.34 with its expected executable hash. No fixture remains active. The coordinating task owns publication, review and integration; broader required features remain unfinished.
