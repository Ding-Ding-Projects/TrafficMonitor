# Silent installed updates

Installed x64 builds check the owning `Ding-Ding-Projects/TrafficMonitor` normal
release channel at startup and every five minutes. Checks and full-package downloads
run on one bounded background worker. Portable builds keep their existing behavior.
The saved automatic-check preference remains the opt-out.

No automatic update dialog, browser, progress window, notification or elevation prompt
is opened. A separate automatic-restart preference defaults to enabled. The UI may
request a cooperative restart only when no ordinary workspace, settings editor,
modal interaction or unsaved draft is active. Otherwise the staged package waits.
Disabling automatic restart keeps checking and staging; the next normal launch can
apply the package before monitoring starts. Disabling automatic checks prevents
both automatic staging and next-launch application.

## Integrity and installation boundaries

Only normal `v<major>.<minor>.<patch>-run<run-id>-<attempt>` releases from the exact
owning repository are admitted. Release metadata must identify a full source commit,
and an owning-repository comparison must prove that commit is contained in an exact
resolved `main` commit. The comparison URL, base and merge-base identities must match.
The bounded branch and comparison responses are retained with staging for offline revalidation;
a normal task-branch preservation release outside `main` is not an update candidate.
Release metadata must also identify
the expected full package, exact download URLs, uploaded assets, bounded lengths and
SHA-256 digests. The full package and `RELEASES` are checked against those digests;
the single-entry feed is additionally checked for its exact package basename,
length and Squirrel SHA-1. TLS certificate verification remains enabled. Redirects
are bounded and restricted to the publisher's standard HTTPS asset hosts. No
credentials or redirect URLs are logged. These unsigned packages have integrity
checks, not a signing or publisher-authenticity guarantee.

Staging lives below the installation root in `native-update`, outside `app-*`
directories. Completed downloads are atomically renamed to `pending`. The helper
revalidates their bytes before invoking the installation's stable
`Update.exe --update <local-feed> --silent`, then uses
`Update.exe --processStart TrafficMonitor.exe`. The supported options were checked
against Squirrel.Windows tag `2.0.1`, `src/Update/StartupOption.cs` and
`src/Update/Program.cs`. Squirrel lifecycle callbacks are handled first and do not
start the network worker.

The helper inherits only a handle to the exact originating process, verifies its
PID, creation time and executable, and waits for its actual graceful exit. It never
terminates that process or a child, restarts Explorer, or changes host power/login
state. Application-local runtime files remain available in the old version while
the helper runs. A named installation lock and the regular instance mutex prevent
concurrent applications. Each immutable version receives at most two apply attempts.
Failure retains the prior installation and uses the stable launcher for recovery;
an updater that has not exited after three minutes is reported as timed out and is
not overlapped by another launch. Configuration, history, startup, skins and plugin
settings are not edited by this backend.

## Status and remaining verification

The native API exposes disabled, idle, checking, downloading, ready, applying,
failed and stopped states, version metadata and a numeric result. Saved restart
preferences return failure when persistence or readback fails. Network work has a
two-minute operation deadline, five-second request timeouts and cancellation signals;
the UI never joins its worker.

The current expedited delivery allows compile and packaging only. No new automated
tests, reviews, runtime checks or screenshots have been performed for this updater.
Offline recovery, cancellation, corrupt metadata/packages, downgrade rejection,
modal/draft deferral, restart timing, timeout recovery and preservation remain runtime
acceptance work.

## Published and installed baseline

Normal release [v1.0.19-run39942930510-1](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.19-run39942930510-1)
is bound to source `77435775e46ca78de13859a9e4f2a15732e53415`. Its four build,
package, publication and documentation jobs completed successfully. The separately
assigned installed-update operation applied version `1.0.19` through the supported
stable Squirrel updater and launcher, both exit `0`. Its receipt reports exact
post-save configuration/history and startup preservation, main window hidden and
taskbar monitor visible from metadata only. The installed executable SHA-256 is
`0438e33baa14c302640326a296bcf2d078a1043356eebc3b57aea98b11f497b1`.

That supported installed update is separate from automatic scheduling: it does not
prove the five-minute cadence, automatic cooperative restart, modal/draft deferral,
next-launch application or failure recovery. No new runtime checks or screenshots
were performed for this documentation refresh. The native MFC interface conversion
and all 110 applicable required features remain the broader unfinished goal.

Read the [documentation home](https://ding-ding-projects.github.io/TrafficMonitor/)
for the complete guide library and the [updater wiki guide](https://github.com/Ding-Ding-Projects/TrafficMonitor/wiki/Silent-updates)
for the matching wiki article. Wiki publication of the refreshed guide is prepared
separately and must be verified before it is reported delivered.

## Feed spelling repair and local diagnostics

Squirrel writes SHA-1 in uppercase in its RELEASES feed. The reader now accepts exactly
forty ASCII hexadecimal characters in either case, normalizes their spelling, and still
requires the exact package digest, byte length, basename and single full-package row.
The SHA-256 checks and strict accepted-main admission remain unchanged.

Installed builds retain best-effort local `native-update/status.ini` fields: `stage` is a
fixed updater operation name, `phase` is the native phase number (0 disabled, 1 idle,
2 checking, 3 downloading, 4 ready, 5 applying, 6 failed, 7 stopped), `error` is the
numeric result, and `http_status` records an unsuccessful HTTP response status when
available, otherwise zero. A transport failure retains its WinHTTP result. Validation
failures retain error 13. These are the last published background-check/startup values,
not a heartbeat or a promise that the updater is still running. Preference toggles and
helper application can occur later; helper outcomes remain in the version attempt file.
No response body, URL, credentials, arbitrary exception message or user content is saved.
If the diagnostic destination is unavailable, native in-memory status remains independent
and the updater does not relax its checks. Managed startup rejection is failed rather
than silently appearing disabled; portable builds retain their existing behavior.

The exact root `build-installer.bat -Version 1.0.9007` at source
`f63e8a456befcd2f6d726712182f6e2a01e9cc11` exited 0. Its root native build
reported two LNK4199 warnings, zero errors and elapsed 15.28 seconds. The
[build receipt](silent-update-repair-build.json) binds the local executable and package.
This is local production evidence only, not publication, installation or automatic-update
acceptance. No tests, reviews, audits, runtime checks or screenshots ran. Full Stage 2,
automatic cadence, restart deferral and recovery remain unfinished.

## Passive status tooltip restart boundary

The existing visible-window restart check also classified the application-owned passive status tooltip as busy. Existing installed metadata records a visible taskbar tooltip, which demonstrates this classification mismatch but does not establish the cause of every previous update delay. The caller now excludes only the exact main/taskbar tooltip handles when their owner and tooltips_class32 class both match. Ordinary workspace, modeless editor, menu, draft, mouse capture and move/size deferrals remain unchanged. No new runtime observations, tests, reviews or screenshots were performed; automatic restart remains unverified.

## UTF-8 feed marker repair

The public 1.0.23 RELEASES file begins with bytes EF BB BF before its valid
forty-character SHA-1 field. The native reader previously included that UTF-8 marker
in the field and rejected its length. The repair verifies the original feed length
and SHA-256 first, then skips only one leading UTF-8 marker for row parsing. Package
bytes, both package digests, exact basename, single-entry checks and accepted-main
admission are unchanged. It accepts neither arbitrary prefixes nor a marker inside
a field. Static package inspection confirmed the public package SHA-1 matches the
feed; this is format evidence, not execution of the updater or automatic-update proof.

Integrity rejection now retains one fixed local substage: feed-size, feed-sha256,
package-size, package-sha256, feed-format, feed-entry-count or package-sha1. The
numeric validation result remains 13. No observed or expected hashes, payload bytes,
URLs, response text, user content or credentials enter this diagnostic file.

Production evidence for this repair is recorded separately. No tests, runtime probes,
reviews, audits, interface interaction or screenshots are run in the expedited mode.
Automatic scheduling, cooperative restart, recovery and full Stage 2 remain unfinished.

The exact root build-installer.bat -Version 1.0.9012 at 6464841107c5b2bc43f15040c3108057ed296caf exited 0. Native build elapsed 10.20 seconds, with 22 warnings and zero errors. The source-bound local receipt is docs/features/installation/silent-feed-marker-build.json. This is compile/package evidence only; automatic scheduling, safe restart, recovery and full Stage 2 remain unverified.

## Published feed encoding

The supported package writer now explicitly writes RELEASES with strict BOM-free
UTF-8 before calculating SHA256SUMS and publication metadata. It preserves Squirrel
row contents, line endings, SHA-1 fields and package bytes, then reads back the
stored feed to require exact text and absence of the leading marker. Parser support
for prior valid BOM-prefixed feeds remains. The local 1.0.9012 receipt precedes this
writer correction; its bytes remain historical. Production 1.0.9013 completed as recorded below.

Exact build-installer.bat -Version 1.0.9013 at 58ff5b8226a9da38307a1b3ec3956e0a4ed35119 exited 0, with two LNK4199 warnings, zero errors and native elapsed 3.84 seconds. The supported writer completed its strict BOM-free UTF-8 readback before checksums. Source-bound receipt: docs/features/installation/silent-feed-writer-build.json. This is local production only, not automatic staging, restart, release or installed-update proof.

## Preserved stale staging and helper handoff diagnostics

A manually completed supported update can leave its already-installed package in
`native-update/pending`. Previously the newer-version requirement rejected that
valid package before the worker could discover subsequent releases. The worker now
revalidates all retained metadata, accepted-main admission, feed and package hashes
first. When its version is equal to or older than the current package, it atomically
retains that directory under a unique `retained-<version>-<tick>` name and continues
normal discovery. Only this retirement path accepts a non-newer version; helper
application still requires a strictly newer release. Nothing is deleted or downgraded.

`native-update/helper-status.ini` records separate `launch` and `helper` sections
with fixed `stage` and numeric `result` fields. Launch failures retain their Windows
result before handle cleanup, including attribute/preparation and process creation.
The helper records entry, parent identity, parent exit, pending validation, Squirrel
application and stable-launcher stages. Its final numeric result preserves the last
operation stage, including early rejection and exceptions. `259` means the last
recorded operation was in progress, not a heartbeat. Process creation success is not
proof of helper entry, successful application or restart. These best-effort records
contain no URLs, payload data, credentials or arbitrary exception text.

Existing installed evidence showed a correctly hashed staged package remaining Ready
for ten minutes with attempt count 1 and no process transition. That record does not
distinguish launch failure, loader failure or UI deferral. The prior implementation
discarded process creation errors and early helper outcomes, so the exact original
handoff cause remains unverified. This correction fixes stale-package discovery and
retains missing diagnostics; it does not claim automatic-restart acceptance.
No tests, reviews, audits, runtime quality checks, interface interaction or screenshots
were performed in this expedited correction.
