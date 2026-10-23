#pragma once
#include <Windows.h>
#include "NativeMonitoringSnapshot.h"

// UI-thread owner of one observational sampling session. Stop detaches message
// delivery immediately; outstanding OS discovery finishes on the thread pool.
class NativeDashboardController final
{
public:
    NativeDashboardController() = default;
    ~NativeDashboardController();
    NativeDashboardController(const NativeDashboardController&) = delete;
    NativeDashboardController& operator=(const NativeDashboardController&) = delete;
    HRESULT Start(HWND destination, UINT message, DWORD intervalMilliseconds);
    void Stop() noexcept;
    std::shared_ptr<const NativeMonitoring::Snapshot> Take(WPARAM generation);
    ULONG_PTR Generation() const noexcept { return m_generation; }
    DWORD WorkerError() const noexcept;

private:
    struct State;
    static void CALLBACK Run(PTP_CALLBACK_INSTANCE instance, void* context, PTP_WORK work);
    std::shared_ptr<State> m_state;
    PTP_WORK m_work{};
    ULONG_PTR m_generation{};
    std::uint64_t m_lastSequence{};
};
