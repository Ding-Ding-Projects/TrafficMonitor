# Cooperative update restart UI

The native UI snapshots the existing update-check preference for the background update service.
A staged update restarts only at a quiet UI boundary. Native settings always defers automatic
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
