# Native task-oriented settings

Settings now opens a native draft editor with three task groups: Monitoring, Floating monitor and
Taskbar. Search filters the current group by English or Chinese label text; Clear restores it.
Monitoring edits the actual 200-30000 ms sampling interval, in multiples of 100, and tray icon.
Floating controls edit compact rates, byte units, separated units, tooltips, always-on-top and
position locking. Taskbar controls edit compact rates, byte units, separated units, tooltips,
right alignment and horizontal layout. These are existing product settings with actual consumers.

Apply validates the interval and passes only these fields to the existing ApplySettings and
configuration-save paths. The original interval detector is initialized from its real baseline.
A recovery-required tray icon may remain visible, and the editor reloads actual applied values.
Revert discards the draft and reloads current values. Reset page draft stages documented defaults
for the entire selected group, including rows hidden by search; Apply is still required. Defaults
are 1000 ms and visible tray, byte units, separated units and tooltips; compact rates, always-on-top,
position locking and right alignment are off; horizontal taskbar layout is on. Unapplied changes
require confirmation before closing or opening All settings (legacy). That route retains every
original OptionsDlg control, including startup scheduling, hardware, fonts, colors and plugins.

Controls use the existing palette and action buttons plus a reusable painted native auto-checkbox.
Native checkbox state and keyboard behavior are retained. Search and interval use native edit
controls. English and Traditional Chinese text are partial localization. Ctrl+L focuses search;
Escape first retains IME composition behavior, then clears a focused nonempty query or invokes the
normal close/discard flow. Group, search and appearance are session-local, not new stored settings.

This is a bounded conversion, not the complete settings inventory. Full adjacent regex workbench,
all language modes, accessibility acceptance and the remaining legacy conversions are unfinished.
No new tests, reviews, audits or screenshots run in the selected expedited delivery mode. Source
and build receipts never establish runtime, IME, keyboard, persistence, recovery or visual proof.
The prior workspace release 1.0.17 belongs to source f0d74c476842e1546ef1d7d666e5af914fc06044,
not this settings change. Installed-update proof is tracked separately by the release owner.
