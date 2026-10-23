# Static documentation renderer

Run the repository root `build.bat --site`. It installs the locked dependencies through the repository's pinned Node route and invokes `docs/site/build.mjs`. Output is `docs/site/dist`, with the fixed project base `/TrafficMonitor/`. Generated output and dependency directories are ignored.

## Delivered source

- Task-oriented overview, explanatory display selector, five-step reading walkthrough with browser-local progress, back/next and reset.
- Complete manifest-selected article pages, category indexes, source links, source-language labels, heading navigation and local image rewriting. Repository articles are read from the exact pinned Git blob and checked against the supplied SHA-256. Wiki originals are read unchanged and checked too.
- Downloads sourced only from the checked-in normal-release metadata. No runtime latest-release lookup.
- Changelog renderer accepts complete upstream translations and preserves fork `originalMessage` verbatim. Plain-text search, date, category and lineage filters operate on locally generated JSON.
- Reading preferences, system/light/dark themes, text scale and reduced motion. English, Cantonese and bilingual navigation labels are available. Long-form walkthrough, settings and feedback localization remains incomplete.
- Local personal-wording upload accepts the version 1 object mapping schema, rejects duplicate/unsafe keys, wrong types, extra fields and oversized files before application. Stored mappings remain browser-local and are never bundled, logged or exported. Clearing restores original controlled labels.

## Component provenance

`@material/web` 2.5.0 supplies real registered filled, outlined and text buttons, outlined text fields, checkboxes and switches. esbuild bundles those implementations locally, retaining linked legal comments. `package-lock.json` pins exact transitive integrity. Native HTML provides document landmarks, links, selects, file inputs and dates. This is a hybrid document surface, not a claim that every HTML control is a Material component.

marked 18.1.1 parses original Markdown. DOMPurify 3.4.17 with jsdom 26.1.0 sanitizes rendered markup before link processing. External images become explicit original-image links; no remote image asset is silently fetched as a current screenshot. Historical documentation is visibly labelled.

## Verification and remaining delivery

Five focused Node tests passed for wording validation and path containment. The first exact `build.bat --site` attempt stopped in the shared download helper because `Get-FileHash` was unavailable. That entrypoint repair is owned separately. After the shared helper repair, the exact entrypoint produced 157 complete articles and zero history records. One historical image source was unavailable and is recorded in build-receipt.json. Browser evidence remains unavailable. A subsequent dependency correction pins jsdom to the supported Node range.

The generated-link audit passed for local destination existence. Anchor validation remains pending. The complete history dataset, refreshed article inventory, deployment, keyboard and persistence drive, current screenshot promotion, full language coverage and normal/minimum theme/scale visual matrix remain acceptance work. Full regex-workbench parity and the broader required-feature inventory are explicitly unfinished. The current implementation offers plain-text search only and does not execute regular expressions or control the installed application.



## Localized interface and registered controls

The interface language layer covers overview explanations, guided reading steps, settings, validation feedback, navigation labels, article-reader framing, download metadata and search result counts in English, Cantonese and bilingual modes. It preserves source articles, quoted code, authored changelog messages and original article titles. Mode changes and personal wording restore the original controlled text without translating URLs, option values or content records.

Settings and filter selects use registered Material Web outlined selects and select options; guided-step actions use Material outlined buttons. The file picker is activated by a Material button and remains a native operating-system picker. Date filters use Material outlined text-field hosts with native date inputs, a type the dependency marks as unsupported for full design conformance. That date-picker limitation remains explicit and requires browser verification.

Focused DOM tests verify language switching and restoration, protected article/history text, localized accessible labels, dynamic counts, stable option values and Material select registration in generated markup. These tests do not establish screen-reader, layout or browser interaction acceptance. The parent verification lane owns genuine built browser evidence and deployment.

## Build-bound history

Every build enumerates all commits reachable from its exact source and absent from the immutable upstream baseline. Fork messages are read directly with Git's full-message format and preserved verbatim, including their complete bodies. Loaded fork records must provide the verbatim method and match the immutable message. No subject-only fallback is permitted.

Upstream translations remain separately reviewed content. The build rejects duplicate identities, missing English summaries and invalid lineage membership. A declared-complete manifest must cover the exact 1,387-commit upstream set. Partial translation counts remain visible alongside the complete current fork count and build source. The generated history JSON is local and source-bound; commit links always identify the exact source commit.

Source-relative extensionless wiki links resolve to their local Markdown articles. Published fork wiki URLs match the manifest's exact `wikiSourceUrl` before historical upstream aliases. Foreign repository links remain external.

## Registered-control typography correction

A real built home-page inspection found the filled-button host and shadow label resolving to `Roboto`, while the body, headings and eyebrow resolved to `Segoe UI, Arial, sans-serif`. No Roboto font was bundled and both Material reference typeface properties were unset. The correction sets `--md-ref-typeface-brand` and `--md-ref-typeface-plain` to the shared local sans-serif stack, including the Traditional Chinese fallback. It does not infer a heading-font defect from appearance alone.

The bundled Material controls also consume secondary-container, highest-surface and error color roles. Those roles now have explicit light/dark values, including system-dark mode, so selection and validation states do not fall back to the package's unrelated default palette. Post-change computed shadow-font values and genuine visual evidence remain the browser-verification lane's responsibility. This is a bounded typography/theme correction, not a full Material conformance verdict.
