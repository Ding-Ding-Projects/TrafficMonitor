#pragma once
#include <windows.h>
#include <string>

namespace NativeUpdate
{
    enum class Phase { Disabled, Idle, Checking, Downloading, Ready, Applying, Failed, Stopped };
    struct Status { Phase phase{Phase::Disabled}; std::wstring version; DWORD error{}; };
    // No UI is created by this service. Call UI-facing methods only on the UI thread.
    void Start(bool enabled);
    void SetEnabled(bool enabled);
    void RequestCheck();
    void Stop(); // Signals cancellation; never joins a worker on the UI thread.
    Status GetStatus();
    bool AutomaticRestartEnabled();
    bool SaveAutomaticRestart(bool enabled); // False means the preference was not saved.
    // Caller must first establish that no editor, settings or modal work is active.
    // On true, immediately use the normal application exit/save path.
    bool PrepareRestart();
    void RestartQueueFailed(DWORD error); // Retains queue failure without forcing closure.
    // After acquiring the normal instance mutex, before monitor/plugin initialization.
    bool ApplyPendingOnLaunch();
    // Before normal initialization and instance handling, after Squirrel callbacks.
    bool HandleHelper(int& exitCode);
}
