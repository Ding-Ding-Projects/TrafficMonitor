# Inventory acceptance and remaining executable checks

## Inventory structure

The explicit expected list is [expected-ids.json](expected-ids.json), with 110 public stable IDs. [inventory.json](inventory.json) must contain each exactly once, and the category README must link all 110 articles. Each item must preserve all eight named surfaces and all eight proof dimensions. The local validator additionally retains an independent handwritten 110-ID list, so changing discovery or the expected file cannot silently remove an identity. These are the reviewed public identities; private naming data is neither required nor shipped.

Required proof dimensions are implementation, documentation, localization, persistence, focusedTests, negativeRegression, builtInteraction and screenshots. Each has status, expected evidence and evidence references. Empty evidence lists and UNFINISHED states deliberately remain visible. PARTIAL is not an acceptance state. An item cannot be accepted merely because this folder exists or a static schema validation passes.

## Local structural and evidence audit

Run `node scripts/required-feature-audit.mjs` from the repository. Diagnostic mode exits zero only when the existing declarations are structurally valid; its JSON explicitly reports unfinished items, applicable surfaces, missing independent surface proof and historical links without source-bound acceptance. `complete: false` and its gap count remain visible. This is not product acceptance.

Run `node scripts/required-feature-audit.mjs --require-complete` for the acceptance gate. Any unfinished applicable item, surface or dimension, missing independent surface proof, invalid declaration or invalid asserted evidence exits nonzero. Neither mode runs tests in CI, changes the inventory or executes the application. No workflow was changed.

Both modes default to the exact current HEAD. `--source <full-commit>` selects the product source under audit, and `--evidence-source <full-commit>` selects the immutable revision containing receipts and images (default HEAD). Keeping these separate allows evidence to be committed after a build without pretending a receipt knew its own future commit hash. `--inventory <file>` permits a prepared declaration file, including isolated negative-test inputs; the expected identities remain the repository-owned reviewed input plus the independent list.

Each existing shared item proof retains its eight dimensions. An applicable surface cannot become COMPLETE until its own `proof` object contains those same eight dimensions. Each dimension has `status`, nonempty `expected` and an `evidence` array. Supported states are UNFINISHED, PARTIAL, COMPLETE and NOT_APPLICABLE. Surface applicability is APPLICABLE or NOT_APPLICABLE with a nonempty specific `reason`; the corresponding status must agree. A genuinely inapplicable dimension within an applicable surface needs its own `reason`. Those reasons remain reviewable declarations, not mechanically proven applicability judgments.

An accepted evidence entry is an object with `featureId`, `surface`, repository-relative `path`, exact audited `sourceCommit` and `sha256`. Shared item evidence uses surface `shared`; individual surface evidence names that exact surface. The file must exist at the evidence revision and match its digest. Historical article-relative strings remain allowed only in unfinished/partial records, are checked for a real source file and always add an acceptance gap.

Built-interaction, screenshot and negative-regression evidence refers to JSON receipts that declare the same source, feature and surface and `verified: true`. Runtime receipts additionally require `buildSHA256`, `state`, `route`, `language`, `theme`, positive `scale`, positive integer viewport width/height and `privacyReviewed: true`. Screenshot receipts add `pixelsInspected: true`, `imagePath` and `imageSHA256`, with actual immutable image-byte verification. Negative receipts require `redObserved: true` and `restoredGreen: true`. These checks cannot establish that a human declaration is honest or that the described interaction occurred; independent review and real runtime evidence remain mandatory.

Run `node scripts/test-required-feature-audit.mjs` locally for the focused mutation checks. The script removes every required identity, every named surface and every dimension in isolated inventory copies, observes rejection, and verifies restoration. Additional cases cover duplicates, invalid applicability, empty reasons, missing evidence arrays/files, stale evidence source, wrong hashes, unrelated evidence ownership, unsupported completion claims and diagnostic/strict CLI outcomes. No production inventory files are edited.

## Remaining executable product checks

- [x] Reject removal of every expected ID, surface record and proof dimension, duplicate identity and malformed applicability; restoration returns the diagnostic inventory to structurally valid.
- [x] Reject absent evidence arrays/files, stale source identities, wrong evidence digests, unrelated evidence ownership and complete claims without independent proof.
- [ ] Remove each required article from an immutable source fixture and verify the article-absence path.
- [ ] Remove each asserted implementation registration, localized resource, persistence path, focused semantic test, interaction receipt or screenshot record independently.
- [ ] Reject stale source/build hashes, wrong state/tuple, missing inspected-pixel verdicts and references to unrelated earlier builds.
- [ ] Reject unsupported acceptance states, unexplained nonapplicability, duplicate IDs and discovered-only expectation lists.
- [ ] Exercise each item-specific negative regression from the article and verify actual red-then-green behavior.

The local structural/evidence validator and its mutation checks implement only the bounded checks described above. The baseline inventory remains unfinished. Product-specific negative regressions and complete built/deployed acceptance are still required.

## Built and deployed interaction proof

Every applicable runtime surface requires its own real interaction and persistent-state proof, not a sibling demonstration. Bind receipts to exact source, package/executable hash, screen/state, language, theme, viewport, scale and actual route. Inspect genuine images for useful content and privacy. Verify normal and minimum supported sizes across three languages, both themes and four display scales. Browser-only documentation must document its native mediation boundary and never claim process control from static examples.

For nonvisual repository requirements, verify actual stored files, release assets, API readback, job outcome and protection state instead of fabricating runtime images. For conditional no-state/no-UI subrequirements, record the exact reason at that boundary while retaining the item. Required hosted documentation and companions cannot be marked not applicable just because their implementation has not started.

## Stage1 receipts remain bounded

The current 1.0.3/1.0.4 receipts support native installer/lifecycle facts only. A hidden main window and Explorer taskbar attachment do not prove Material Design 3 conversion, redesigned settings, other languages, alternate scale tuples or universal tools. Subsequent sign-in, Explorer restart, fresh-host acquisition and cancellation remain unverified. Never overwrite or relabel original evidence to expand its claims.

## Documentation and publication checks

Review each article for real configuration/defaults, failure behavior, privacy and source references, and publish the corresponding Cantonese article. Verify all relative links and every live internal article/wiki route, current images, installer download bytes, homepage field and social metadata. Run the actual tracked-source publication scanner and normal local hooks before commit; private source references and operational mappings stay outside this folder. The integrating maintainer owns default-branch integration and remote proof.
