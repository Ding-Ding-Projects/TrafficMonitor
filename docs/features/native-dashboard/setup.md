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

The source compiled and packaged at a500e6c through the exact root build-installer.bat -Version 1.0.9019, exit 0. The native build reported 114 warnings and zero errors in 23.64 seconds. See [production receipt](setup-build.json). Runtime has not been exercised: the existing release instance owns the global application singleton mutex, and this lane did not close or disturb it. Native buttons reuse the existing themed component; choices retain native combo accessibility and keyboard semantics. Native labels, read-only review text, DPI scaling and system light/dark/high-contrast palette are wired, but no visual or accessibility result is inferred from source. Setup search/full builder parity, complete component parity and application-wide required features remain acceptance gaps. Existing images do not show this wizard.
