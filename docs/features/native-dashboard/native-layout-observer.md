# Observe native structure without inferring rendered text

Run `build.bat --layout-observer` to build and verify the diagnostic CLI from existing
MSVC tools. This diagnostic route does not install tools or launch TrafficMonitor.

The observer takes a verified PID, named desktop, top-level HWND, executable path, complete
creation FILETIME, executable SHA-256, new JSONL basename and a 5–30 second timeout. It opens
the target without termination rights; its watchdog can terminate only its own helper.
Output is a new file beside the observer, with a protected current-user-only ACL.
Existing destinations, path traversal and reparse ancestors are rejected.

Process identity, executable path/hash, desktop membership and HWND subtree are checked
at both ends. An explicit per-monitor-v2 context provides physical window/client
rectangles, parent/control IDs, native focus, EDIT formatting rectangles and genuine
standard-scroll results. No raw control text, profile data or target font handles are
requested. Collection caps at 2048 descendants; EDIT requests have a 500 ms timeout.

Output identifies itself as `incomplete-native-structural-observation`, retaining
`receiptComplete=false`, `validV2Receipt=false` and `validV3Receipt=false`.
Renderer font/glyph bounds, custom rail ranges, paint sequence and image binding remain
unavailable. Sequential observations are not atomic rendered-layout proof. Executable
checks hash the current on-disk file, not independently measured loaded code pages.

The integrated root route passed twelve native checks covering wrong desktop, reused
identity, wrong hash, foreign/subtree HWNDs, output traversal, numeric overflow, existing
output, destroyed root and owned-helper timeout. Script checks require incomplete JSONL,
EDIT data, both identity endpoints and the protected ACL. Unshown owned windows supplied
those checks; live product acceptance remains pending.

Private output contains machine paths and native identities. Keep it in ignored private
evidence and publish only reviewed summaries. No HTTP API is exposed.
