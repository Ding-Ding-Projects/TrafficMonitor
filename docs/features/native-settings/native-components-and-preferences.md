# Native Settings and shared components

Use Settings to choose interface language and appearance, find an exposed monitoring preference, or stage a change before applying it. The productive application remains native C++/MFC. This increment provides 24 discoverable settings, not the complete settings or required-feature inventory.

## Choose language and appearance

Open Settings and choose Interface. Its selectors support English, Cantonese and bilingual text, with system, light and dark appearance. Search filters the two semantic preferences independently; the selector menus have their own search and adjacent regex entry. Apply saves and reads back the explicit choices. Use saved restores the saved pair to the draft. Reset stages startup defaults rather than writing immediately.

The versioned preference file is stored beside the application's stable configuration, outside installed version directories. Unknown or malformed schemas protect their existing bytes. Drafts retain a revision and fingerprint so a stale editor cannot silently overwrite a different saved choice. The setup flow retains separate draft/base/progress state. These storage contracts have focused production-module checks; complete restart/conflict and all-consumer runtime acceptance remain unfinished.

## Edit monitoring preferences without losing drafts

Group searches, cross-group discovery and per-setting actions retain their existing consumers. Invalid numeric text blocks application and reveals the setting. Changing interface language refreshes numeric labels, default/source descriptions, discovery rows and the current explanatory or validation message without reloading unsaved numeric text. A live numeric draft and an invalid draft were exercised across language changes. Checkbox-draft preservation is source-reviewed, not directly exercised by this increment.

Native edit-action menus retain Undo, Cut, Copy, Paste, Delete and Select all. Their search field, Regex and Back actions share the native component presentation. Search state remains independent per owning workflow. Existing search capability remains bounded by its implemented engine; this increment does not establish full RegexBuddy parity.

## Native controls and scrolling

Shared components provide styled action buttons, text fields, selectors, semantic navigation icons, section panels, result lists and custom scroll rails. Real native edits retain their editing route instead of replacing the input engine with painted text. Scroll rails reserve their own gutter and appear only for overflowing content. Home, End, Page Up, Page Down, wheel and drag received bounded real interaction checks. Unequal-height overflowing rows, complete clipboard/IME behavior, hover states and the complete keyboard matrix remain unverified.

The rail's retained UI Automation provider respects disabled state, rejects disabled value requests before queuing and on the UI thread, and revokes element-dependent queries after destruction. A real retained element/pattern observed ElementNotEnabledException 80040200 while disabled and ElementNotAvailableException 80040201 after normal closure. This is narrow provider evidence, not full accessibility acceptance or an arbitrary concurrent handle-reuse guarantee.

Settings retains the operating-system caption and resizing behavior. Its native dark-mode request was observed through attribute readback and genuine pixels. Explicit border/caption/text-color attributes were unavailable on the selected host, so the native fallback remains. The API support boundaries are documented in [Microsoft's DWM attribute reference](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute).

## Verification and source boundaries

The final independently accepted correction unit is source `76a41a4f89d802524501e8cc558f4e7d3ac2fa21`, local package 1.0.9033. The exact root installer entrypoint exited 0. Executable SHA-256: `9e433eba89ec0185b295526aef214541890d6c6d5184afa400563a830df378ea`. Full package SHA-256: `95132235a40583d237681b6eb6e2a5a6999f33e93cdc42db315ece32598fa9c1`. Earlier 9031 and 9032 observations retain their exact source bindings in the task history and closeout records.

At integrated task source `a8fc568c6f47a1b2606bc1eb31408bdd615f03cf`, the exact root preference target passed 23 storage/draft cases and 24 real production INI serialization cases. A deliberate preservation mutation failed and restored checks passed. The unchanged production text helpers were extracted into CommonText.cpp to execute the actual INI implementation without test-only substitutes. Later combined-source checks must retain their own source binding.

Independent review accepted the bounded corrections after inspecting the real images and action hashes. The supplied native visual observations are 96 DPI only. The fixtures closed normally and restored accepted installation 1.0.45 with saved data and startup comparisons passed. Local candidate evidence is not proof of a later integrated or released binary.

## Remaining work and recovery

The full application remake is unfinished. Remaining owned dialogs, workspace/setup/regex conversion, all applicable controls, full required-feature and regex parity, high contrast, complete language/theme/scale coverage and accepted integrated delivery remain open. Minimum scrolling may move a section heading partly outside its viewport. Selector panel-edge polish remains. Formal native image promotion needs a truthful native measurement schema and fresh producer observations; browser counters are not fabricated. Raw evidence has not been relabeled or promoted as complete acceptance.

Preserve existing configuration, traffic history, skins and plugins. Unknown-schema or stale-draft states must remain recoverable rather than being reset to make a check pass. Installation, sign-in startup, tray/taskbar behavior and silent updating retain their existing contracts. Documentation website, wiki and reader completion remain last. This feature exposes no HTTP API, so a Postman collection does not apply.
