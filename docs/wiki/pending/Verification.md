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


## Current supported update baseline

Normal release [1.0.19](https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.19-run39942930510-1)
is bound to source `77435775e46ca78de13859a9e4f2a15732e53415`; its four release jobs
completed successfully. The separately assigned supported installed-update operation
reported updater and stable launcher exit 0, post-save data/startup preservation,
hidden main window and visible taskbar monitor from metadata only. This does not
prove automatic five-minute timing, cooperative restart, next-launch apply, failure
recovery or modal/draft deferral. See [[Silent update behavior and limits|Silent-updates]].
The native MFC conversion and all 110 applicable required features remain unfinished.
## Still unverified or unfinished

- A real subsequent interactive sign-in.
- Automatic taskbar reattachment after an Explorer restart.
- Fresh-host prerequisite acquisition and installer cancellation.
- Automatic update cadence, cooperative restart, next-launch apply, modal/draft deferral and failure recovery.
- The native Material Design 3 interface conversion and all applicable required features.
- Complete rendered documentation localization, responsive/accessibility matrix and deployed routes.
- Complete English translation of upstream commit history; fork messages remain bilingual verbatim.

Design references and source inventories are not runtime evidence. Current presentation images must
be replaced from genuine rebuilt states as the redesign is delivered. Historical originals retain
their provenance and are not relabelled as new proof.
