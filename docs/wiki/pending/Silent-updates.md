# Keep TrafficMonitor updated quietly

Use the installed edition's native Monitoring settings to keep automatic checks enabled
and choose whether it may restart automatically. The check preference controls automatic
checking and next-launch application. The separate restart preference defaults on;
turning it off keeps quiet staging and leaves installation for the next normal launch.
A failed preference save retains the draft and reports the failure.

1. Open the native settings editor and choose Monitoring.
2. Keep automatic checks enabled to check at startup and every five minutes.
3. Choose the automatic-restart setting, then Apply. Revert reloads saved choices;
   resetting Monitoring stages the enabled defaults for a later Apply.
4. Continue working. The updater stages a verified full package without opening a
   browser, update dialog, progress window, toast or elevation prompt.
5. Finish any active settings, editor, modal or ordinary workspace work. Automatic
   restart may proceed only through a cooperative save/exit when those surfaces are safe.
   Otherwise the package waits; the next normal launch can apply it before monitoring.

The UI's status reports disabled, idle, checking, downloading, ready, applying, failed
or stopped. A ready package has not necessarily been applied. A failed child process is
not reported as a successful installation. Explicit automatic-check opt-out is preserved.
Portable builds keep their original behavior.
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
for this article. Publication records distinguish prepared content from verified publication.

## Related tasks

- [[Install or update|Installation]]
- [[Use the taskbar and tray|Monitoring]]
- [[Verified behavior and remaining work|Verification]]
- [[Wiki home|Home]]
- [Complete documentation home](https://ding-ding-projects.github.io/TrafficMonitor/)
