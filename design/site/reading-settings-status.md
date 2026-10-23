# Reading settings implementation boundary

The documentation Settings page adds six local reading options: interface density,
interface font, article width, article line spacing, accent palette, and the default
expansion of collection filters. Material Web select and switch controls update
the actual interface. Filter search remains visible; the disclosure retains an
active-filter count. Previously saved expansion takes priority over the default.

Reading layout and collection expansion have separate reset actions. Version 1
JSON import/export includes only these six preferences, with UTF-8 and LF metadata.
Imports are limited to 8 KiB, validate the complete allowlisted schema, and preserve
the previous state on invalid input or unavailable storage. This does not export
personal wording, queries, experiments, installed application data or all settings.

The Settings search and its independent local regex workbench include this group.
English, Cantonese and bilingual modes use the existing localization mechanism.
System fonts are selected locally; no font download is introduced.

`settings-contract.json` retains all 110 source identities, applicability, reasons,
required behavior, existing partial consumers, persistence, reset and recovery.
Applicable items without a consumer remain unfinished. Inapplicable items have no
invented preference. Full per-element appearance editing, schedules, all export
formats, complete regex parity and other missing settings are still unfinished.

Source checks do not establish browser appearance, keyboard behavior or responsive
acceptance. The integration owner must verify the built settings and article pages,
all language/theme/scale tuples, persistence, import recovery and narrow layouts.
