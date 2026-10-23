# Native verification and staged handoff

## Evidence status

No built reference viewer, production implementation, screenshots, screen recording, UI Automation inspection or design-parity result is supplied by this design package. SVGs are explicitly static design fixtures. The resource inventory is source discovery at `47ace8a91966c818d9180ae4a3a0b1c6790eb4c2`, not runtime verification. A separate reviewer must approve or return the design before source implementation.

## Deterministic selectors

Reserve developer-only arguments `--ui-reference=<screen>/<state> --ui-theme=<light|dark> --ui-language=<en|yue|bilingual> --ui-client-dip=<width>x<height> --ui-scale=<100|125|150|200> --ui-fixture=design-v1`. The reference viewer is a separate native developer executable that reads the existing design SVG/manifest bytes directly. It must not redraw a second hand-transcribed mock. Production deterministic mode navigates the real native control tree using isolated synthetic data and separate temporary application data; it must never touch normal startup, settings, network adapter choices or installed user data. Normal production mode reads real data. Both routes are pending implementation.

## Required tuple matrix

For each inventory row and each declared state: light and dark, English/Cantonese/bilingual, 100/125/150/200 percent, normal and minimum client viewport. Dashboard normal 1120x760 DIP, minimum 720x540 DIP. Settings normal 1040x760, minimum 720x540. Tool workspaces normal 1120x800, minimum 720x540. Dialogs normal 640x600, minimum 480x400; if content needs more height it scrolls internally. Pickers normal 360x440, minimum 280x320 bounded by current monitor work area. These are client DIPs, not physical pixel dimensions. For example 720x540 DIP at 200 percent is 1440x1080 physical client pixels. Also test an actually smaller work area with a maximized root and internally scrolling content rather than placing controls offscreen.

Taskbar: inspect actual Explorer slot and measure its real bounds at every scale, rather than pretending it is a 720-DIP window. Cover Windows 10 classical, Windows 11 and Wine routes only when the supported environment exists; missing environments remain unverified. Parent stage-one evidence covers one current host, not every target. High-contrast, larger text, long adapter names, long translated strings, keyboard-only, IME, screen reader, reduced motion, disconnected adapter, device-loss redraw and monitor-DPI transition are additional cases.

Each fixture supplies a fixed sample sequence and time; animations freeze at named states. Capture source SHA, executable SHA-256, build receipt, exact selector, client/outer physical bounds, DPI, locale, font identities, theme, state, route, privacy verdict and timestamp provenance. Retain raw reference, raw product, labelled side-by-side and machine-readable geometry/pixel-diff results. Inspect the images. A reference alone proves only design intent.

Native geometry uses HWND/UIA bounds and measured text/layout rectangles, not DOM APIs. Export a versioned native layout receipt with owning ancestors, measured text extents, clipping regions, logical and physical rectangles, visibility, font and source token. Document this framework-appropriate adaptation; never label it a DOM receipt. Reject overlapping action targets, text outside its owning region, inaccessible overflow and mismatch between UIA and visual bounds. Compare matching tuples only. Record every intentional difference with reason and approval.

## Interaction acceptance

1. Normal login launch keeps taskbar visible and dashboard hidden; explicitly disabled startup remains disabled after update.
2. Tray Open dashboard opens one instance; repeated requests activate it; close hides; collection and taskbar continue; explicit Exit terminates normally.
3. Live values change from real samples, not fixtures; disconnect/stale/error differ from zero; paused display does not pause accounting.
4. Adapter selection affects the intended scope without resetting daily history; detail identity matches the selected stable ID after refresh and reorder.
5. Every rendered control is registered, keyboard-operable and discoverable through UIA with actual matching bounds and state.
6. Every new search and popup filter has its own anchored complete regex route; missing engine capabilities remain unfinished, never fake debug traces.
7. Apply/Cancel, settings restart persistence, theme previews, language changes, local-only private imports, notifications and exports preserve their documented data boundaries.
8. A negative regression removes one handwritten surface/component/search/guidance/evidence row or replaces a component with a generic painted placeholder; each must fail, restoration must pass.
9. Update/uninstall preserve the stage-one configuration and startup boundaries; taskbar reattaches after Explorer recreation without opening the dashboard.
10. Replace current public presentation only with inspected real captures of released state. Preserve historical originals separately. No design SVG is presented as a shipped screenshot.

## Sequenced implementation

- P1: Native foundations, UIA, tokens, fonts, measure/arrange, command router; modeless Network dashboard and adapter details. Preserve every existing functional route. First releasable native slice after its applicable checks, real evidence and release/update verification.
- P2: Setup resume flow, Settings and taskbar/floating-meter editors, connection selection and appearance primitives. Preserve existing setting consumers and migrate config additively.
- P3: History, Sensors, Plugins, notifications, offline documentation and changelog. Existing plugin-owned windows remain separately inventoried.
- P4: Full cross-surface personalization, element guidance/playgrounds, complete search/regex parity, tab management, rich palette, locks/authenticator, local history and exports. Foundations shared across every owned surface; no completion inferred from one example.
- P5: Actual converter, local-model manager and server-manager workspaces, with version/capability inventories and process/privacy boundaries. These are real product work, not empty destinations.
- P6: Complete surface/feature ledger, all tuples, reference parity, genuine recording, publication refresh and accepted-channel update verification. Only this integrated proof can close the full conversion objective.

The first slice is a useful staged release, not permission to delete later requirements. Each phase preserves unfinished rows with exact next action. Build through the repository's root entrypoints. Parent owns integration, publication and release decisions; this design lane changes only `design/`.
