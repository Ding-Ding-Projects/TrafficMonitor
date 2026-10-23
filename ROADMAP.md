# TrafficMonitor delivery roadmap

## Installer and release foundation

- [x] Create the organization fork, preserve upstream history, and establish `main`.
- [x] Add reproducible x64 build and installer entrypoints.
- [x] Package a genuine unsigned Squirrel.Windows installer with required runtime dependencies.
- [x] Enable current-user login startup on fresh install through the stable launcher.
- [x] Default installed taskbar display to visible and main window to hidden.
- [x] Preserve explicit choices and data across updates; remove only owned startup registration.
- [x] Verify installer, update, runtime display, and release download behavior.
- [x] Publish and verify the first customized normal release.

Releases 1.0.3 and 1.0.4 passed current-host installation and update checks. The current taskbar
monitor is attached to Explorer with its main window hidden. Subsequent sign-in, Explorer restart,
fresh-host prerequisite acquisition and cancellation remain separately unverified.
Local publication hooks and the hosted publication job pass; branch-protection enforcement is
not yet established.

## Native interface and required features, after the first delivery

- [ ] Design and review a native MFC Material Design 3 component system.
- [ ] Convert the complete owned interface while preserving monitoring and plugins.
- [ ] Inventory every required feature and its applicability by delivered surface.
- [ ] Implement and verify all applicable features, documentation, and companions.
- [ ] Publish complete documentation and genuine current build evidence.

Unchecked items are unfinished; source implementation alone does not establish runtime acceptance.
