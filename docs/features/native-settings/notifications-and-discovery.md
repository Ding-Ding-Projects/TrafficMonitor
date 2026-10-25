# Find settings and configure notifications

Open Settings from the monitoring workspace or an existing monitor menu. Monitoring, Floating monitor, Taskbar and Notifications each retain their own search query and regex options while the editor stays open. Search matches names, explanations and formatted draft values. **All groups** discovers settings in every native group; choose a result and **Open selected setting** to navigate. The destination's previous filter remains intact, with the selected setting temporarily revealed. **Return to group filter** removes that temporary reveal.

## Notification drafts

Daily traffic notifications accept 1 to 32767 MB or GB. Memory notifications accept 1 to 100 percent. Temperature notifications accept 1 to 120 Celsius, using existing hardware-monitoring availability. The build without temperature monitoring displays those controls as unavailable and retains their stored values unchanged. It never enables hardware monitoring automatically.

Turning a notification off retains its threshold and unit. Available thresholds remain editable while the notification is off. Defaults are explicit: notifications off, daily traffic 200 MB, memory and temperature thresholds 80. Applying these settings uses the existing threshold-crossing consumer and its notification interval; changing a threshold does not itself send a test notification.

All numeric drafts and units are checked before either ordinary settings application or automatic-restart preference persistence. Empty input, signs, whitespace, trailing characters, overflow, out-of-range values and invalid interval steps are rejected. The editor reveals the offending setting and retains other drafts and group filters. An automatic-restart preference save/readback failure retains the draft and prevents ordinary settings application. The legacy apply callback is not a transactional persistence receipt; runtime configuration readback is a separate acceptance step.

## Row actions and navigation

Notifications use six measured logical groups. Each has its enable checkbox and Details action,
then a separate labelled threshold and unit. Daily traffic retains its real MB/GB selector;
immutable units are text. Unavailable temperature values stay disabled and unchanged, while
their full-contrast explanation and Details action remain available. The scroll body grows
with bilingual text rather than squeezing all six groups into one screen. Apply, Revert and
Reset group remain in the fixed footer.

Right-click a setting, press Shift+F10/Menu while its control is focused, or choose its Details button. The shared searchable menu can show its explanation, stage that setting's default, or reload that setting from the currently applied consumer. Resets affect only that compound setting. Group reset includes settings hidden by filtering. Revert reloads current consumers rather than the values from when the editor first opened.

Rows occupy a real child viewport beneath fixed search and navigation controls. Apply, Revert, Reset group and legacy navigation stay outside that viewport. Tab navigation and validation scroll the target into view. The menu and adjacent regex editor keep their existing native ownership and command-identity rules. Settings and its owned dialogs continue to defer the automatic updater restart.

## Privacy and acceptance limits

Search and drafts remain local. No notification test, remote data transfer or new notification engine is introduced. Each row shows its explicit reset default; configuration-file/default provenance is labelled unavailable because the loader does not expose a per-field presence receipt.

English and Cantonese labels are supplied for the new controls, within the existing language-selection mechanism. Complete bilingual mode, persistent cross-surface appearance, full RegexBuddy parity, element appearance/locks and complete legacy migration remain unfinished. Build success is not runtime keyboard, persistence, accessibility or visual evidence. Those results must be recorded against the exact executable and source revision.

## Bounded 9047/9048 verification

Candidate 9047, source `fd0a87a`, covered English, Cantonese and bilingual in light and dark,
at 1280 × 840 and 760 × 640 client pixels and actual 96 DPI. Its original images exposed
rough doubled disabled-label strokes. Candidate 9048, source `337903e`, uses one explicit
muted-label paint path for normal paint and native printing. All six dark bottom-state tuples
then showed single strokes. The older images remain evidence of the older revision.

In 9048, all six notifications remained off and four unavailable temperature values stayed 80.
A valid disabled daily-traffic draft of 32767 GB was applied and read from configuration before
restoration. Invalid zero left configuration bytes unchanged; Revert recovered 32767 GB.
The fixture was restored to disabled 200 MB with configuration readback. These are existing
consumer checks, not notification-delivery or unavailable sensor-hardware tests.

The [combined receipt](../../evidence/native-surfaces-9048.json) records exact source, package
and executable identities. Portable fixtures exited normally with all five normal-profile
hashes and startup absence unchanged. No installer ran. Higher physical DPI, full reverse
keyboard traversal, IME, complete accessibility and strict image promotion remain unfinished.

## 廣東話操作說明

喺監察工作區或者原有選單開啟設定。每個組別會保留自己嘅搜尋同正規式選項；「所有組別」會列出其他組別嘅結果。選擇結果再按「開啟所選設定」，會暫時顯示該項而唔會清走原本篩選。

每日流量門檻係 1 至 32767 MB 或 GB；記憶體係 1 至 100%；溫度係 1 至 120 °C。冇溫度監察功能嘅版本會保留原有數值，唔會自動開啟硬件監察。停用通知唔會刪除門檻。預設全部通知停用，流量 200 MB，記憶體同溫度門檻 80。

修改先留喺草稿，按「套用」先交畀原有設定流程。無效輸入會顯示返該項，其他草稿保持不變。右鍵、Shift+F10 或「詳情」可開啟解說、重設該項草稿或者還原目前套用值。套用門檻唔等於即時發出通知；編譯成功亦唔等於操作、持久保存同畫面驗證已完成。
