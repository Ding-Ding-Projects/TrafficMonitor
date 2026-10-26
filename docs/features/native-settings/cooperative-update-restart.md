# Cooperative update restart UI

The native UI snapshots the existing update-check preference for the background update service.
A staged update restarts only after the person chooses **Restart to install update** in the
[update banner](../installation/update-banner.md) (or after enabling automatic restart, which is off by
default), and then only at a quiet UI boundary. Native settings always defers automatic
restart while its modal editor exists, including unchanged or unapplied drafts. Active legacy
dialogs, visible ordinary workspaces and menu, drag or capture activity also defer restart.
The passive floating and taskbar monitors do not by themselves prevent restart.

When the backend reports Ready, the UI verifies that boundary and asks it to prepare continuation.
Only successful preparation calls the existing graceful close/save path. Settings and history
retain their existing save semantics. The backend owns the five-minute schedule, staging and
pending-next-launch fallback. Closing the main dialog signals stop without joining its worker.
No host power action or forced process close is introduced.

This UI hook is coupled to NativeUpdate/SilentUpdater.h and requires the combined backend unit.
It has no standalone build or runtime verdict. No new tests, reviews, audits or screenshots run
in the expedited pass. Busy-state, draft preservation, restart, update and fallback behavior remain
unverified until separately exercised under an authorized verification mode.

## Update preferences

Monitoring includes the existing automatic-check preference and a separate automatic-restart
preference, disabled by default. When automatic checking is enabled, silent checks run every five
minutes. Turning off automatic restart still allows staging; pending updates apply on the next
normal launch. Read-only service status refreshes every two seconds while Settings is open.

These controls participate in drafts. Apply validates the interval first, then saves and reads
back a changed restart preference through the backend API before invoking the existing settings
consumer. A failed restart-preference save retains the draft, reports the failure and does not
apply other settings. Revert reads current preferences. Reset page draft stages automatic checking as
enabled and automatic restart as disabled but writes nothing until Apply. Backend persistence and update execution stay
owned by the updater; this UI does not introduce a second preference file or write path.
