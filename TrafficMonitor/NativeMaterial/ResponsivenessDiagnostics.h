#pragma once
#include <Windows.h>
#include <string>

// Explicit local opt-in for numeric UI-thread diagnostics. No text, settings,
// network observations or user input enter this mapping. It expires at exit.
namespace NativeDiagnostics
{
    enum class Operation : unsigned
    {
        Apply, Publish, SystemAppearance, Notify, Receive, RefreshRows,
        RefreshDetails, Layout, Draw, Measure, FrameAppearance, Posted,
        WorkspaceThemeMessage, WorkspaceSettingMessage, SettingsThemeMessage,
        SettingsSettingMessage, SettingsReceive, Count
    };
    // Readers take repeated snapshots: individual counters are atomic, a whole
    // snapshot is not transactional. startedTicks is the latest entry, including
    // nested calls. Durations are inclusive; Layout counts attempted entries,
    // including entries rejected by its existing reentrancy check.
    struct Counter { LONG64 calls, active, totalTicks, maximumTicks, startedTicks, maximumDepth; };
    struct Snapshot
    {
        LONG64 version, frequency, processId;
        Counter counters[static_cast<unsigned>(Operation::Count)];
    };
    class Mapping
    {
    public:
        Mapping()
        {
            wchar_t executable[32768]{};
            const DWORD length = GetModuleFileNameW(nullptr, executable, _countof(executable));
            if (!length || length >= _countof(executable)) return;
            const std::wstring marker = std::wstring(executable, length) + L".ui-diagnostics";
            if (GetFileAttributesW(marker.c_str()) == INVALID_FILE_ATTRIBUTES) return;
            const std::wstring name = L"Local\\TrafficMonitor.UiDiagnostics." + std::to_wstring(GetCurrentProcessId());
            m_handle = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(Snapshot), name.c_str());
            if (!m_handle) return;
            if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(m_handle); m_handle = nullptr; return; }
            data = static_cast<Snapshot*>(MapViewOfFile(m_handle, FILE_MAP_WRITE, 0, 0, sizeof(Snapshot)));
            if (!data) return;
            LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency);
            data->version = 2; data->frequency = frequency.QuadPart; data->processId = GetCurrentProcessId();
        }
        ~Mapping() { if (data) UnmapViewOfFile(data); if (m_handle) CloseHandle(m_handle); }
        Snapshot* data{};
    private:
        HANDLE m_handle{};
    };
    inline Snapshot* Data() { static Mapping mapping; return mapping.data; }
    class Scope
    {
    public:
        explicit Scope(Operation operation)
        {
            if (auto* data = Data())
            {
                m_counter = &data->counters[static_cast<unsigned>(operation)];
                QueryPerformanceCounter(&m_started);
                InterlockedIncrement64(&m_counter->calls);
                const LONG64 depth = InterlockedIncrement64(&m_counter->active);
                if (depth > m_counter->maximumDepth) InterlockedExchange64(&m_counter->maximumDepth, depth);
                InterlockedExchange64(&m_counter->startedTicks, m_started.QuadPart);
            }
        }
        ~Scope()
        {
            if (!m_counter) return;
            LARGE_INTEGER ended{}; QueryPerformanceCounter(&ended);
            const LONG64 elapsed = ended.QuadPart - m_started.QuadPart;
            InterlockedAdd64(&m_counter->totalTicks, elapsed);
            // All instrumented operations are owned by the same UI thread.
            if (elapsed > m_counter->maximumTicks) InterlockedExchange64(&m_counter->maximumTicks, elapsed);
            InterlockedDecrement64(&m_counter->active);
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    private:
        Counter* m_counter{};
        LARGE_INTEGER m_started{};
    };
}
