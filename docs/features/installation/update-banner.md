# Update banner, manual check and visible updater state

This article describes how the native installed edition tells a person about an update and lets them
decide when to restart. It extends [silent staged updates](silent-updates.md), which still own
discovery, staging, hash validation and the apply protocol.

## What a person sees

| State | Shown when | Buttons |
|---|---|---|
| Ready | A newer release is downloaded, verified and staged. | **Restart to install update**, **Later**, **Release notes** |
| Waiting to restart | Restart was chosen but a Settings editor, dialog, menu or drag is still open. | **Cancel restart**, **Release notes** |
| Checking | A manual **Check for updates** is running. | **Dismiss** |
| Up to date | A manual check finished and no newer verified release exists. | **Dismiss** |
| Failed | A manual check, or a restart the person chose, failed. | **Check again**, **Dismiss** |

The ready banner is a separate top-level window. It appears without taking focus, never runs a
modal loop, stays on screen until the person chooses, and does not count as open work for the restart
check. It names the exact version, links **Release notes** to that release's GitHub page, and warns that
the update is not code-signed, so Windows may show an unknown publisher warning, and that it was
verified against the SHA-256 hashes published with the release. Text follows the English, Cantonese or
bilingual language preference. Escape and the window close button mean **Later** (or **Dismiss**),
never a restart.

## Manual check

**Check for updates** in the monitor and taskbar context menus runs a check on the background updater
thread, even when automatic checking is off. Progress and the exact result appear in the banner. If
the updater does not acknowledge the request within three seconds, the banner reports
`updater-unavailable` instead of waiting. A manual check also brings a postponed (**Later**) update back.

## Visible updater state

The Settings Monitoring page shows the service state. A failure shows its fixed stage name
(for example `package-sha256`, `package-size`, `feed-sha256`, `release-metadata`, `restart-readiness`),
the numeric error code and the HTTP status, never only a spinner or a generic message. The banner adds
the Windows description of the error code.

## Schedule and validation

Unchanged from silent updates: a check at startup and then every five minutes. The release metadata
must be a normal release from the owning repository whose source is contained in `main`; the
`RELEASES` feed and the full package are rejected on any size or SHA-256 mismatch, and the feed row
must name exactly one full package. All network and file work runs on the updater worker thread.

## Why restart is the person's choice

Earlier builds relaunched themselves at a quiet moment, which could interrupt a person who was simply
watching the taskbar monitor. Restart is now started only by **Restart to install update**. The
automatic restart preference is **off by default**; when a person turns it on it still waits for a quiet
interface and never overrides **Later**. The only automatic apply that remains is applying an already
staged update at the next normal launch, before any monitor or editor is shown, where no user work can
exist yet. Unsaved-work protection is unchanged: an open Settings editor (even with an unapplied draft),
legacy dialog, menu, drag or capture defers the restart, and the banner says it is waiting.

## Verification

* `build.bat --verify-update-banner` compiles `tests/native-update-banner.cpp` against the pure decision
  and text rules in `NativeUpdate/BannerLogic.h`, then compiles a disposable copy in which an
  automatic restart ignores its preference and requires that copy to fail.
* `build.bat --verify-updater` additionally checks that automatic restart is off by default, that an
  unchosen restart starts no helper, that same-size and truncated package and feed mutations are
  rejected at `package-sha256`, `package-size` and `feed-sha256`, and that a manual check is queued and
  visible while automatic checks are off.
* Not verified: the banner window, its buttons, focus behavior, DPI layout and the real
  restart-to-install flow have not been exercised in a running installed copy. The window code compiles
  as part of the full build only.
