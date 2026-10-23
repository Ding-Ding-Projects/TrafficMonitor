# Verified behavior and remaining work

Release `v1.0.8-run39901823689-1` is bound to source
`04a9a136ae800c4b1c547cf09c1bf3688ac0265a`. Its native build, packaging and publication jobs passed.
Downloaded package hashes matched, and the installer is unsigned.

The selected-host supported update from 1.0.4 to 1.0.8 preserved saved configuration bytes and startup
registration. Its stable launcher started the updated executable with the main window hidden and
the compact taskbar monitor attached beneath Explorer. The genuine taskbar image and sanitized
receipt are retained in `evidence/installer/1.0.8/` in the source repository.

Earlier 1.0.3/1.0.4 checks exercised installation, update, owned startup handling, removal and saved
data preservation. Their original evidence remains archived. A 1.0.2 callback-resource problem was
corrected in 1.0.3; the earlier release is not the accepted installer baseline.

## Still unverified or unfinished

- A real subsequent interactive sign-in.
- Automatic taskbar reattachment after an Explorer restart.
- Fresh-host prerequisite acquisition and installer cancellation.
- Manual release-page launch interaction and complete automatic update UI.
- The native Material Design 3 interface conversion and all applicable required features.
- Complete rendered documentation localization, responsive/accessibility matrix and deployed routes.
- Complete English translation of upstream commit history; fork messages remain bilingual verbatim.

Design references and source inventories are not runtime evidence. Current presentation images must
be replaced from genuine rebuilt states as the redesign is delivered. Historical originals retain
their provenance and are not relabelled as new proof.
