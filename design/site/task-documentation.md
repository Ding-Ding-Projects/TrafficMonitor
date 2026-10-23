# Task-led documentation redesign

Status: design handoff for independent review. No rebuilt runtime or visual proof is claimed. This replaces the catalogue-led documentation presentation while retaining every complete source article, wiki route, source link and history record.

## Problem and first implementation boundary

The current Documentation destination leads with category identifiers and the complete article list. The reader has an existing contents rail but no persistent article search or useful related-task navigation. The required-feature category is a specification directory rather than a useful explanation of product outcomes. A visual reskin of those lists does not resolve the problem.

The bounded redesign delivers three connected surfaces: a task-led documentation home, a contextual article reader and a capability explorer. Existing guided setup becomes one of several practical learning paths. Original complete content remains readable independently of JavaScript. No guide implies it executed an installed application action.

## Information architecture and routes

- `/docs/`: Learn TrafficMonitor. Start with a visible Material search field, a resume-learning panel and three primary tasks: Install and find the meter, Understand network activity, Fix an unexpected result. Secondary tasks cover Customize the display, Use plugins and Build from source.
- `/docs/tasks/<id>/`: each task has an outcome, a short prerequisite note, three to five practical steps, a contextual example, related complete articles and visitor-owned progress. Each step contains an actual explanation or decision, never only a link. The existing `/guide/` remains valid and joins this navigation.
- `/features/`: Explore capabilities. Group the complete 110-item inventory by reader benefit, with surface and status filters, visible search and expandable details. Also serve this presentation at the existing `/categories/required-features/` route. Original specification articles and the complete index remain accessible.
- Existing category routes become editorial collection pages: human-readable heading, purpose, a recommended task and relevant article summaries. The complete article index is an explicitly secondary Library section, not the first thing on Documentation.
- Existing article/wiki routes retain their identities. Each reader has breadcrumbs, visible global documentation search, title and description, source-language indicator, collapsible provenance, on-page contents, complete article, relevant next tasks and mark-as-read control.
- Changelog and its filters remain separate. Preserve all 1,387 translated upstream entries and all full verbatim fork messages. No redesign rewrites historical messages.

## Task content and real interactions

Install and find the meter explains installer launch, unsigned-publisher context, taskbar/tray discovery and existing preference preservation. It links the current installation guide and verified release metadata. Understand network activity explains receive/send, selecting a connection and interpreting an idle or disconnected adapter. An interactive diagram switches between upload and download with clear direction labels and explanation, explicitly labelled an illustration with no live measurement. Fix an unexpected result offers a symptom selector for missing meter, zero rates and unexpected connection, revealing concrete checks and the matching complete manual sections.

Customize the display explains taskbar/floating/both choices and links the preserved skin documentation with its historical label. Use plugins explains discovering supported plugins and verifying compatibility, without executing downloads. Build from source explains the exact root entrypoints and the distinction between native product and documentation output.

Progress is local to the browser, keyed by stable task/step identity in a versioned record. Back/Next changes the active explanation, Mark read changes progress, Resume opens the saved unfinished step, Reset affects only that path. Storage failures retain an in-memory session with a localized message. Reading completion never becomes application-setup completion. Existing setup progress keys remain intact.

## Documentation search

One shared search component is rendered on home, every collection, each task, the capability explorer and all article/wiki readers. It uses the locally bundled `md-outlined-text-field`, a labelled search region, clear action, result count and ordinary link results. Search is case-insensitive literal text over human title, summary, headings and complete article plain text. Build a local index from the already hash-verified pinned rendered content. No remote search or user-query telemetry.

Typing changes results without moving focus. Enter follows a focused result through ordinary links, Tab traverses results, Escape clears only the search field when it owns focus. No global keyboard interception. Empty query shows task suggestions; no matches gives an actionable shorter-query suggestion; loading and unavailable-index states are explicit and leave navigation usable. Result excerpts use textContent and never HTML from a query. Limit initial displayed results with explicit Show more, retaining the full match count.

An adjacent labelled Regex workbench disclosure explains that full engine-bound workbench parity is unfinished and keeps plain search available. It must not masquerade as a working regex control or silently execute user input as a regular expression. This staged delivery retains the complete workbench as an open requirement.

## Capability explorer and status truth

Read `docs/features/required-features/inventory.json` from the same pinned repository-content source used for documentation. Validate schemaVersion, declared count, unique stable IDs and the exact item set. Bind emitted capability JSON to that source SHA and inventory hash. Do not maintain a second status copy in renderer source.

Every item receives a human-facing benefit heading and concise explanation, grouped into Reading and accessibility, Personalization, Finding and navigating, Data and tools, Installation and delivery, or Specialized workflows. Group assignment is deterministic and total; fail if any ID is absent or duplicated. Technical IDs remain available in details and provenance, not the leading navigation label. Descriptions must say what the feature would allow without claiming delivery.

Default surface is Native application. Changing the surface reads that item's existing surface status, applicability and reason. Status labels are Not yet delivered, Partially verified and Not applicable, mapped exactly from the inventory. A source status is never upgraded because this website contains a similarly named control. Details show the requirement, baseline, phase, surface reason, eight existing proof dimensions and source specification. Unavailable evidence says Not recorded. Keep every item discoverable, including specialized or inapplicable workflows; filters show both matching and total counts. Clear filters restores all 110 items.

## Layout and Material components

Retain the locally bundled Material Web package and shared typography/theme roles. Use actual outlined text fields, select/options, buttons, checkbox and switch components. Use semantic document sections and links for reading content; do not turn source examples into product controls.

At 1280px, the shell keeps a 208px navigation rail. Documentation content has a 1040px maximum with a two-column task/detail composition; the reader uses a 220px contents rail and readable main text measure. At 320px, navigation wraps, search fills the available width, task choices stack, and the contents rail becomes a labelled disclosure above the article. Cards contain purpose, next action and progress, not raw paths. All flex/grid children permit shrinking. Buttons wrap complete bilingual labels and grow vertically. Tables retain deliberate labelled horizontal scrolling only within their own region, never document-wide overflow.

Light and dark palettes use existing surface/container/outline/primary roles. Focus indicators remain visible in both. No new remote fonts, icon service, CDN or fabricated product screenshot. Illustrative diagrams are semantic HTML/CSS and explicitly labelled. Preserve historical-image notices and source provenance; only genuine inspected source-bound images can become current product presentation.

## Localization, accessibility and states

All authored shell, task text, labels, descriptions, search feedback, progress, capability summaries and status controls support English, Cantonese and bilingual modes using the existing persisted preference. Complete source articles retain their original language. Provide keyboard-accessible disclosure controls, labelled filters, polite result announcements, skip navigation and correctly nested headings. Preference changes must update new dynamic content and semantic names without changing source text.

States to verify: documentation first visit, saved progress, storage unavailable, task step changes, symptom selection, diagram choice, search empty/results/no-results/index unavailable, article contents expanded/collapsed, capability surface/status filtering, zero matches, expanded evidence, clear filters, source-link follow and deep-link reload.

Every state has normal1280 and minimum320 viewport tuples, English/Cantonese/bilingual, light/dark and100/125/150/200 percent display scale. Reduced motion removes animated transitions; no progress or information depends on motion. Parent verifies actual built/deployed tuples, keyboard interactions, persistence and zero document overflow. Design references are not runtime evidence.

## Implementation ownership and acceptance

GUI lane owns only `docs/site/**` and `design/site/**`. Add cohesive modules for task content, documentation rendering/search and capability mapping instead of expanding the existing mixed build template. Parent owns manifest refresh, root build entrypoints and deployment. Preserve deep links under `/TrafficMonitor/`, wiki alias resolution, hash checks, sanitization, personal-wording privacy and history validation.

Independent review precedes implementation. First implementation checkpoint delivers Documentation home, one complete multi-step network learning path, shared reader search and the complete capability explorer. Remaining task paths are completed in the same redesign before claiming the documentation-flow request finished. Acceptance requires real interaction and persistence, full article/asset inventory, all110 capability records, unchanged complete history, actual deployed routes and inspected narrow bilingual states. Full regex parity and unrelated universal features remain explicitly unfinished.
