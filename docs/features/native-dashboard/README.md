# Native network workspace

- [Workspace responsiveness diagnostics and limits](responsiveness-diagnostics.md)
- [List measurement repair and portable diagnostic preparation](list-layout-responsiveness.md)
- [Native structural observer and incomplete-evidence boundaries](native-layout-observer.md)

- [Released native interface images and verification limits](released-interface-evidence.md)

Choose **Open monitoring workspace** from the existing tray, taskbar or floating-monitor menu.
The workspace opens only after this explicit action. Repeating the action focuses its existing
window. Closing it leaves the existing taskbar, tray and traffic accounting running.

## Inspect live adapters

The adapter table is fed by immutable snapshots from the existing background sampling controller.
Select an adapter to inspect its receiving and sending rates, observation timestamp, state,
interval, link speeds and addresses. The graph and sample-history table describe that adapter,
not the legacy selected aggregate. Missing rates remain unavailable; disconnected, stale and
first-sample states remain explicit. Persistent GUID identity keeps a selection associated with
the same adapter. If it disappears, details report the absence instead of selecting another row.

Search filters adapter names and descriptions with plain text. Clear restores the complete list.
Pause display freezes the displayed snapshot while collection continues; resume takes the latest
snapshot. Restart samples resets only this observational session, leaving saved traffic history
and monitor configuration intact. The sample table provides a textual alternative to the graph.
The Details tab uses a selectable, read-only native text control.

## Existing workflows

History, Plugins and Settings invoke the existing application commands. Toggle floating monitor
retains the previous show/hide behavior and saves that existing preference. Switch appearance
changes the workspace palette for this window; system high-contrast colors take precedence.
Search, view selection and palette choice are session-local. No workspace opens during startup.
Tab moves through controls, F6 reaches the view tabs, and Ctrl+L focuses search. Escape clears a
focused nonempty search, returns from another view to Adapters, or closes the workspace. Active
IME composition retains its own Escape handling. The context menu uses the shared searchable
native menu and offers adapter details, display pause/resume and observational restart.

## Scope and verification limits

This is a partial native workspace implementation. It uses the existing Material Design 3
palette, Direct2D drawing and accessible action buttons, plus native report lists, edits and tabs.
Native table accessibility is not proof of a custom UIA Grid/Table implementation or complete
screen-reader behavior. English and Traditional Chinese workspace text are implemented; complete
language modes and all localized workspace strings remain unfinished. Existing menu action labels
are updated in all bundled language resources.

The original workspace milestone carried no new tests, reviews or screenshots. The current preserved
workspace candidate still lacks complete runtime, keyboard, accessibility, IME and high-DPI acceptance.
Compilation and packaging results are recorded separately in the handoff. Existing historical
menu evidence does not verify this new workspace. Full regex workbench parity, practical onboarding,
complete settings/dialog conversion and the remaining required features stay unfinished.

Discovery and worker failures are shown with their actual result codes. Closing detaches delivery
and signals cancellation without joining discovery on the UI thread. Addresses stay in the local
view; this module introduces no export, telemetry, network transmission or persisted address store.

## First-open setup candidate

See [native setup behavior and verification limits](setup.md). Portable 9047/9048 checks cover bounded first open, save/resume, unified scrolling, forward Tab and selected accessibility behavior at 96 DPI. Complete runtime acceptance remains pending.
