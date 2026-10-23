# Offline release snapshot labeling amendment

Status: proposed for independent review, not implemented. Scope is truthful release labeling in the existing documentation companion. No layout redesign, native application change, release refresh, external navigation change or packaging change is proposed.

## Evidence and distinction

The single-file documentation companion 1.0.9021, built from c4231a8776e884c629274fb075068763e1c77fb9, displayed `v1.0.15-run39912330939-1` in its header and download presentation. That value is the release manifest bundled with the documentation. It is neither the companion package version nor the installed native application's version, and it does not establish the currently available release. The observed executable SHA-256 is `8cadcc94940a383786c4b53ff6576b1445fffbe8cd908e488f321d5841c7bc49`. This amendment does not relabel that historical executable as repaired.

## Display contract

Retain the existing brand, navigation, typography, spacing and controls. On the exact offline origin `traffic-docs://reader`, replace the bare header release value with a wrapping text label: **Bundled release snapshot: {tag}**. The tag remains literal data escaped by the existing renderer. Do not use Latest, Current release, Installed version or Companion version for this value.

Home prefixes its release detail line with the same snapshot label and adds one short explanation beside the existing download action: **Release information was bundled with this reader and may be out of date. External links do not open in the offline reader.** Downloads uses **Bundled release snapshot** instead of **Verified release input**, retains the exact recorded publication and verification values, and adds the same freshness explanation. Existing publication facts describe that named historical release only. The download action must not imply that the offline reader installs or downloads anything; use **Installer link from bundled snapshot** while retaining its existing blocked destination and link semantics.

The footer distinguishes **Documentation source: {source}** from **Bundled release metadata checked: {recorded value}**. If no verified check time is supplied, render **Check time unavailable**. A release publication date is not a package build date. Never derive any date from a filename, file timestamp, current clock or release tag.

No new companion package version or build date is displayed in this bounded amendment. The current page-rendering context has a documentation source and a release manifest, but no validated companion identity context. A future producer-bound package identity may be displayed only through an explicit validated input, with independently named fields and missing-value states. The existing recorded documentation source remains valid and is not renamed package source.

Hosted HTTPS documentation keeps its current release presentation and link behavior. The offline labels must never appear merely because the browser is disconnected, a fetch fails or a query parameter requests them. Delivery mode is selected only by the exact trusted scheme and host. An unexpected custom origin fails closed to a neutral snapshot description rather than asserting freshness.

## Localization and accessibility

| English | Cantonese |
| --- | --- |
| Bundled release snapshot | 隨附版本資料快照 |
| Release information was bundled with this reader and may be out of date. External links do not open in the offline reader. | 版本資料隨閱讀器一同封裝，可能已經過時。離線閱讀器唔會開啟外部連結。 |
| Installer link from bundled snapshot | 隨附快照內嘅安裝程式連結 |
| Documentation source | 文件來源版本 |
| Bundled release metadata checked | 隨附版本資料核實時間 |
| Check time unavailable | 未提供核實時間 |

Use the existing language mechanism for English, Cantonese and bilingual modes. Labels and accessible names must agree. The full release tag remains selectable and readable; wrap long values without ellipsis. Do not add a modal, notification or new focus stop. Keep the keyboard order and existing links. Supporting explanations are ordinary text adjacent to the relevant data and action, not hover-only tooltips or color-only badges.

## State inventory and verification tuples

- `offline-header-snapshot`: every local route, tag present, absent check time.
- `offline-home-snapshot`: snapshot prefix, freshness explanation and blocked installer-link description.
- `offline-downloads-snapshot`: historical publication facts and explicit snapshot scope.
- `offline-provenance`: separate documentation source, recorded release-check time or unavailable state; no invented package identity.
- `hosted-unchanged`: HTTPS shell, home and downloads retain current behavior.
- `offline-localization`: English, Cantonese and bilingual labels persist through existing preferences.

Normal companion outer size is 1180 by 820 pixels at 96 DPI; the observed renderer was 1164 by 781 CSS pixels. The native minimum is an outer 640 by 480 window, so remeasure its actual renderer area rather than treating the outer dimensions as CSS dimensions. Verify wrapping at that actual minimum, both themes and all three languages. The broader 100/125/150/200 percent matrix remains an acceptance duty and cannot be claimed from these source notes. The hosted 320-CSS-pixel behavior remains unchanged but requires a focused negative regression for the new labels.

## Implementation handoff and scope request

`docs/site/lib/pages.mjs` supplies the explicit release/provenance wrappers and alternate localized text. `docs/site/src/locale.js` may register directly related copy only. Focused new tests cover exact-origin selection, escaped tag data, unavailable time, no invented package version/date, accessible label agreement and unchanged hosted output. Do not edit the separately owned `docs/site/test/content.test.mjs` fixture.

A narrow additional runtime path is necessary: `docs/site/src/app.js` must select the exact offline delivery mode and activate those labels before normal localization runs. The existing build generates one static HTML output for both hosted and offline delivery, and the content security policy disallows inline scripts. Changing only pages.mjs cannot reliably select an origin-specific presentation while preserving hosted behavior. Request approval for only that initializer before implementation; do not weaken the content security policy, change packaging or inject an inline script as a shortcut.

Reuse existing style rules if they already wrap the header. Any required `docs/site/src/style.css` edit must be separately scoped to wrapping the release label and explained from actual measured bounds. No styling edit is assumed in this handoff.

Independent review must approve this amendment and its runtime-path scope before implementation. Then build through the exact root entrypoints, retain source-bound genuine before/after captures with actual capture timestamps, verify the hosted negative case and record remaining gaps. No design preview or prose document is runtime proof.
