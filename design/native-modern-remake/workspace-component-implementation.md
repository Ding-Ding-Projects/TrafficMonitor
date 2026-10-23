# Workspace implementation boundary

This bounded unit follows the approved 1280 by 840 normal and 800 by 600 minimum workspace composition: persistent left navigation, measured heading/search/action/status region, and a scrollable monitoring body. Adapters, Sample history and Details are exactly three existing data destinations. Their native button roles are explicit; no unimplemented tab provider is claimed.

The stock bordered search edit, report lists, tab strip and nonclient scrollbar are replaced by the existing TextField, ResultList, Button, ScrollViewport and ScrollableTextField components. Adapter and sample rows retain every prior column value in labelled wrapping rows. This is an adaptive row presentation, not deletion of state, rates, interval or relative time. Native list selection and the existing stable adapter identity model remain authoritative. Details remains selectable read-only native text with an internal rail.

Presentation owns a child data view and a copy of the displayed observation, never a borrowed worker pointer. It retains the existing discontinuity-aware receive/send chart. The third metric is the actual sample interval, not the reference's synthetic daily-total example: this observational sampler does not supply accounting totals. Pause/resume, restart samples, floating monitor, original History and Plugins, Settings, Interface and Setup routes remain real actions. Setup implementation and monitoring backend are unchanged.

This early source checkpoint is uncompiled and unverified. Candidate 1.0.9042 packaging, genuine normal/minimum images, keyboard/focus/selection, live/pause/restart behavior and independent review remain pending. Full language/theme/DPI, UI Automation, clipboard/IME and complete regex parity are not claimed. Remaining native owners and legacy dialogs remain mandatory later units.
## First packaged observation and drawing repair

Candidate 1.0.9042 at 7b8e114ac36ab701beb7f54435cbfb07b6a95d65 compiled and packaged through the root installer entrypoint. The actual child data view was blank in captured normal and minimum states, so those images are rejected visual evidence (TM-WORKSPACE-DATAVIEW-PRINT-001). The new child had used an HWND render target without a print-client path. Candidate 1.0.9043 replaces it with one Direct2D DC drawing path shared by normal painting and print-client handling. Chart discontinuity and observation semantics are unchanged. Corrected pixels remain unverified until the new package runs.

The initial layout also retains text-only navigation rather than the reference's semantic navigation icons. That design-parity gap remains explicit for independent review. No full visual acceptance is claimed.
