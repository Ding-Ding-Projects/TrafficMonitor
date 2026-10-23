# TrafficMonitor implementation handoff

## Objective

Deliver the installer and release foundation first. Native MFC interface conversion and the
complete applicable required-feature inventory follow only after the first delivery.

## Verified baseline

The public fork is `Ding-Ding-Projects/TrafficMonitor`, with `main` as its default branch.
The starting source is `930f17533d6098989ebad62f210aa97d75ef174b` from
`zhongyang219/TrafficMonitor`. Upstream license files and attribution remain intact.

## Active implementation

Separate isolated branches own native installer lifecycle handling and build/package/release
automation. Their changes are not yet integrated or verified. The selected runtime is the
maintained x64 edition with optional hardware-plugin support. Existing preferences must survive
updates; fresh installed defaults enable login startup and taskbar display while hiding the main window.

## Verification limits

No customized installer, local runtime, update, or release has been verified yet.
Organization runner discovery returned HTTP 403, so the pipeline uses a hosted Windows image.
No host restart or sign-out is authorized for verification.
