# Existing control and action destinations

Inspected against production baseline f1c6cc59. This is a required migration inventory, not proof of implementation. Existing command IDs remain stable. Native ownership, worker lifetimes, input semantics and draft boundaries do not change with visual placement.

## Workspace.cpp

| ID | Existing function | New destination |
| --- | --- | --- |
| HistoryId 52001 | Open original traffic history | History navigation item; retains original dialog until its mandatory later redesign |
| PluginsId 52002 | Open plugin manager | Plugins navigation item |
| SettingsId 52003 | Open Settings | Settings navigation item |
| FloatingId 52004 | Toggle floating monitor | Always visible compact toolbar toggle with current state and full accessible name |
| ThemeId 52005 | Open Interface preferences | Settings > Interface shortcut in contextual navigation |
| PauseId 52006 | Pause/resume display, collection continues | Always visible Pause display / Resume display toolbar action; status states collection continues |
| RefreshId 52007 | Stop/restart sampler and clear sample model | Restart samples toolbar action, never a page refresh icon with ambiguous meaning |
| SearchId 52008 | Filter adapters | Persistent search field |
| ClearId 52009 | Clear owning search | Named trailing Clear action in search field |
| TabsId 52010 | Three data tabs | Adapters, Sample history, Details, shown in every workspace reference |
| AdaptersId 52011 | Native adapter table and selection | Adapters tab; stable adapter identity retained |
| SamplesId 52012 | Sample table | Sample history tab; extra normal/minimum references show selection |
| DetailsId 52013 | Selectable read-only adapter details | Details tab; extra normal/minimum references show details |
| StatusId 52014 | Live/paused/filter status | Measured status below contextual toolbar |
| SummaryId 52015 | Monitoring summary | Metric strip and accessible summary |
| RegexId 52016 | Independent owning search editor | Adjacent Regex action |

Workspace has exactly three existing data tabs, not four. The reference values are synthetic. Toolbar actions are visible at normal and minimum sizes, while the data body scrolls only when its declared content height exceeds its viewport. Sample and details references show the body at its lower scroll position when required.

## SetupFlow.cpp and shared Editor

| ID | Existing function | New destination |
| --- | --- | --- |
| Language 55001 | Retained legacy choice, hidden after shared editor integration | Remains hidden compatibility state, never a duplicate language selector |
| EnglishStyle 55002 | English style range 1..5 | Step 1 below preference preview/apply domain; full numeric value and accessible range |
| CantoneseStyle 55003 | Cantonese style range 1..5 | Step 1 adjacent vertical group, same semantics |
| ModeChoice 55004 | Monitoring mode | Monitoring step |
| AdapterChoice 55005 | Stable adapter choice | Monitoring step with refresh result |
| TaskbarChoice 55006 | Taskbar action preference | Existing setup step assigned by actual progress model, retained in migration |
| StartupChoice 55007 | Startup action preference | Existing startup step, real result retained |
| Back 55008 / Next 55009 | Step navigation | Fixed footer; neither implies global preference Apply |
| SaveClose 55010 | Save wizard draft and close, or protected close without saving | Fixed footer with truthful protected-state label |
| Refresh 55011 | Rediscover adapters | Monitoring step adjacent to adapter choice |
| KeepRemaining 55012 | Keep current choices for optional remaining actions | Existing optional-step route and review, not discarded |
| Editor 56200 / 56201 | Language / appearance searchable selectors | Step 1 shared preference sections, independent menu state |
| Editor 56202 | Explicit Apply preferences | Step 1 preference domain, above style ranges and separate from Next/Save and close |
| Editor 56203 / 56204 / 56205 | Use saved / Reset / explicit Reapply | Preference status/action region; Reset text action; Reapply shown only for conflict |
| Editor 56210 | Draft preview | Always present within the scrollable preference domain |

Initial and scrolled step-1 references exist for every size/language/appearance. They show the same content at different offsets, never delete the preview or style controls to fit. Style values retain their current documented limitations; visual redesign adds no unsupported style consumer.

## Workbench.cpp

| ID | Function | New task destination |
| --- | --- | --- |
| Pattern 55001 | Pattern/literal edit, limit 256 | Shared expression field above each task panel |
| Subject 55002 | Local subject, limit 8192 | Test panel; stacked above results at minimum width |
| Replacement 55003 | Literal replacement, limit 4096 | Replace panel, explicitly no group substitution |
| Mode 55004 | Literal/regex mode | Shared expression options, labelled toggle, separate from Case |
| Case 55005 | Exact/ASCII-ignore case | Shared expression options, labelled toggle |
| Run 55006 | Search execution | Test panel primary Search action; keyboard Enter retains actual execute behavior |
| Replace 55007 | Replace preview | Replace panel explicit action |
| Split 55008 | Split preview | Replace panel separate Split preview action, never conflated with Replace |
| Apply 55009 | Accept options into owning search | Fixed footer Apply to search |
| Cancel 55010 | Close without owner mutation | Fixed footer Cancel and draft-safe Escape |
| Results 55011 | Match selection | Test results panel |
| InsertAny 55012 | Insert . | Expression helper strip: Any |
| InsertDigit 55013 | Insert [0-9] | Expression helper strip: Digit |
| InsertWord 55014 | Insert supported word expression | Expression helper strip: Word |
| InsertGroup 55015 | Insert noncapturing group | Expression helper strip: Group |
| InsertStart 55016 / InsertEnd 55017 | Insert anchors | Expression helper strip: Start / End |
| SavedList 55018 | Select stored expression | Library panel named searchable selector |
| SavedName 55019 | Collection name, limit 96 | Library panel text field |
| SaveNew 55020 | Validate/save new expression | Library panel Save new |
| LoadSaved 55021 | Load selected expression into draft | Library panel Load draft, no implicit owner acceptance |
| RenameSaved 55022 | Rename stored expression | Library panel Rename |
| DeleteSaved 55023 | Confirm/delete stored expression | Library panel Delete, existing confirmation retained |
| RecoverSaved 55024 | Confirm/recover backup | Library panel Recover backup with actual result |
| RefreshSaved 55025 | Reload collection | Library panel Refresh |
| output, control ID 0 | Read-only replacement/split output | Replace panel output editor, named explicitly despite anonymous old ID |
| engine/help/note/collectionNote | Engine identity, supported syntax, limits, operation/collection state | Explain panel for full syntax/limits; persistent compact engine identity and operation status; Library local status |

The four task tabs organize existing controls, not invented capabilities. Explain presents existing engine/help text and explicit unsupported features; it is not a debugger. Mode, Case and all six insertion helpers stay reachable from the expression region through a named expanding options section with keyboard access. Library actions are not silently hidden behind an unnamed generic menu. Each panel needs its own measured layout and preserved focus/scroll state during implementation.

The example pattern is `Ethernet`, which the retained bounded NFA supports. The references must not claim a match for unsupported word-boundary escapes. Separate minimum bilingual subject and results references demonstrate reachable stacked content. The range and thumb derive from the complete panel content, not an arbitrary decorative ratio.

## Release 48 refinement: Interface disclosure

Proposed `NativePreferences::Editor::InterfaceDetails=56206` is a new presentation-only action, unused in baseline `25ad3adcb8c5cdf7c0d32a62c065a0b612b5fafd`. A visible Details button beside the preference section title toggles measured `m_scope` content in the existing body; Settings and setup forward the same shared-editor command. It exposes scope and actual defaults/migrated/saved provenance, preserves draft/base and independent selector search state, and never writes preferences. Conflict/protected/write-result messages remain inline in `m_note`. See [exact mapping and behavior](release48-refinement.md#interface-details-action-and-content). This is proposed wiring, not implemented runtime behavior.
