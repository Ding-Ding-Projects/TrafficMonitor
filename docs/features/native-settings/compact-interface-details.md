# Compact interface preferences and Details

Language and Appearance now use two compact, divider-separated rows instead of separate large cards. Wider layouts align labels and selectors; narrow layouts stack them with measured wrapping. A compact preview shows draft interface wording. Apply remains explicit, Use saved and Reset are text actions, and Reapply appears only for a conflict.

Details is a transient disclosure beside the section heading, or on its own line when the measured heading and action cannot fit together. It explains scope and actual preference provenance without exposing paths or raw revisions. Opening or closing it does not save, reset, reload or replace a draft. Conflict, protected-file, uncertain-write and save-result messages stay visible outside the disclosure. Search still counts the two semantic settings, not the explanatory action.

The same native button HWND handles mouse and keyboard input and exposes a UI Automation Button role with an optional ExpandCollapse pattern. Provider requests express the desired state rather than toggling blindly; repeated Expand or Collapse is designed to be idempotent. Those provider semantics passed source review but still need successful client verification. No separate legacy accessibility adapter is claimed.

Candidate 1.0.9046, source `cdd0e7c40b13e48989e5c546b7ce0afaa58c4d0f`, built through the exact root installer entrypoint with two existing delay-load warnings and zero errors. The existing native preference checks passed: 23 preference and 24 setup serialization cases, deliberate mutation rejection and the corrected preference repeat.

Actual uninstalled verification covered English, Cantonese and bilingual modes, light/dark themes, and measured client sizes 1280 × 840 and 760 × 640 at 96 DPI. Twenty-four matrix images show collapsed/expanded states. Mouse, Enter and Space changed the disclosure visibly. The initial disclosure action left all fixture INI bytes unchanged. At minimum bilingual size, End on the actual rail exposed Appearance and Preview, and Home returned to the top. All five normal-profile file hashes and startup absence were preserved through normal exit.

This is bounded interaction and inspected-pixel evidence. UIA client routes were unavailable or timed out; higher physical DPI scales, restart persistence, high contrast, complete provider behavior and complete renderer-specific layout receipts remain unverified. The setup owner only received disclosure command forwarding; its full unified step viewport remains unfinished. Shared palette/button refinement and notification grouping are separate remaining work.

See the [bounded receipt](../../evidence/compact-interface-9046.json). Raw images and partial geometry remain private pending complete native promotion evidence, and are not substituted for finished-design screenshots.
