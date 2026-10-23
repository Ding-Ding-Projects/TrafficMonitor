# Inventory acceptance and remaining executable checks

## Inventory structure

The explicit expected list is [expected-ids.json](expected-ids.json), with 110 public stable IDs. [inventory.json](inventory.json) must contain each exactly once, and the category README must link all 110 articles. Each item must preserve all eight named surfaces and all eight proof dimensions. A future validation implementation compares this fixed expected list, not only whatever files it discovers. Regeneration must never drop an absent requirement from the expectation list.

Required proof dimensions are implementation, documentation, localization, persistence, focusedTests, negativeRegression, builtInteraction and screenshots. Each has status, expected evidence and evidence references. Empty evidence lists and UNFINISHED states deliberately remain visible. PARTIAL is not an acceptance state. An item cannot be accepted merely because this folder exists or a static schema validation passes.

## Negative omission procedure still required

- [ ] Remove each expected ID, article, surface record and proof dimension separately; the completeness validator must reject each mutation and accept restoration.
- [ ] Remove each asserted implementation registration, localized resource, persistence path, focused semantic test, interaction receipt or screenshot record independently.
- [ ] Reject stale source/build hashes, wrong state/tuple, missing inspected-pixel verdicts and references to unrelated earlier builds.
- [ ] Reject unsupported acceptance states, unexplained nonapplicability, duplicate IDs and discovered-only expectation lists.
- [ ] Exercise each item-specific negative regression from the article and verify actual red-then-green behavior.

These checks are a specification. No executable completeness validator or negative-regression result was produced by this documentation inventory task.

## Built and deployed interaction proof

Every applicable runtime surface requires its own real interaction and persistent-state proof, not a sibling demonstration. Bind receipts to exact source, package/executable hash, screen/state, language, theme, viewport, scale and actual route. Inspect genuine images for useful content and privacy. Verify normal and minimum supported sizes across three languages, both themes and four display scales. Browser-only documentation must document its native mediation boundary and never claim process control from static examples.

For nonvisual repository requirements, verify actual stored files, release assets, API readback, job outcome and protection state instead of fabricating runtime images. For conditional no-state/no-UI subrequirements, record the exact reason at that boundary while retaining the item. Required hosted documentation and companions cannot be marked not applicable just because their implementation has not started.

## Stage1 receipts remain bounded

The current 1.0.3/1.0.4 receipts support native installer/lifecycle facts only. A hidden main window and Explorer taskbar attachment do not prove Material Design 3 conversion, redesigned settings, other languages, alternate scale tuples or universal tools. Subsequent sign-in, Explorer restart, fresh-host acquisition and cancellation remain unverified. Never overwrite or relabel original evidence to expand its claims.

## Documentation and publication checks

Review each article for real configuration/defaults, failure behavior, privacy and source references, and publish the corresponding Cantonese article. Verify all relative links and every live internal article/wiki route, current images, installer download bytes, homepage field and social metadata. Run the actual tracked-source publication scanner and normal local hooks before commit; private source references and operational mappings stay outside this folder. The integrating maintainer owns default-branch integration and remote proof.
