# Task-oriented native settings: bounded implementation handoff

The native settings entrypoint opens an owned modal editor with Monitoring, Floating monitor and
Taskbar groups. It exposes actual existing consumers: sample interval and notification icon;
floating compact rates, byte units, separated units, tooltips, always-on-top and position lock;
taskbar compact rates, byte units, separated units, tooltips, right alignment and horizontal layout.
Each group has visible plain-text search, checked controls, no-match feedback and editable drafts.
Apply routes only these fields through the existing ApplySettings consumer and SaveConfig path.
Revert reloads current application values. Reset page stages documented defaults only for that
group, including filtered-out fields, and needs Apply. Close/legacy navigation confirms discarding
unapplied changes. Existing All settings opens the complete original OptionsDlg after closing this
editor. Startup scheduling, hardware, plugins, fonts, colors and all other controls remain there.

Checkboxes retain native BUTTON auto-checkbox behavior and its accessibility provider while
painting with the shared palette. Actions reuse the existing accessible custom Button. Native
edits retain text editing and IME. English and Traditional Chinese chrome are partial localization.
The minimum client area is 760 by 640 logical pixels; content is grouped to fit without scroll.
Search is literal and independent of the menu search. Adjacent complete regex workbench, complete
language modes, custom accessibility acceptance and the other legacy conversions remain pending.

State inventory: current settings, modified draft, invalid interval, filtered results, no matches,
page-reset draft, reverted values, applied/current values, forced-visible tray recovery, discard
confirmation, legacy handoff, light/dark/high-contrast and DPI relayout. Existing main/taskbar/tray
and startup defaults stay unchanged. Applying can retain a notification icon required for recovery.
The baseline setter initializes the existing interval-change detector without replacing its logic.
No native worker, new settings storage or startup scheduler is introduced.

This expedited implementation runs no new tests, reviews, audits or screenshots. Compilation and
packaging are separate source-bound results. This document is intended behavior, not runtime proof.
