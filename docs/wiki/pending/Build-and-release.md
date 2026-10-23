# Build and release

Run root `build.bat` for the maintained x64 `Release (lite)` target. Run root
`build-installer.bat` to produce the genuine unsigned Squirrel.Windows package. The scripts detect
supported missing tools and use verified acquisition routes. MSVC v143, MFC and the selected Windows
SDK are required; native UAC consent belongs to the user. Cancellation or a required restart is an
incomplete result, never automatic restart or successful no-op.

The installer packages the native executable, matching runtime DLLs, resources and installed-mode
marker. Normal release delivery includes `Setup.exe`, `RELEASES`, the full `.nupkg` and
`SHA256SUMS.txt`. Release tags are bound to the intended source commit and read back after publication.

`build.bat --site` is the documentation build route. It uses pinned Node 24.10.0 and locked local
dependencies, then builds `docs/site/dist/`. The website contains complete local articles and wiki
content. Reading-progress controls never execute the installed application.

The workflow keeps the required publication-language check before native packaging and documentation
builds. General tests and lint are not run in that workflow. Local focused checks and actual runtime
verification are separate evidence. Publishing a release does not prove every required feature.

Preserve the upstream license and attribution when distributing builds. See the source repository's
build documentation and handoff for exact limitations and source-bound evidence.
