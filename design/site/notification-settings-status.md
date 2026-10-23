# Nonblocking feedback settings

The notification contract requires corner anchoring, stacking, timed information,
and warnings/errors that remain until dismissed. The documentation feedback service
now implements these behaviors and exposes Material Web controls for corner and
information duration (6 seconds, 10 seconds, or until dismissed). Preview actions
produce explicitly labeled examples through the same service as real outcomes.

Hover and keyboard focus pause informational dismissal. Closing a focused message
moves focus to another dismiss control or the main workspace. Messages are polite
live regions and never modal. Long content wraps within a scrollable corner stack.
No animation or sound is introduced. English, Cantonese and bilingual copy uses the
existing localizer. Only preferences persist; messages are never stored or exported.
Invalid stored preferences recover to defaults. Failed storage produces a persistent
warning; failed reset retains the previous preferences.

The Settings search and independent regex workbench include this group. The complete
110-item mapping records this partial notification consumer. A notification centre,
its own search/builder, history, bulk operations, optional recovery actions and full
required-feature acceptance remain unfinished. Native application feedback is not
changed. Real browser keyboard, timing, persistence, localization and narrow-layout
verification remains required by the integration owner.

The feedback service retains at most five active message cards and one overflow
summary. Identical message/severity pairs coalesce with a saturating occurrence
count. At capacity, new distinct messages are not retained; the persistent summary
states that loss explicitly and counts occurrences up to the displayed 10000+
bound. No existing card is evicted, including focused cards. Each message is bounded
to 4096 UTF-16 code units; oversized input produces a fixed warning. Dismissing a
card cancels its timer. Pausing and resuming starts a fresh full interval, rather
than preserving remaining milliseconds. Preference changes also restart eligible
informational intervals. This behavior does not create a notification history.
