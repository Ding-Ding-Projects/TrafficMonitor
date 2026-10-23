# Native settings verification

## Candidate and production

The native runtime candidate is commit `5d9ec3207862eea16a028b72305de9400619943c`. The exact root `build.bat --silent` and `build-installer.bat -Version 1.0.9025` completed successfully on Windows. The final packaging build reported two existing LNK4199 delay-load warnings and no errors. Package 1.0.9025 is a verification fixture, not an accepted-channel release or installed-update claim.

| File | SHA-256 |
| --- | --- |
| TrafficMonitor.exe | `3a22a512a32c6363bdd1c345cfe6d13ed6f1f13b44a66cf15ba78691e018bac3` |
| TrafficMonitor-1.0.9025-full.nupkg | `3320d1b96dcdf2aec679806e9e8c59dc740c61ea0cb8de159086a9e25d698b5f` |
| Setup.exe | `5131ab3ed86cd9d67bf5ae3a217775b8010f42b9c82a42fc035672eb5c77d34c` |

The package manifest records that exact commit, version, x64 architecture and Release (lite) configuration. Later changes to tests and this report do not change the source identity of these bytes.

## Focused executable checks

Run `build.bat --verify-settings`. The production validation helpers pass 46 checks covering strict parsing, bounds, overflow, trailing input, units, unavailable capabilities and asynchronous editor ownership. Positive numeric cases check the exact parsed output.

The same entrypoint builds a disposable copy of the production helper with the previous always-increment ownership behavior. That mutation produces exactly five repeated-open failures, exits 1, and is required to be rejected. The unchanged production helper then passes all 46 checks again. This is mutation evidence, not execution of an earlier source revision. Compilation failures and unexpected mutation outcomes remain nonzero failures.

## Observed native interaction

A named hidden desktop run on 2026-10-23, from 18:12 to 18:15 UTC, used the package executable above and a real portable configuration. Action receipts retain fresh before/after UTC timestamps and raw image hashes. The following observations were made against that runtime:

- Reopening the existing regex workbench, running its preview and applying the query `memory` updates the original Notifications search and shows one matching setting.
- Each group retains its independent query. Searching all groups for the formatted value `81` finds Memory-use notification; explicit navigation temporarily reveals it while preserving the destination's `memory` query.
- A memory draft of 101 blocks Apply after an interval draft of 1200 was entered in another group. Validation returns to the memory row and focuses its edit control without losing the interval draft.
- Correcting memory to 81 and applying writes `monitor_time_span = 1200` and `notify_tip.memory_tip_value = 81` in the fixture configuration. Notification enable remains false and the daily-traffic threshold remains 200 MB.
- The row menu exposes explanation, default-draft and revert actions. Enter selects the explanation for the correct row, and its text appears in the editor.
- The real child viewport contains the scrolling rows while the search and footer remain fixed. Light and dark states were inspected at the normal window size and enforced minimum client area of 760 by 640, on an actual 96-DPI display.
- Temperature controls are disabled in the lite build and explicitly report unavailable capability.

The fixture closed normally. The matching existing installation was restored through its supported launcher. Stored data matched the post-close snapshot and the startup preference was unchanged before restoration. Private process identities, configuration snapshots and raw receipts are retained outside the repository.

## Remaining acceptance

- Raw screenshots have been inspected but are not yet promoted through the repository evidence inventory and receipt validator. This article does not substitute for that promotion.
- Shift+F10 did not open the menu through the message-based transport, whose documented modifier limitations prevent treating that observation as a product defect. The helper does not expose the Menu key by name. Hardware-keyboard proof remains unverified.
- A second process loaded the saved interval 1200 and memory threshold 81 from the fixture configuration. WM_NEXTDLGCTL followed by message-based VK_APPS opened the focused memory row menu. These are process-reload and message-input observations, not hardware-keyboard proof. Pointer context actions, complete tab order, screen-reader behavior and failure-injected automatic-restart preference persistence still need focused runtime verification.
- The second Chinese-locale run exposed mojibake in candidate 1.0.9025 because the two new UTF-8 source files lacked byte-order marks for the existing compiler configuration. Explicit UTF-8 markers now correct those inputs; a rebuilt native verification is pending. The 125%, 150% and 200% real display-scale tuples remain unverified. Chinese-versus-other language selection is implemented; complete bilingual mode is not.
- Full RegexBuddy parity, persistent global language/theme conversion, complete settings migration and the remaining required-feature inventory remain unfinished.

No actual notification was triggered as part of this verification. The existing settings callback is not a transaction or configuration-file readback guarantee.
