# Keep cancelled update preparation retryable

Update preparation is separate from applying a package. A cancelled preparation,
expired ten-second permission window or failed helper creation does not consume
either of the two durable apply attempts. Failed preparation remains Ready with
its numeric diagnostic and a ten-second retry delay; active monitoring continues.
Only the apply transaction, after actual parent exit and final staging validation,
reserves an attempt immediately before the bounded updater call. Two failed apply
transactions still prevent a third call for that immutable version.

The background helper now validates the complete pending release, accepted-main
admission, feed/package sizes, SHA-256, feed row and package SHA-1 before signaling
readiness. Damaged staging therefore cannot authorize the original process to exit,
including during early launch. Identity handles, cancellation checks and the safe
UI decision remain required. Validation repeats after parent exit and must identify
the same version/source/feed/package, protecting against a changed staging directory.
Diagnostics contain fixed operation names and numeric results, never sensitive paths.

## Focused local checks

Run `build.bat --verify-updater` on the supported Windows/MSVC host. The command
builds the real updater implementation and transport into an isolated console fixture,
then exercises inherited helper handles, repeated cooperative cancellation, two real
ten-second permission expiries, helper creation failure, corrupted feed/package
readiness rejection and a durable two-apply callback limit. It never updates the
installed client. The fixture's Update.exe is deliberately non-executable.

`build.bat --verify-updater --baseline` checks the preserved pre-repair source
2098090699c6029c47fb1e432742b7cc51834350 and is expected to exit nonzero: cancellation
charges attempts and corrupted staging receives readiness. Both baseline and fixed
checks are local only; no tests or lint are added to the release workflow.

These local checks establish focused preparation and budget behavior. A separate
[genuine natural 1.0.34 to 1.0.35 acceptance](silent-natural35-acceptance.json) now verifies
accepted client 1.0.34 with this repair automatically applying integrated 1.0.35, exact released
bytes and saved-data/startup preservation. The observation made no manual check, close,
apply or updater-state change. One successful transition does not establish production
negative paths, full GUI/installer acceptance, offline recovery or repeated reliability.
Broader release-grade closeout remains unfinished and separately coordinated.
