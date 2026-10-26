# Screenshot archive

This file keeps every screenshot reference that used to be the current presentation in the README files, with its original context and the provenance that can be determined from Git history. The README files now show a newer, categorized set of captures instead (see the Screenshots section of [README.md](../README.md#screenshots)).

None of the image files were deleted, renamed or edited; only the references moved here. The links below point at the original files in place. Dates in the Git columns are the dates of the Git commits that first added or last changed the file. They are not capture dates unless the evidence inventory says so, and an unknown capture date is recorded as unknown rather than guessed.

Archived on 2026-10-26 when the README screenshots were replaced with captures from local build 1.0.9070 (source `fe1804f8bee820d01fbf719cbb789515f1bbf00c`).

## README.md: released 1.0.48 native interface images

Previously shown under the heading "Released native interface images" in [README.md](../README.md). These are genuine, unedited native-window captures from release 1.0.48 (source `ddcf441f4c3cab2a5e3b62e9ff6aff1babf3bdfa`, [release v1.0.48-run40021045002-1](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.48-run40021045002-1)), taken in English with the light appearance at 96 DPI, and added to Git in commit `303236c` on 2026-10-23. They show the interface before the Material Design 3 refinement. The [inventory](../evidence/ui/native-release48/inventory.json) holds their hashes and capture receipts, and [released-interface-evidence.md](features/native-dashboard/released-interface-evidence.md) states their verification limits. A workspace image was excluded at the time for privacy and incomplete chart rendering.

| Original image path | Original README context | Captured (UTC, from the inventory) | State shown |
| --- | --- | --- | --- |
| [evidence/ui/native-release48/settings-interface.png](../evidence/ui/native-release48/settings-interface.png) | "Find settings and inspect notification controls" group | 2026-10-24 01:29:21 | Interface settings destination, English, light |
| [evidence/ui/native-release48/settings-notifications.png](../evidence/ui/native-release48/settings-notifications.png) | same group | 2026-10-24 01:29:25 | Notifications settings with unavailable hardware controls, English, light |
| [evidence/ui/native-release48/setup-language.png](../evidence/ui/native-release48/setup-language.png) | "Use the first-run setup workflow" group | 2026-10-24 01:28:07 | First-run language and message-style step, English, light |
| [evidence/ui/native-release48/setup-startup.png](../evidence/ui/native-release48/setup-startup.png) | same group | 2026-10-24 01:28:20 | Startup and taskbar review with both choices kept unchanged, English, light |
| [evidence/ui/native-release48/regex-test-valid.png](../evidence/ui/native-release48/regex-test-valid.png) | "Test a pattern and preview a replacement" group | 2026-10-24 01:31:10 | Regex Test view, two matches for a harmless local example, English, light |
| [evidence/ui/native-release48/regex-replace-valid.png](../evidence/ui/native-release48/regex-replace-valid.png) | same group | 2026-10-24 01:31:06 | Replace view, literal local replacement preview, English, light |

## README_en-us.md

All images below were referenced from [README_en-us.md](../README_en-us.md). The Explorer-taskbar capture was the first image after the installer introduction, and the `Screenshots/en_us` images were the upstream screenshots in the "Screenshots" section, some of them repeated in the usage text. The upstream images were committed by the upstream author (Zhong Yang) and their original capture dates are unknown; they show the upstream interface of the time, not this fork.

| Original image path | Original context | First added to Git | Last changed in Git |
| --- | --- | --- | --- |
| [evidence/installer/1.0.8/taskbar-current-desktop.png](../evidence/installer/1.0.8/taskbar-current-desktop.png) | "Installed monitor attached to the current Explorer taskbar", top of the page. It accompanied the verified Explorer attachment and hidden main window; the [receipt](../evidence/installer/1.0.8/runtime.json) records installation, update and startup evidence. | `c4373ce`, 2026-10-23 | same commit |
| [Screenshots/en_us/main1.png](../Screenshots/en_us/main1.png) | "Main Floating Window" | `424e1a5`, 2018-08-12 | `bb5248a`, 2021-10-28 (image optimization) |
| [Screenshots/en_us/main.png](../Screenshots/en_us/main.png) | "Right-Click Menu" | `424e1a5`, 2018-08-12 | `bb5248a`, 2021-10-28 |
| [Screenshots/en_us/taskbar.png](../Screenshots/en_us/taskbar.png) | "Taskbar Window" | `424e1a5`, 2018-08-12 | `bb5248a`, 2021-10-28 |
| [Screenshots/skins.PNG](../Screenshots/skins.PNG) | "Colorful Skins" | `3bf55fa`, 2018-02-10 | same commit |
| [Screenshots/en_us/selecte_skin.png](../Screenshots/en_us/selecte_skin.png) | "Change Skins" in "Screenshots", and again at the start of "Customizable Skin" | `424e1a5`, 2018-08-12 | `bb5248a`, 2021-10-28 |
| [Screenshots/en_us/option.jpg](../Screenshots/en_us/option.jpg) | "Options" in "Screenshots", and again at the start of "Configuring Options" | `7652bae`, 2020-08-11 | `bb5248a`, 2021-10-28 |
| [Screenshots/en_us/taskbar_item_settings.png](../Screenshots/en_us/taskbar_item_settings.png) | Display Settings dialog illustration in "How to Use" | `21aa945`, 2021-07-25 | `f7147ea`, 2025-01-21 |

## README_zh-CN.md

All images below were referenced from [README_zh-CN.md](../README_zh-CN.md), which kept the Chinese-language upstream screenshots. The first image was the same Explorer-taskbar capture as in the English README. Capture dates of the upstream images are unknown.

| Original image path | Original context | First added to Git | Last changed in Git |
| --- | --- | --- | --- |
| [evidence/installer/1.0.8/taskbar-current-desktop.png](../evidence/installer/1.0.8/taskbar-current-desktop.png) | Explorer-taskbar capture, top of the page | `c4373ce`, 2026-10-23 | same commit |
| [Screenshots/main1.png](../Screenshots/main1.png) | "主悬浮窗" (main floating window) | `3bf55fa`, 2018-02-10 | `bb5248a`, 2021-10-28 |
| [Screenshots/main.png](../Screenshots/main.png) | "右键菜单" (right-click menu) | `3bf55fa`, 2018-02-10 | `bb5248a`, 2021-10-28 |
| [Screenshots/taskbar.PNG](../Screenshots/taskbar.PNG) | "任务栏窗口" (taskbar window) | `3bf55fa`, 2018-02-10 | `89f774a`, 2018-03-20 |
| [Screenshots/skins.PNG](../Screenshots/skins.PNG) | "多彩皮肤" (colorful skins) | `3bf55fa`, 2018-02-10 | same commit |
| [Screenshots/taskbar_item_settings.png](../Screenshots/taskbar_item_settings.png) | Display settings illustration in the usage text | `21aa945`, 2021-07-25 | `f7147ea`, 2025-01-21 |
| [Screenshots/selecte_skin.png](../Screenshots/selecte_skin.png) | "自定义皮肤" (custom skins) | `3bf55fa`, 2018-02-10 | `bb5248a`, 2021-10-28 |
| [Screenshots/option.jpg](../Screenshots/option.jpg) | "选项设置" (options) | `7652bae`, 2020-08-11 | `bb5248a`, 2021-10-28 |

## Related archives

- [Original README before the offline companion](history/README-before-offline-companion.md)
- [Documentation-reader image archive](history/documentation-companion-9024-images.md)
