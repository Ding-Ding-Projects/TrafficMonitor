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
Local publication hooks and the hosted publication job pass. The strict `publication-language`
required check is configured; administrators retain bypass permission.

## Native interface and required features, after the first delivery

- [ ] Design and review a native MFC Material Design 3 component system.
- [ ] Convert the complete owned interface while preserving monitoring and plugins.
- [x] Inventory every required feature and its applicability by delivered surface.
- [ ] Implement and verify all applicable features, documentation, and companions.
- [ ] Publish complete documentation and genuine current build evidence.

Unchecked items are unfinished; source implementation alone does not establish runtime acceptance.

The [110-item inventory](docs/features/required-features/README.md) records implementation and
proof separately. Context menus require the new visual system, action search and a full regex
workbench. Current presentation screenshots must be replaced with genuine rebuilt evidence,
with historical originals retained separately.

## Documentation and current-host milestones

- [x] Deploy the documentation home and verify exact About homepage readback.
- [x] Translate all 1,387 upstream history entries and retain complete fork messages.
- [x] Publish five prepared fork wiki pages and retain their source.
- [x] Verify current-host native build dependency acquisition and accepted 1.0.12 update.
- [x] Repair and repeat the settings page language/theme/scale overflow matrix.
- [ ] Complete keyboard, accessibility and remaining documentation interaction acceptance.
- [ ] Finish native components, live dashboard, setup, context menus and every remaining surface.

