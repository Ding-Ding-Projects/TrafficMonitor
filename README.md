# TrafficMonitor

A native C++/MFC network monitor for Windows, with a compact taskbar display, tray access and a floating monitor. This fork delivers the maintained x64 edition, preserves upstream attribution and supports the existing optional plugin interfaces.

[Documentation website](https://ding-ding-projects.github.io/TrafficMonitor/) · [Verified 1.0.36 release](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.36-run39992941450-1) · [Installation](docs/features/installation/README.md) · [Changelog](CHANGELOG.md) · [Required-feature status](docs/features/required-features/README.md) · [简体中文](README_zh-CN.md)

## Install and start monitoring

Use the genuine unsigned Squirrel.Windows `Setup.exe` from the verified release. The x64 package includes its matching CRT/MFC runtime files. Fresh installed settings enable startup at the current user's interactive sign-in, show the taskbar display and hide the main window. Existing saved choices, traffic history, skins and plugins take precedence during updates. Tray actions remain available for settings, the main window, startup and exit.

Startup and shortcuts use the stable installation launcher. Uninstall removes only owned integration and preserves saved data by default. Optional hardware plugins are not silently installed; see [hardware support and acquisition](docs/features/installation/README.md). An unsigned installer can show an unknown-publisher warning. Current-host verification does not establish fresh-host acquisition, cancellation or a subsequent real sign-in.

## Silent updates and verified boundaries

The installed monitor checks on startup and every five minutes, validates the accepted release and package, and stages updates silently. It cooperatively restarts at a safe lifecycle boundary, preserving busy-work checks and the next-launch fallback. No Windows restart or sign-out is required or initiated.

Normal release **1.0.36** is bound to `f795e86b309cb72f80e030fe7c802ea9c44b4fd2`; all four release jobs succeeded. One genuine **1.0.34 to 1.0.35** transition used a real prepublication baseline and verified exact released bytes, readiness/launcher results, one apply attempt and configuration/startup/history preservation. A subsequent automatic **1.0.35 to 1.0.36** transition used an explicitly post-publication preservation snapshot, with the earlier verified terminal state retained separately. These are two bounded observations, not long-term reliability or production-negative-path proof.

See [natural35 evidence](docs/features/installation/silent-natural35-acceptance.json), [natural36 evidence](docs/features/installation/silent-natural36-acceptance.json), and [update behavior and limits](docs/features/installation/silent-updates.md).

## Native interface work

The native monitoring backend, taskbar integration, tray and plugin interfaces remain. The new network workspace, task-oriented settings and resumable four-step setup are useful partial implementations. Setup opens through the explicit workspace action, with save/resume and acknowledged partial operations; ordinary sign-in monitoring remains unobtrusive. Setup language/style controls currently apply to the wizard only.

Bounded setup checks cover light theme at actual 96 DPI, three languages, save/resume, Keep-current completion and selected accessibility consumers. Choice expansion, complete keyboard/accessibility behavior, the full theme/scale matrix, full regex-builder parity and the broader required-feature inventory remain unfinished. The indexed 110-item ledger is an obligation inventory, not proof that 110 features shipped.

Read [workspace](docs/features/native-dashboard/README.md), [setup](docs/features/native-dashboard/setup.md), [settings](docs/features/native-settings/README.md), [search](docs/features/search/README.md), [roadmap](ROADMAP.md) and [handoff](HANDOFF.md).

## Offline documentation companion

The additional single-file documentation reader supplements the hosted website. Build it through `build.bat --docs-companion -Version <version>` or `build-installer.bat --docs-companion -Version <version>`. No adjacent runtime folder or separate installer is needed to launch the portable executable.

Local candidate **1.0.9024**, production source `421ac841a7825b682f0a67928be117c6b21c6ea4`, passed exact-entrypoint packaging, 40 local documentation checks and bounded real one-file interaction. Snapshot labels distinguish bundled release information from the installed monitor or companion package version; external source/download links remain local and visibly explained. A three-language, light/dark, normal/minimum home matrix passed at 96 DPI. Higher scales, full keyboard/accessibility and complete feature acceptance remain pending. This candidate receipt is not proof of a later integration build or a published companion asset.

See [companion build, runtime and release boundaries](docs/features/build/offline-documentation-companion.md). The integrated workflow attaches companion outputs to the same normal native release and fails clearly if that delivery is incomplete.

## Build

Run root `build.bat` for the maintained x64 native build and `build-installer.bat -Version <three-part-version>` for genuine Squirrel packaging. The entrypoints detect supported missing tools and obtain trusted pinned installers or verified portable packages, preserving existing installations and requiring the user's native consent when elevation is necessary. An unavailable licence, cancelled consent or unavoidable restart is an explicit incomplete result.

Use `build.bat --site` for the complete article/wiki website. Article production derives its inventory and immutable content from the held source, rather than silently reusing a historical manifest. See [build instructions](docs/build/README.md) and [build feature documentation](docs/features/build/README.md).

## Current and historical images

Current rebuilt-native-interface image promotion remains pending complete timestamp/source receipts. Earlier taskbar and upstream interface images are historical and are preserved with their provenance in the [README and image archive](docs/history/README-before-offline-companion.md). No historical image is relabeled as evidence for a newer build. Reviewed documentation-companion images remain pending promotion against their exact producing source. Their raw bytes and private receipts are retained; no later source is substituted to make validation pass.

## Upstream and licence

TrafficMonitor originates from [zhongyang219/TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor). This fork retains the [original licence](LICENSE) and upstream attribution. The complete earlier introduction, usage, skins, plugins, related links and image presentation remain available in the [historical README archive](docs/history/README-before-offline-companion.md); current operational guidance is organized in the documentation linked above.
