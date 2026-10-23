# Modern native monitoring interface

Design handoff for independent review, not implemented behavior. Source baseline: `f1c6cc59f254ebf37503115636765619b3b35914`. This proposal supersedes the visual composition in `../native-interface-preferences/` while retaining its storage, conflict, migration and lifecycle contracts.

The existing 1.0.9028 interface was rejected visually. Its narrow runtime exercise proved default provenance, searchable draft selection, deferred persistence and a saved bilingual/dark pair in Settings. It did not prove restart, cross-surface refresh, wizard conflict, protected-schema closure or the complete display matrix. The Interface page incorrectly reported no matches while showing controls, used undersized form text, and retained an untranslated footer. Those defects remain open.

## Product direction

TrafficMonitor is a working instrument: see current traffic, identify an adapter, inspect a time interval and change monitoring behavior. The new composition uses a quiet navigation column, a compact command bar, readable data and aligned forms. It removes the wall of equal-priority rounded buttons, oversized empty cards and tiny form captions. No marketing hero, command catalogue or decorative illustration replaces productive controls.

Five native owners share one visual language: monitoring workspace, Settings, setup, searchable menu and regex editor. Native C++/MFC, DirectWrite/Direct2D and actual HWND controls remain. No browser renderer or runtime migration is proposed. Original resource dialogs are explicitly inventoried in `handoff.md`; the five-owner implementation does not silently claim that those dialogs were rebuilt.

## Actual design references

`render_references.py` generates sixty editable SVG references under `references/`: five surfaces × normal/minimum × English/Cantonese/bilingual × light/dark. The SVGs are authored proposals with synthetic example values. They are not application screenshots or runtime evidence. Each has its exact tuple in its filename and an embedded design-only description. They are stored directly with this handoff, not in an external design service.

Start with [workspace](references/workspace-normal-english-light.svg), [Settings](references/settings-normal-english-light.svg), [setup](references/setup-normal-english-light.svg), [menu](references/menu-normal-english-light.svg), and [regex](references/regex-normal-english-light.svg). Compare [minimum bilingual Settings](references/settings-minimum-bilingual-dark.svg) and [minimum bilingual workspace](references/workspace-minimum-bilingual-dark.svg). SVG geometry is in logical pixels; the normal and minimum sizes are stated in `layout-and-tokens.md`.

Two additional component sheets show [light anatomy](references/components-light.svg) and [dark anatomy](references/components-dark.svg), including text fields, action hierarchy, checkbox/switch/range, checked rows and the custom scroll rail. `control-registry.md` maps each raw native control and scrollbar to its replacement and retained accessibility/input behavior. This is a complete visual replacement criterion inside the owned surfaces, not a color-only patch.

References show principal resting states. Conflict, protected storage, validation, empty results, keyboard focus and high contrast use the same components and are specified in `states.md`. Those states still need real built evidence. An SVG contains no executable preference service, monitoring data, regex engine or native accessibility provider.

## Review decision requested

Review the composition, component boundaries, all sixty reference tuples, preserved behavior and state inventory. Resolve any rejected layout before implementation. Approval of this design is not approval of runtime quality or a claim of full required-feature delivery. The implementation handoff and acceptance criteria are in `handoff.md`.
