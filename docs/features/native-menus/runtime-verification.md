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
