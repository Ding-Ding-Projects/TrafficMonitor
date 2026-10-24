# Native design refinement continuation

Owner direction: pure Material Design 3 client controls, less clutter and no generic stock presentation, while retaining native caption controls. The owner requested preservation and cleanup before implementation completion.

This branch contains122 editable reference proposals under design/native-modern-remake, based on the preserved native workspace source. The first proposal is25ad3adcb8c5cdf7c0d32a62c065a0b612b5fafd. The corrected proposal is dee325bae359d05a33714ccaa2f856dda2b43e98.

Corrections enforce translated secondary text of at least14 logical pixels with measured line boxes, expose Interface Details through proposed InterfaceDetails=56206 in NativePreferences::Editor, and provide complete unavailable Notifications styling with readable reasons. XML parsing, nonnegative dimensions, text-size floor and byte-identical regeneration passed for122 SVGs. Twenty-six current representative SVG/raster pairs were rebound and inspected by the author.

Final independent re-review remains pending. These files are proposals, not production UI, runtime screenshots or full accessibility/display-matrix acceptance. Do not merge the branch's rejected workspace ancestry into main as accepted behavior. Integrate only independently approved changes through the owning task and retain the branch/worktree while its tip lacks proven main ancestry.

Read release48-refinement.md, the action map and the review record. Re-review the unchanged corrected candidate before implementation. Preserve every real action, draft, persistence and protected-close contract. Keep the existing producer on the separate responsiveness branch pinned until its actual result is known.

No further implementation is authorized by this handoff itself. Resume only under the owner's continuing task direction. Preserve public source language, private evidence boundaries and original captures. No forced process termination, force push, host power/login action or unproven cleanup.
