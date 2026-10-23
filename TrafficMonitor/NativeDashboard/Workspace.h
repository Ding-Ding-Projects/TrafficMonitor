#pragma once
#include <afxwin.h>
#include <functional>

namespace NativeDashboard
{
    // Explicit user action only. Does not change monitoring or startup defaults.
    bool Open(CWnd* owner, DWORD intervalMilliseconds, std::function<void()> floatingMonitor);
}
