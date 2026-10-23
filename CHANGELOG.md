# Fork changes

## Installer foundation, in verification

- Add native x64 build and genuine unsigned Squirrel.Windows packaging entrypoints.
- Include matching application-local Microsoft CRT and MFC runtime dependencies.
- Handle installer callbacks before interactive startup and single-instance detection.
- Register current-user login startup through a stable launcher, with ownership checks and readback.
- Keep installed configuration outside versioned executable folders.
- Default missing installed preferences to taskbar-visible and main-window-hidden.
- Preserve saved settings, startup choices, portable behavior, plugins, and deleted shortcuts.
- Add a source-bound normal-release workflow with retained safe build outputs.

Runtime installation and update acceptance remain separate from source and build verification.
