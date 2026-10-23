# Documentation download binding

The release job publishes and verifies one normal release for its exact source revision. It exposes
that verified tag as a job output. Documentation builds wait for the release job, read back that
exact tag, and reject a draft, prerelease, mismatched source or missing/ambiguous download asset.
They never select an unrelated newer release during an overlapping run.

The build records the release tag, release source and SHA-256 of the actual metadata input in
`build-receipt.json`. The checked-in `docs/release-metadata.json` remains the last explicitly
verified local fallback. Hosted builds replace that input with their own verified release readback
before invoking the exact root `build.bat --site` entrypoint.

The expected delivery files are `Setup.exe`, `RELEASES`, one full `.nupkg` and `SHA256SUMS.txt`.
Download publication, installer runtime acceptance and documentation deployment remain separate
verdicts. A successful metadata readback does not prove a downloaded binary's runtime behavior.

The workflow dependency and binding are source changes until an actual hosted run and deployed
receipt establish their behavior. Local metadata currently records the verified normal 1.0.15
release at source `3f573fc0031ef6b6cb123dfbc80c05c33a5063db`.
