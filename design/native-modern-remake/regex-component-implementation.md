# Regex workbench component implementation

The next bounded implementation follows the approved Test, Replace, Explain and Library task map. These destinations use accessible native buttons with selected tonal treatment, not a claimed UI Automation tab provider. Pattern entry and explicit Options remain above the scrollable task body; Apply and Cancel stay outside it. Each task retains its outer scroll position and editing controls retain their drafts. Options exposes mode, case and all six insertion helpers.

Native EDIT windows retain selection, caret, undo, clipboard, IME and read-only behavior. The new ScrollableTextField attaches a proportional ScrollRail to actual wrapped display lines rather than replacing the editor text model. ResultList supplies styled selectable match rows and its internal rail. SearchSelector uses the existing searchable menu with an independent regex filter and explicit selected check. Choosing a saved name does not load it or apply an owner filter.

The 1280 by 840 reference and 640 by 400 minimum guide layout. Test subject and results become stacked below 880 logical pixels. Labels and bilingual action heights are measured, and the task body preserves overflow reachability. Supported native frame colors retain caption, resizing and platform accessibility.

This source checkpoint has not yet compiled or run. Exact package 1.0.9036, representative real interaction and images, independent review, full language/theme/DPI coverage, retained accessibility providers, clipboard and IME remain unverified. Engine and collection serialization semantics are unchanged. Complete RegexBuddy parity and other owned surfaces remain unfinished.

## Compact height and native text scrolling

Below 600 logical client pixels in height, the repeated explanatory heading is omitted; the native window title, pattern label, task navigation and fixed actions remain. The normal heading uses 24-pixel semibold type and a separate 14-pixel bilingual secondary line. Task body overflow is intentional and must remain keyboard/wheel reachable. This is not permission to claim the complete layout matrix from one scale.

The multiline rail is an actual editor child, with its width reserved in the native formatting rectangle. Wheel routing accumulates partial deltas, respects the operating-system line/page preference, does nothing while disabled, and allows read-only output to scroll without editing. Buttons inside a section receive its current surface color by value; the shared default remains unchanged for other owners.
