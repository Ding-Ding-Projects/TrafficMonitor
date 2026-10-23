#pragma once
#include <afxwin.h>
#include <cstdint>
#include <string>
#include <vector>

namespace NativeDashboard::Setup
{
    enum class Mode { Automatic, All, One };
    struct Adapter { std::wstring guid, name; ULONG index{}; unsigned long long luid{}; };
    struct Selection { Mode mode{Mode::Automatic}; std::wstring guid; };
    struct Discovery { std::vector<Adapter> adapters; std::uint64_t epoch{}; DWORD error{}; bool allAllowed{}; };
    Discovery Discover();
    // Called only by explicit dashboard navigation. False means saved/incomplete.
    bool Show(CWnd* owner, bool explicitResume = false);
}
