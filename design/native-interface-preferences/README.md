# Native language and appearance preferences

Design proposal, awaiting independent review. Baseline: `575c871e0b6a52667bce1b77f69d62634e457455`. No production implementation or runtime verification accompanies this document.

## Scope and user outcome

Choose English, Cantonese or bilingual presentation, and System, Light or Dark appearance once. The current native workspace, setup, settings, searchable menu and regex editor use the same accepted preferences immediately and after reopening. Monitoring, tray/taskbar identity, adapter selection, plugins, startup actions, update restart protection and legacy resource-language configuration retain their existing behavior.

The design remains native C++/MFC, DirectWrite/Direct2D and Win32 controls. It is a bounded language/appearance increment, not completion of the full settings inventory, regex parity, personal wording, adjustable colors or typography, message-style delivery, documentation, or legacy-dialog modernization.

## Source evidence and boundaries

| Existing owner | Observed baseline | Required change boundary |
| --- | --- | --- |
| `TrafficMonitor.cpp`, `InitInstance` | `LoadGlobalConfig` selects portable/module or application-data storage; installed mode overrides to application data; `LoadLanguageConfig` then sets the thread language | Initialize new preferences after these decisions, never derive a second directory or alter `language_id` |
| `NativeDashboard/Workspace.cpp` | `m_chinese` derives from legacy LANGID; appearance reads registry once; ThemeId flips local Light/Dark | Read shared snapshot; replace toggle with direct navigation to interface preferences; refresh existing controls and presentation without restarting sampler |
| `NativeDashboard/Presentation.cpp/.h` | Rate, state, details and chart text take `bool chinese` | Consume language mode/text service; preserve values, GUIDs, timestamps and sample history |
| `NativeDashboard/SetupFlow.cpp` and `SetupFlowText.h` | `dashboard-setup.ini` schema 1 stores language 0/1/2, style values and action receipts; text pairs support three modes; system appearance read locally | Preserve progress/actions; initialize new draft from accepted preferences; commit language/appearance explicitly; centralize owned copy without destroying resumable wizard state |
| `NativeSettings/Settings.cpp`, `SettingCatalog.cpp/.h`, `Values.cpp` | Local language boolean and appearance toggle; four setting groups plus discovery; general setting draft is separate | Add interface group with real selectors, its own bounded Apply/Revert/Reset workflow; include entries in existing discovery and filter; preserve numeric drafts and existing validation |
| `NativeMenus/SearchableMenu.cpp` | Reads HMENU text/command IDs, derives language from LANGID; has accessibility provider and theme-change handling | Localize owned chrome and known owned command labels; preserve command IDs, shortcuts, states, submenu ownership, plugin-provided text and opaque-item fallback |
| `NativeRegex/Workbench.cpp/.h` | Caller passes language boolean and appearance snapshot; asynchronous validation/collections and independent draft | Read shared snapshot, refresh chrome safely; preserve query, replacement, selections, running work and accepted owner search |
| `NativeMaterial/Theme.cpp/.h` | Effective appearance is Light/Dark; `Palette::Resolve` already prioritizes high contrast | Keep this rendering enum; resolve System at shared boundary; high contrast remains authoritative |

## Shared state and storage

Proposed `NativePreferences/Preferences.h/.cpp` owns typed `LanguageMode { English, Cantonese, Bilingual }`, `AppearanceMode { System, Light, Dark }`, immutable `Snapshot` (including revision and effective appearance), `Load`, `Current`, `TryApply`, and `ResetDefaults`. Separate `Storage` handles validation and replacement; `Text` owns stable localized IDs and presentation pairs. Do not put storage or UI callbacks into `NativeMaterial::Palette`.

Persist a small versioned `native-interface.ini` under the already selected `theApp.m_config_dir`. Schema 1 contains only validated language and appearance enum values. File size cap: 16 KiB. No paths, personal mappings or user text belong in this schema. This separation avoids a later `SaveConfig()` rewriting stale copies of native preferences, and preserves all unrelated `config.ini` keys. Proposed names are implementation contracts, not claims of existing APIs.

On missing file: use valid language 0/1/2 from existing schema-1 `dashboard-setup.ini` if explicitly present; otherwise derive English/Cantonese from the existing effective legacy LANGID, matching the baseline Chinese-primary behavior. Appearance defaults to System. Do not write a migrated file merely by opening a surface. Record in-memory provenance as saved, setup migration, legacy fallback or default. On malformed existing preferences, retain bytes, use safe fallback and show an actionable recoverable warning. Unknown schema is read-only until explicit reset; do not silently downgrade it. Reject invalid individual values as a complete invalid document, never partially apply it.

`TryApply` validates the whole proposed pair, writes a same-directory unique temporary file, flushes/closes, atomically replaces the destination and reads it back before publishing a new in-memory revision. Use the existing supported Windows replacement idiom with real error propagation; never report saved after a failed write, replacement or readback. Preserve the previous accepted snapshot and file when replacement fails. If readback fails after replacement, report an uncertain save and reload/reconcile before further edits, rather than falsely claiming rollback. Remove only the operation-owned temporary file. Serialize writers in this process; external modification detected since loading is a conflict requiring reload or explicit retry, not last-writer-wins data loss. No cross-process broadcast or global registry write.

Reset changes only these two preferences, through the same explicit Apply path. Reset defaults are the startup-derived default language and System appearance, disclosed beside the control. Reset never erases setup progress, legacy LANGID, settings drafts or personal data. Failure leaves the dialog open with selected draft and Retry/Revert controls.

## Native interaction design

Use a sixth Settings destination, **Language and appearance**, after the four monitoring groups and before discovery, with stable semantic group identity rather than scattered numeric assumptions. Rename the existing Appearance shortcut to **Language and appearance** and navigate directly to this group. Workspace offers that same destination. Do not cycle hidden preferences through a toolbar button.

The page title is **Make monitoring comfortable to read**. Under it show two stacked fields:

1. **Interface language**, native dropdown with English / 廣東話 / English + 廣東話. Supporting text: affects the new native monitoring workspace, setup, settings, menus and search editor; original dialogs and plugin wording keep their own language.
2. **Appearance**, native dropdown with System / Light / Dark. Supporting text displays effective appearance for System and explains high-contrast precedence when active.

A compact, non-interactive sample below shows a localized monitoring-status label and sample value explicitly marked as a preview. The draft preview affects only this sample, not other open windows. A local footer carries **Apply interface preferences**, **Revert interface changes**, **Reset interface defaults**, and a live saved/unsaved/save-failed status. Existing monitoring-group Apply/Reset must never silently save or discard interface changes, or vice versa. Switching groups preserves each draft; closing with either dirty domain uses the existing discard workflow extended to identify both. No modal confirmation for an ordinary successful save.

Setup step 1 exposes the same language and appearance choices. Its draft language may localize the wizard as today; an explicit **Apply interface preferences** action persists and shares the pair without executing adapter/taskbar/startup actions. Step Next and Save-and-close retain wizard drafts without claiming globally applied preferences. Review names the accepted pair separately from an unapplied draft; finishing requires applying or explicitly keeping the accepted pair. Existing style controls and saved values stay intact, with current limitations still disclosed. A newer accepted preference revision must not overwrite a dirty setup draft on resume: show that saved preferences changed and offer Use saved or Keep draft.

## Localization and rendering API

`TextId` tables separated from behavior contain English and Traditional Chinese Cantonese text for all owned strings in these surfaces, including status/result chrome, validation and accessible names. Reuse and migrate existing pairs rather than leave parallel local T helpers. Production files containing non-ASCII text retain UTF-8 BOM and the established compiler character-set behavior.

Return a `TextRun { primary, secondary }` for display, plus a complete accessible string. English and Cantonese have one run. Bilingual uses English primary and Cantonese secondary. Draw secondary underneath in compact but legible type, not an ever-longer inline concatenation. Standard buttons require measured two-line support or a short primary label plus adjacent secondary label with one accessible action name; do not merely inject newline into controls that cannot render it. Dropdown values remain stable IDs, independent of translated positions. Human names, expressions, adapter aliases, paths, engine diagnostics, versions and command shortcuts are not translated.

Known owned HMENU commands use a curated command-ID-to-TextId map; preserve the native text for unknown/plugin commands and disclose this boundary. Never switch thread LANGID to reload resources, mutate caller HMENU strings, translate arbitrary text or rewrite plugin names. Original resource dialogs remain on legacy language configuration. This increment cannot claim application-wide legacy bilingual coverage.

## Active windows and lifetime

The application owns the service on its UI thread. Each top-level native surface registers after HWND creation and unregisters before destruction through an RAII subscription. Use a process-local dedicated WM_APP message, subscription identity/generation and latest revision, not HWND_BROADCAST, raw captured `this` callbacks, or pointers to temporary snapshots. Validate the registration identity in the recipient before reading the current snapshot so a reused HWND cannot consume stale work. A queued notification contains no owning pointer. Destruction invalidates registration before controls, accessibility provider state or fonts are torn down.

On a successful preference change, each live surface updates captions, accessible names, cue banners, columns, group labels and brushes, recomputes measured layout, and invalidates. Preserve focused control ID, caret/selection, expanded details, scroll anchor, selected stable adapter/command identity and all user drafts. Recompute translated search subjects without changing query/pattern/flags; discard stale asynchronous filter results by existing generations and rerun against the current subjects. Do not cancel unrelated regex collection writes or sampling work. Structured status IDs allow a status to relocalize without rerunning its operation; third-party diagnostic payload remains literal.

An active menu preserves its submenu stack, command selection and search state while relayout occurs; if its owner is being destroyed it follows existing cancellation. A regex editor independently subscribes, so it does not depend on the invoking window still being alive. Shutdown destroys subscriptions before service destruction. Notification delivery never posts to another process.

System appearance resolves `AppsUseLightTheme` read-only, defaulting to Light when unavailable. On WM_SETTINGCHANGE/WM_THEMECHANGED refresh effective appearance only for System, while all modes refresh high-contrast palette. Deduplicate unchanged effective snapshots to avoid notification loops. WM_DPICHANGED recalculates fonts and geometry without changing preferences.

## Review and delivery condition

Independent design review must resolve any API/lifetime or draft-domain ambiguity before implementation. The source map is inspected; visuals, functionality, full layout matrix, installation, release, and cross-surface persistence remain unverified. See `states.md` and `references.md` for implementation acceptance and deterministic design geometry.

## Independent review corrections (required before implementation)

`TryApply(candidate, expectedBase)` accepts the draft's accepted revision AND durable base fingerprint. Reject stale same-process revisions or changed stored fingerprints before writing; retain the candidate and show current accepted values. Use saved discards only this preference draft. Reapply retained draft is a deliberate action after refreshing its base, never an automatic retry.

Wizard schema 2 persists language, appearance, preferenceDirty, baseLanguage, baseAppearance and baseFingerprint alongside every existing step, style, adapter, startup, taskbar and operation receipt. Schema-1 migration checks actual key presence with an out-of-range sentinel; absent language is not an explicit English selection. An explicit old language becomes a retained draft based on the accepted pair, and appearance inherits that pair. Missing/invalid bases require reconciliation before applying. Unknown schema prevents all incidental saves and completion writes. Saving draft A, accepting B elsewhere and restarting must resume A with a conflict against B, never replace B from A merely by opening or navigating.

Each selector is a labelled native button that opens its own searchable checked menu, not an ordinary combo box. Language and appearance retain independent option/filter/pattern/flag states and independent regex-editor ownership. Stable enum-backed command IDs select drafts only; menu cancellation leaves draft unchanged. Each menu exposes its selected checked item, search field and adjacent independent NativeRegex entry. Its accessible role is button/menu, not falsely reported combobox. Escape closes regex/menu before the parent, IME composition never applies a preference, and cancellation returns focus to the originating selector. Reuse NativeMenus machinery with explicit retained option state; global Settings search is not a substitute.

Settings has exactly six destinations: the four existing monitoring groups, Interface at index 4, Discovery at index 5. Measure each navigation label and wrap whole buttons across rows. Derive search/status/body origins from the measured header bottom. Preserve the 760 x 640 minimum client size. Measure bilingual action labels, selector labels, navigation, status and accessible names as well as explanations; no equal-width six-way squeeze.
