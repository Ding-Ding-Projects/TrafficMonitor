# Continuation handoff

The native network workspace source is implemented on the dedicated feature branch. Its design
is in design/native-dashboard/observational-workspace.md and behavior/limits are in
 docs/features/native-dashboard/README.md. New runtime code is confined to NativeDashboard;
shared sampling, controller, palette and button implementations are reused unchanged.

Next: commit the coherent source, run the exact root build.bat and build-installer.bat with a
unique local candidate version, and record actual results. Keep the source pinned during each
producer. The parent session owns default-branch integration, publication and installed updates.

The current expedited mode prohibits new tests, reviews, audits and screenshots. Runtime,
accessibility, IME, keyboard and visual behavior for this workspace remain unverified. Historical
menu and installer evidence applies only to its recorded source. Full regex parity, onboarding,
complete custom tables, complete localization, settings/dialog conversion and other unfinished
requirements remain open. Preserve existing monitoring, taskbar/tray defaults and saved data.
