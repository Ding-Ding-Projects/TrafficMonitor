# Silent installed updates

## Current updater outcome

| Evidence boundary | Current verified state |
| --- | --- |
| Product source/build | Synchronous WM_CLOSE refinement `72f1fc929fe5d3a31b7a6e1ee73de20bb61a6ec3`; exact root local package 1.0.9017 exited 0 (8.79 seconds, 69 warnings, zero errors). Earlier queued 1.0.9016 and source receipts remain historical. |
| Normal release | [v1.0.29-run39970237810-1](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.29-run39970237810-1), exact source `50288f54a16fb2b0e92464127bd7de8481567a54`, normal flags false/false, published 2026-10-23T14:35:38Z; all four jobs succeeded. |
| Installed automatic update | Corrected client 1.0.28 naturally updated to package 1.0.29 without a triggered check, manual close/update or preference change. The old process exited; one new process had the exact released executable SHA-256 `129f47fb6e05d398113b0034bf5f484317d8b58094ef35e16b1b146dfdb44bba` and manifest source 50288f54. Main hidden/taskbar visible readiness was verified by metadata. Embedded executable versions remain 1.8.6.0. |
| Handoff result | Attempt 1; Squirrel result 0; helper launch created/result 0; final helper stage stable-launcher/result 0. Helper file modification was 14:38:08.403841Z, first new-process snapshot 14:38:30.691627Z, before the 14:45:38Z deadline. Applied staging is retained and active pending is absent. The inherited old result 1460 was replaced by the real successful result. |
| Preservation | Configuration bytes and startup registration match the prepublication baseline. Historical traffic records are unchanged, the current date record remains and its counters are nondecreasing. Monitoring/graceful saves legitimately advanced current traffic, which is not data loss. |
| Publication | Parent HTTP readback verified home 200, real release 1.0.29 installer link, deployed source/release source 50288f54, 196 complete articles and zero unavailable assets. Deployment completed 2026-10-23T14:37:01Z. Stored About is exactly `https://ding-ding-projects.github.io/TrafficMonitor/`. Availability proof only. |
| Published wiki | Master `27e2231ae4f8986ee394075dc46fdf9838c7e77c`, exact prepared source `ebf95b99a4bd95c2e741e9099eaa5d2f2538d67b`, guide SHA-256 `ebdd88f968f884a853c5b72e6dd6e2994da5db7ca19e65221ce95072e97572b1`. Stored source proven; live wiki interaction unverified. This newer acceptance record has not yet been published to the wiki. |
| Remaining acceptance | One real automatic transition is proven. Offline/corrupt input/cancellation/rollback, busy editor/modal/draft/menu deferral, visual/localization/accessibility and repeated future cadence remain unrun. No broad Stage 2 or required-feature completion claim is made. |

The bounded natural-update receipt is `docs/features/installation/silent-natural29-acceptance.json`. Source, local production, normal release, stored publication and this one natural update are distinct proof. No tests, audits, reviews, UI quality checks or screenshots ran.

## Historical implementation and production records

The records below retain their original sources, receipts and limits. Earlier versions, queued-close proposals, pending-publication statements and initial observation gaps describe those earlier points in time; the current table above governs the current state.

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

The initial implementation pass allowed compile and packaging only. Later assigned
normal installed-update observations establish the narrow current-table facts; they
do not constitute general runtime acceptance. No new tests, reviews, UI quality checks
or screenshots were performed.
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
for the matching wiki article. That earlier prepared snapshot was subsequently published with exact stored-source
proof, most recently at wiki master 27e2231ae4f8986ee394075dc46fdf9838c7e77c.
Live wiki interaction remains unverified.

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

Local production `build-installer.bat -Version 1.0.9015` at `7fffeb22b26e7f0e6d0cec247f19fb895957213f` completed with exit 0, native elapsed 10.76 seconds, two LNK4199 warnings and zero errors. The [production receipt](silent-handoff-build.json) binds the executable and package. This does not prove helper loading, automatic restart or recovery.

## Verified manual package 1.0.25 baseline

Normal release [v1.0.25-run39959454885-1](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.25-run39959454885-1) is bound to source `693ae4d349dcd5b993b826d05e6c0fd9a8eae400`. The assigned supported manual update completed with Squirrel update and stable launcher exit 0. Its installed executable SHA-256 is `2994f92bfc3b6c224f8d6e7f64948293ca9e9e0505d68cbf65dd989664d9d685`, matching the released package executable. Settings, traffic history and startup registration matched the post-graceful-save baseline, and all four roaming files remained unchanged after launch. Embedded executable versions remain 1.8.6.0, separate from package version 1.0.25.

This is the verified manual baseline, not a claim that 1.0.25 is the newest release. The unchanged obsolete pending-package state was retained before fallback; no repeated observation, preference change or check trigger occurred. Automatic cadence, helper loading, cooperative restart and recovery remain unverified. Native production 1.0.9015 remains bound to source `7fffeb22b26e7f0e6d0cec247f19fb895957213f`; no native rebuild is part of this documentation followup.

The updater wiki guide was separately published at wiki master `0ab91b5d0ba50584dce8d75d2a2cb7c0a0d2a6dd`. Its post-fetch raw blob matches prepared source `99708ed18dc61c996113ed027a3f5d2dea370e4e`, SHA-256 `411488cb8820ab87fead547dcf40e39202a090c754ce6cab368e4768b292b718`. The other six guides and prior publication proof are retained. This proves stored content, not live wiki interaction.

## Verified manual package 1.0.26 and natural staging retirement

Normal release [v1.0.26-run39960973956-1](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.26-run39960973956-1) is bound to source `0485c5bbdd04469153c9bf1a062df57b15f3c0aa`. The assigned supported manual Squirrel update and stable launcher both exited 0. Installed package version is 1.0.26 and its executable SHA-256 `7ce698aed64d500b61dce81f3f44bf753337079f990991384988bbc582f912f2` matches the released package executable. Embedded executable versions remain 1.8.6.0. Settings, traffic history and startup registration matched the post-graceful-save baseline; all four roaming files remained unchanged after launch. No automatic check was triggered or preference changed.

The existing obsolete pending 1.0.24 state was preserved before manual fallback. After normal 1.0.26 startup, metadata showed a retained 1.0.24 staging directory and no active pending directory. All five retained metadata/feed/package files were byte-identical to the preserved pre-update pending files. The naturally published background state was `stage=release-metadata`, `phase=1` (Idle), `error=0`, `http_status=0`. No helper-status file was present. This establishes natural retirement of the validated obsolete staging during normal startup, not automatic package application, five-minute cadence, helper loading, cooperative restart or recovery acceptance. Those remain unresolved.

The exact wiki stored-source proof remains master `0ab91b5d0ba50584dce8d75d2a2cb7c0a0d2a6dd`, prepared source `99708ed18dc61c996113ed027a3f5d2dea370e4e`, SHA-256 `411488cb8820ab87fead547dcf40e39202a090c754ce6cab368e4768b292b718`; the published guide is unchanged. Native production 1.0.9015 remains bound to `7fffeb22b26e7f0e6d0cec247f19fb895957213f`. This followup changes documentation only.

## Historical queued close-message candidate (1.0.9016)

The accepted release 1.0.27 was published at 2026-10-23T13:44:29Z from source `e0debba58fc20da1c40a48d6e8df3983feca3d9d`. The assigned natural-update observation retained metadata from before publication through the first 30-second snapshot after its ten-minute deadline. It observed a correctly staged package, attempt count 1, successful helper creation (launch result 0), and helper stage `parent-exit` ending with result 1460 while the exact original process remained alive. It did not observe automatic application. The subsequent authorized graceful manual Squirrel update and stable launcher each exited 0 and applied package 1.0.27; released and installed executable SHA-256 matched `fa7b1537475cc845821228ab3cd4bade7574ecae3c8ebcbff3d8fce92af6f83e`. Settings, history and startup were preserved against the legitimate post-save baseline. Traffic monitoring and graceful close may update history before that baseline, which is not data loss.

Source correlation establishes the caller mismatch: `OnSilentUpdatePulse`, dispatched by `WM_APP + 0x45A`, called `OnClose()` directly. The save handler then called inherited `CDialog::OnClose`. The matching MFC implementation inherits `CWnd::OnClose`, which calls `Default()`, forwarding the last-dispatched message. Under the pulse that was the custom message, not `WM_CLOSE`, so default close processing was not requested. The caller now queues a real `WM_CLOSE` after the unchanged cooperative safety checks. Closing state is set only when posting succeeds. Failure records fixed `close-queue` stage and its numeric result, reports failure, and never forces closure or starts a competing helper. Existing saves, modal lifetime, identity-bound parent wait, timeout, integrity and attempt bounds remain unchanged.

This correction is source and production evidence, not post-correction automatic-restart acceptance. No tests, reviews, audits, interface interaction or screenshots ran. A later real accepted update must establish the repaired transition. Historical 1.0.9015 production remains bound to `7fffeb22b26e7f0e6d0cec247f19fb895957213f`.

Exact `build-installer.bat -Version 1.0.9016`, including root `build.bat`, at `46b97789b07920748866b7ed63d83aabfa721075` completed with exit 0, native elapsed 9.84 seconds, 69 warnings and zero errors. Receipt: `docs/features/installation/silent-close-message-build.json`. This is compile/package evidence only.

## Current synchronous automatic close dispatch

The unpublished queued-close candidate and its 1.0.9016 receipt above remain historical. The final caller now sets closing state before `SendMessageW(window, WM_CLOSE, 0, 0)` on the same already-checked UI thread and returns immediately without touching the potentially destroyed window. This gives normal cooperative save/close handling the real message context without allowing queued input between its safety decision and close dispatch.

The matching MFC `AfxCallWndProc` implementation saves `m_lastSentMsg` for nesting, assigns the nested message, invokes the window procedure, and restores the prior state on return. Therefore inherited default close handling sees the nested `WM_CLOSE`, rather than the outer update pulse. Existing draft/menu/window checks, saves, modal lifetime and helper identity/timeout/attempt bounds are unchanged. No termination is forced. The unused unreleased queue-failure API was removed because this path no longer queues a message. This source correlation is not runtime acceptance.

Exact `build-installer.bat -Version 1.0.9017`, including root `build.bat`, at `72f1fc929fe5d3a31b7a6e1ee73de20bb61a6ec3` exited 0, native elapsed 8.79 seconds, 69 warnings and zero errors. Its source-bound receipt is `docs/features/installation/silent-synchronous-close-build.json`. Native 1.0.9015 and 1.0.9016 sources and receipts remain preserved. No tests, reviews, audits, UI quality work, interface interaction or screenshots were performed. A later real accepted update must still prove automatic handoff and relaunch after this refinement.
