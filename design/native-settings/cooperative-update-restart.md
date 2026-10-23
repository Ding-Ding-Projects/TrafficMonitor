# Cooperative update restart: UI integration contract

The background updater owns discovery, five-minute scheduling, downloads, staged state and the
next-launch fallback. UI code never supplies live MFC objects to that worker. A private message
pulse snapshots the current update-check preference on the UI thread and reads updater status.

Only a Ready update can request cooperative restart. Restart is deferred while a native settings
editor exists, including unchanged drafts, while the main owner is disabled by a modal dialog,
while any tracked legacy dialog exists, during menu/move/capture activity, or while an ordinary
visible top-level window exists on the UI thread. Passive floating and taskbar monitor windows
are excluded from the visible-workspace rule. The workspace, plugin dialogs and legacy settings
are not force-closed. Failure to inspect UI state defers restart.

After these conditions pass, PrepareRestart prepares the backend continuation. Only success calls
the existing graceful OnClose path, preserving configuration and history save behavior. Destroy
stops the UI pulse and signals updater shutdown without joining its worker. Application startup
and ExitInstance remain owned by the backend unit. A staged update can instead apply on the next
normal launch. No host restart, sign-out, forced process termination or visible input is used.

State inventory: disabled preference, no update, checking/downloading, ready-but-busy, ready-and-idle,
helper preparation failure, graceful close, shutdown, and next-launch fallback. Settings lifetime
is counted with stack ownership across the complete modal call, including discard confirmation.
The query is UI-thread-only and is not a thread-safe backend input.

This is intended integration behavior. The expedited mode runs no new tests, reviews, audits or
screenshots. Compilation and packaging belong to the combined backend/UI source candidate.

The coupled Settings controls expose existing automatic checking and separately persisted automatic
restart, both defaulting on for page reset. Restart preference must save and read back before the
ordinary settings consumer runs. Failure leaves all controls as draft for retry or revert. The
read-only two-second status refresh never changes preferences or dismisses the editor.
