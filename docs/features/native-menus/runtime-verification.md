# Bounded native runtime verification

Source: `8c6ca105bff3171f257feb5b40c49903ca8e069f`.

The exact root `build-installer.bat -Version 1.0.8000` completed successfully. Its embedded root build reported zero errors and two existing LNK4199 warnings. The genuine package was extracted into a separate portable verification directory using the application's supported portable mode. The candidate was never installed or published. Executable SHA-256: `9099edb27f7a2609758adb11eeb5104eced29e55900b278e77cfbd55ac1ff70e`.

The real application ran on a named off-screen Windows desktop through the Lowlevel Python binding. Process identity, creation time and window ownership were checked before input. The tuple was English, light appearance, 96 DPI (100%), popup 440 by 560 pixels. This is one tuple, not the complete layout matrix.

## Observed behavior

- Opened the actual Plugin Manager and its context menu with an empty plugin inventory. The three unavailable commands rendered with their labels and icons; the default Details item used bold text.
- Entered `impossiblequery` in the native search edit. Results disappeared and the real No matching commands static control became visible.
- Escape cleared the query while leaving the popup open. Down moved into results. Enter on the unavailable command did not execute or close the popup. Escape then dismissed it.
- Retrieved the real results provider through direct IAccessible and IDispatch. Both reported role 12 (menu item) and the identical state value 3145987 for the first disabled/default row. The dispatched default-action property was empty, as expected for that disabled item.
- Opened actual historical statistics. No history-menu acceptance is claimed: attempted tab coordinates reached the combo control, and the pass ended without exercising that menu.

The menu-only images were inspected privately. They contain no adapter addresses or machine paths. They remain local diagnostic material, not promoted evidence: the complete layout-probe/provenance receipt has not been produced.

## Restoration

Only the matching existing monitor was temporarily closed through WM_CLOSE after identity validation and configuration preservation. The candidate subsequently exited gracefully. The existing version 1.0.12 was restored through its stable updater process-start route. Readback confirmed its main window hidden, actual taskbar child visible, startup registration unchanged and saved configuration byte-for-byte unchanged. No forced termination, installation of the candidate, visible-menu interaction or host power action occurred.

## Still unverified

Direct and dispatched default-action execution; queued-action identity races and negative mutations; enabled execution; nested submenus; checked/radio states; focus restoration; real IME composition; actual UI Automation patterns and screen-reader operation; dynamic plugins; seven other integrated consumers; dark/high contrast; remaining languages/scales/work-area tuples; full regex workbench. Read agreement from MSAA is not full UI Automation acceptance. A successful build and this bounded runtime pass do not establish complete Material Design conformance or release readiness.

## Main, tray and enabled-action follow-up

The two main/tray call sites now use the same presenter, source `bfe6ffec867543d8ef63ef867b1180ba7fb422bf`. Only the include and those two calls changed. The exact package entrypoint was repeated for version 1.0.8000. The first run compiled successfully but failed when its checksum generation attempted to read its own existing output file. Preserving that previous checksum outside the release-output directory allowed the same entrypoint to finish with exit 0. The permanent packaging repair is owned by the integration lane.

New executable SHA-256: `f6f20afd2e070c1d629c02af6c4a04b51bb3922cf24fa62e3262f61eb67c2e91`. The candidate again ran in supported portable mode on the off-screen desktop; it was never installed. The existing installation was restored afterwards, with hidden main window, visible taskbar child, unchanged startup registration and unchanged saved configuration.

Observed on the actual main-window context menu:

- Right-click opened the shared searchable native popup.
- Searching `opacity`, Down, then Right opened the nested opacity menu. Direct and dispatched accessibility reads reported the submenu action.
- Direct IAccessible default action selected 100%. Reopening showed its checked state and the 80% row unchecked.
- IDispatch default action selected 80%. Reopening showed 80% checked and 100% unchecked. Both methods therefore executed real enabled commands, not merely property reads.
- Left returned to the parent and restored its independent `opacity` query. Escape cleared that query; the next Escape dismissed the popup.
- The actual calendar-history menu opened from its button. Direct accessible action opened its first-day submenu, and dispatched action selected Monday. The original Sunday state and later Monday marker were observed. A later direct action restored Sunday in the portable candidate.

A rendering/evidence discrepancy remains unresolved: the initial calendar submenu image contained Sunday and Monday text, but repeated PrintWindow images of the reopened submenu retained selection/check graphics while omitting those labels. Accessibility still returned the labels. Neither a capture-only cause nor an application rendering cause is established, so visual acceptance for that tuple remains incomplete.

A GetGUIThreadInfo query returned no usable hidden-thread focus result. The disconnected-provider helper did not run because of a helper indentation error; that helper was repaired and syntax checked after safe teardown, but its intended runtime check remains unrun. The list-history menu requires the actual cursor position over its Figure header; posted off-screen mouse messages alone did not exercise that route, and the visible cursor was not moved.

Queued-action race mutation, disconnected-provider invocation, focus restoration, tray callback runtime, remaining consumers, full UI Automation, IME and the full layout matrix remain unfinished. The new enabled-action evidence supersedes only the corresponding earlier unverified item, not the whole acceptance list.

## Calendar capture-route and provider-lifetime follow-up

The same executable was relaunched for a bounded diagnostic pass. On the actual calendar submenu, a direct PrintWindow capture of its native ListBox rendered both Sunday and Monday correctly, while the corresponding whole-popup PrintWindow capture omitted those labels. This establishes a difference between the two capture targets. It does not establish the cause or prove whole-window visual acceptance, and no speculative product-source rendering change was made.

A retained real IAccessible object was then invoked after its popup was closed through WM_CLOSE. Direct default action returned `CO_E_OBJNOTCONNECTED` (`-2147220995`). The IDispatch call raised `DISP_E_EXCEPTION` (`-2147352567`). Neither call executed a command. This verifies the destroyed-provider rejection path, not the queued-action identity race, whose deterministic runtime mutation check remains unfinished.

The candidate exited gracefully after this pass. Existing version 1.0.12 was restored and read back with its main window hidden, taskbar child visible, startup unchanged and saved configuration unchanged. No hidden candidate remains running.
