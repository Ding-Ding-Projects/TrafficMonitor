# First-open native setup

## Start and resume

Choose Open dashboard explicitly. An incomplete setup opens four steps: language and message style, monitoring scope, taskbar/login startup, and review. Ordinary launch and login startup continue monitoring without this wizard. After completion, Open dashboard opens Network. The dashboard context menu contains Resume setup for later changes.

Back retains edits. Next stores progress, while Save and close saves the bounded local draft without applying monitor, taskbar or startup changes. Reopening restores that draft. Completed setup reopened explicitly loads the current monitor choice and initializes both optional changes to Keep current.

## Actual choices

The setup language has English, Cantonese and bilingual choices. Independent levels from 1 to 5 change the wizard's supporting message voice while keeping factual results unchanged. These controls are explicitly scoped to setup; other windows keep their existing language behavior. Application-wide language and message coverage remains unfinished.

Automatic, All eligible adapters and One adapter use the existing monitor's selection semantics. Discovery uses GetIfTable2 identities and current eligible legacy mappings. A sorted GUID/LUID/index set identifies the discovery epoch. Apply refreshes and validates that epoch, resolves the GUID again and requires one current mapping. A removed identity or changed discovery returns to adapter choice. No persisted numeric row position is used as identity. Traffic counters and stored history are not reset; the existing connection-change flag invalidates the transition interval.

Taskbar and startup default to Keep current. Explicit enable/disable is applied only when needed. Taskbar uses the existing visibility consumer with current window/configuration readback. Startup uses CTrafficMonitorApp::GetAutoRun/SetAutoRun; installed mode retains the managed stable launcher and portable mode retains its existing mechanism. No registry/task-scheduler substitute or host sign-in action is introduced.

## Persistence and incomplete results

`dashboard-setup.ini` lives beside the application's selected configuration. Schema 1 stores only bounded integers, one adapter GUID, step, completion and three operation states. It stores no credentials, addresses or traffic samples. Files larger than 16 KiB or unsupported schemas are not loaded. Saves replace a temporary file atomically; an unsuccessful save leaves the window open and completion unrecorded.

Apply records each pending operation before execution and its outcome afterward. Retrying reads actual effective state first, so an already effective startup/taskbar choice is not toggled again. Each selected operation must return and read back successfully before completion is saved. Partial effects remain in place; there is no rollback claim. Keep remaining current values replaces only incomplete choices with the current settings. A removed adapter still requires an available choice. Closing never enables startup.

The wizard is modal to the monitor owner. The existing automatic-update safety check sees that owner disabled while setup is active. The dashboard's normal teardown and monitor close path are unchanged.

## Verification boundary
## Corrected production and bounded runtime result

Production source `11ad9cd297d56f4884ae044f521a0c154b6ec971` adds a UTF-8 BOM to the setup text header so the existing MSVC compilation decodes localized strings correctly. Exact `build-installer.bat -Version 1.0.9020` exited 0, native build 6.61 seconds, 2 warnings and zero errors. The executable SHA-256 is `d0447ab5b69863bd319339abaed50227790713c867b2517883405de14b327c94`.

A genuine portable fixture on a named hidden desktop verified explicit first open, Next, Save and close, reopening at step 2, default Automatic with taskbar/startup Keep current, Apply, persisted completion and all three successful results, and opening the productive Network workspace. The package installation marker was excluded before launch; the draft was observed beside the fixture executable and absent from the installed data location. Installed data remained byte-identical to the legitimate post-close baseline throughout the fixture and startup registration remained unchanged. Normal fixture teardown and stable-launcher restoration succeeded; the accepted installed release 1.0.33 executable hash was independently read back.

An earlier 1.0.9019 attempt retained the package installation marker and therefore used installed data. It was stopped before Apply. Its taskbar visibility change was restored through the normal consumer, and its exclusively test-created draft was preserved outside the installed data directory after exact hash verification. Traffic records were retained with nondecreasing counters; no whole settings or history file was restored. That attempt is not portable-isolation evidence.

Raw screenshots remain private and are not accepted presentation evidence. Corrected English punctuation is visible, but the PrintWindow route returns blank native-button pixels and stale paint remnants after transitions. Their cause remains unresolved. No visual, keyboard, UIA, Cantonese/bilingual, theme, DPI, adapter-churn or partial-failure acceptance is claimed. The complete required-feature work remains unfinished. Independent review and coordinating-task integration are still pending.

## First-open repair candidate

The setup operation executor now retains acknowledged successes across save/resume. Retrying an incomplete startup operation does not reapply an earlier adapter or taskbar choice after another workflow changes it. Only editing the corresponding draft choice invalidates its result. Interrupted Applying operations read effective state before invoking the consumer. The standalone SetupFlowProgressTests.cpp exercises retained partial success, intervening legacy settings, choice-specific invalidation, interrupted application, failed consumers and the intent-save barrier. It passed; removing the completed-operation skip in a disposable copy caused the expected assertion failure.

Setup choices now use the reusable NativeMaterial::Choice owner-drawn native combo, style levels use two NativeMaterial::RangeValue trackbars with the actual native range provider, and review uses NativeMaterial::ReadOnlyText with native selection/scroll semantics. Typography separates 22-DIP semibold headings, medium labels and 14-DIP body text. Button hover and capture-loss cancellation are explicit. The native print path uses a DC render target and the same Button::Draw function and state as normal HWND rendering, with numeric render results queryable through WM_APP+0x521. No reconstructed screenshots or alternate screenshot-only drawing are used.

The revised layout intent is a 760 by 640 DIP normal client area and 720 by 600 DIP minimum, retaining 44-DIP actions. This explicitly supersedes the original 720 by 540 design size and the first implementation's 860 by 760 default. Reference images remain historical design references until updated and independently reviewed. Complete language/theme/scale, keyboard, UIA and rendering acceptance remains pending. This source candidate has not yet completed production or runtime verification.

Production repair receipt: exact build-installer.bat -Version 1.0.9021 at 4624978c31bdcd0d40e613339c214f39e6a6c550 exited 0, native 16.30 seconds, 78 warnings, zero errors. See docs/features/native-dashboard/setup-repair-build.json. Runtime and visual acceptance remain pending; the earlier 9020 run is not evidence for this changed candidate.

## Runtime-directed rendering and accessibility correction

The 9021 run confirmed Range Home-key changes and a seeded partial-result resume scenario: actual legacy All-to-Automatic selection remained Automatic after retry, and the current disabled taskbar was not overwritten by the acknowledged old Enable choice. Startup remained Keep current. This is a deliberately seeded persistence scenario, not a spontaneous startup-failure test. Fixture teardown restored accepted release 1.0.34 with unchanged installed data and startup.

The bilingual light 720 by 600 client tuple at actual 96 DPI exposed supporting-text clipping in the fixed 68-pixel information region. Layout now measures heading and information text with the actual control font and available width. Numeric range values have a separate text region above the thumb. Same-hidden-desktop UI Automation returned Invoke for buttons but no required native choice/range patterns. Explicit shared-state providers now expose RangeValue, Selection/SelectionItem and ExpandCollapse; mutations post to the control UI thread, destruction revokes the HWND, and stale selected-item identities are rejected.

Native HWND target creation and EndDraw returned S_OK while PrintWindow left button pixels blank without entering WM_PRINTCLIENT. A direct cross-process WM_PRINT probe entered the handler but returned E_INVALIDARG, so it is not accepted rendering proof. Ordinary button painting now uses the actual CPaintDC with a Direct2D DC render target and the same Draw function as native printing. No image is reconstructed. These latest source corrections require a new production run and real rendering/UIA verification; earlier screenshots are retained only as historical failure evidence.
