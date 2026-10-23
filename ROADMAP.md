# TrafficMonitor delivery roadmap

## Installer and release foundation

- [x] Create the organization fork, preserve upstream history, and establish `main`.
- [x] Add reproducible x64 build and installer entrypoints.
- [x] Package a genuine unsigned Squirrel.Windows installer with required runtime dependencies.
- [x] Enable current-user login startup on fresh install through the stable launcher.
- [x] Default installed taskbar display to visible and main window to hidden.
- [ ] Preserve explicit choices and data across updates; remove only owned startup registration.
- [ ] Verify installer, update, runtime display, and release download behavior.
- [x] Publish and verify the first customized normal release.

Release 1.0.3 and its installer callback/startup-command launch passed current-host checks.
Real Explorer attachment, subsequent sign-in, update acceptance and fresh-host prerequisites remain
separate verification gaps. Full-tree publication enforcement is pending inherited encoding and
literal-collision support; no complete protection verdict is claimed.

## Native interface and required features, after the first delivery

- [ ] Design and review a native MFC Material Design 3 component system.
- [ ] Convert the complete owned interface while preserving monitoring and plugins.
- [ ] Inventory every required feature and its applicability by delivered surface.
- [ ] Implement and verify all applicable features, documentation, and companions.
- [ ] Publish complete documentation and genuine current build evidence.

Unchecked items are unfinished; source implementation alone does not establish runtime acceptance.
