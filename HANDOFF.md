# TrafficMonitor implementation handoff

## Objective

Deliver the installer and release foundation first. Native MFC interface conversion and the
complete applicable required-feature inventory follow only after the first delivery.

## Verified baseline

The public fork is `Ding-Ding-Projects/TrafficMonitor`, with `main` as its default branch.
The starting source is `930f17533d6098989ebad62f210aa97d75ef174b` from
`zhongyang219/TrafficMonitor`. Upstream license files and attribution remain intact.

## Delivered installer foundation

Native lifecycle, build and packaging work is integrated on `main`. Release
[`v1.0.3-run39897902320-1`](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.3-run39897902320-1)
is bound to `b54506269e63e620bb97d9c3e471cd0994564f1d` and its workflow completed successfully.
The genuine unsigned installer includes matching application-local CRT/MFC DLLs. Actual installed
launch loaded those DLLs from the application directory. The fixed English version-resource key
passed the pinned Squirrel detector with a 2932-byte resource, and the real installer invoked its
callback and registered the stable current-user startup command. That exact command launched the
installed executable on the hidden desktop. The main window was hidden and the taskbar monitor
rendered live network readings. See [the sanitized receipt](evidence/installer/1.0.3/runtime.json).

An earlier 1.0.2 package installed but did not invoke its callback due to a language-specific
version-resource mismatch. It is superseded by 1.0.3. Its task-created installation was removed
through the supported uninstaller before installing 1.0.3; saved application data survived.

## Verification limits

Organization runner discovery returned HTTP 403, so the pipeline uses a hosted Windows image.
No host restart or sign-out is authorized for verification.
Release 1.0.4 subsequently passed the supported real update from 1.0.3: configuration bytes were
unchanged, and a disabled startup choice remained disabled. The requested enabled state was restored.
Its supported uninstall removed owned startup registration and executable files while preserving
saved data. Reinstallation restored startup. The stable launcher then started 1.0.4 on the current
desktop with the main window hidden and the monitor attached beneath Explorer's `Shell_TrayWnd`.
See [current desktop evidence](evidence/installer/1.0.4/runtime.json).

Subsequent sign-in, real Explorer restart, fresh-host prerequisite acquisition and cancellation are
not claimed. No host restart, sign-out or Explorer restart was performed.
Full-tree local publication checks now pass with exact inherited literal exceptions and reviewed
legacy decoding. A staged-content negative probe was rejected. The hosted publication job passed
before its dependent packaging job. The strict `publication-language` required check is configured
on `main`; administrator bypass remains enabled and must not be described as absolute protection.
The native interface design is under independent review. The complete 110-item applicability
inventory is recorded under `docs/features/required-features/`; implementation remains unfinished.

Managed installation update checks now avoid the upstream portable archive feed. A manual check
opens this fork's release page; background checks return without contacting the upstream feed.
This is update-source containment, not an implemented automatic Squirrel update interface.
Compilation and installed interaction of this change remain pending its next build.
