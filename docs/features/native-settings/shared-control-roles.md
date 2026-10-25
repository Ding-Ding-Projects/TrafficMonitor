# Shared native control roles

The shared palette distinguishes primary actions, tonal actions, selection, containers,
outlines and their foreground colors. Filled Apply, tonal Revert, outlined actions and text
Details retain their native commands. Selectors have an outline and allocated chevron.
Checkboxes draw 18-unit rounded checked/mixed indicators through one normal paint/print
path. Sliders retain native values and keys, with a 4-unit track, 20-unit thumb and discrete
ticks. Selected list rows use a distinct foreground/background pair and visible check.

Actions use the reviewed 24-unit inset when the measured complete label fits. Existing
owner allocations fall back to the previous 15-unit inset where necessary. Text actions
use 12 and selectors 16. Complete 40/56-unit heights and slider state-layer geometry remain
unfinished. This adaptation preserves labels while owner layout migration continues.

## Verification and limits

Source `6e16e09`, integrated at `4a20ad6`, passed the native build and focused visual,
read-only text and result-list checks. Real Direct2D pixel assertions reject the missing
selector-outline mutation. Measured normal-palette text contrast is at least 6.507:1 for
the primary, tonal and selected pairs. The tests use empty labels, do not prove translated
fit or hover/focus/pressed states, and require high contrast off for that mutation.

Portable 9049 inspected 17 genuine images across Setup, Interface, Notifications, menu,
regex and workspace at 96 DPI. Slider 5→4→5 and checkbox draft 0→1→Revert 0 were read back;
checkbox/revert preserved fixture INI hashes. No candidate was installed. See the
[source-bound receipt](../../evidence/native-components-9049.json).

That run found a menu containment defect after a 320 × 400 resize: Regex remained at
x=321 with width 88, outside the client. Source `9d22668` separates anchored placement
from actual-client child reflow, preserves selected/top rows and retains external sizing
through filtering. The [9050 follow-up](../../evidence/menu-reflow-9050.json), source
`965241d`, verified the same actual 320 × 400 client at 96 DPI: Regex is at x=220..308,
all visible controls are contained, filter/clear changes 16→2→16 rows without changing
the size or selected/top row, and submenu/Back plus owned Regex cancellation retain the
menu. Original 320-wide creation and monitor/DPI moves remain separate unverified items.

Full owner typography, higher physical DPI, high-contrast-enabled behavior, IME, complete
keyboard/accessibility, native renderer measurements and strict image promotion remain
unfinished. Command identities, saved preferences and release hold are retained.
