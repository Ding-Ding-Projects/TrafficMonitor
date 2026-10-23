#include "NativeDashboardController.h"
#include <atomic>
#include <new>

struct NativeDashboardController::State
{
    std::mutex mutex;
    HWND destination{};
    UINT message{};
    ULONG_PTR generation{};
    DWORD interval{};
    HANDLE stopEvent{};
    bool wakeQueued{};
    DWORD error{};
    std::shared_ptr<const NativeMonitoring::Snapshot> latest;
    ~State() { if (stopEvent) CloseHandle(stopEvent); }

    void WakeLocked()
    {
        if (destination && !wakeQueued)
            wakeQueued = PostMessageW(destination, message, generation, 0) != FALSE;
    }
};

NativeDashboardController::~NativeDashboardController() { Stop(); }

HRESULT NativeDashboardController::Start(HWND destination, UINT message, DWORD interval)
{
    if (!IsWindow(destination) || message < WM_APP || message > 0xbfff
        || interval < 100 || interval > 60000) return E_INVALIDARG;
    Stop();
    static std::atomic<ULONG_PTR> nextGeneration{ 0 };
    auto state = std::make_shared<State>();
    state->destination = destination;
    state->message = message;
    state->interval = interval;
    state->generation = ++nextGeneration;
    state->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!state->stopEvent) return HRESULT_FROM_WIN32(GetLastError());
    // The single submitted callback owns this baton, including after Stop.
    auto baton = std::make_unique<std::shared_ptr<State>>(state);
    PTP_WORK work = CreateThreadpoolWork(Run, baton.get(), nullptr);
    if (!work) return HRESULT_FROM_WIN32(GetLastError());
    m_state = std::move(state);
    m_generation = m_state->generation;
    m_lastSequence = 0;
    m_work = work;
    baton.release();
    SubmitThreadpoolWork(work);
    return S_OK;
}

void NativeDashboardController::Stop() noexcept
{
    if (m_state)
    {
        // Serialized with PostMessage: no callback can post after detachment.
        std::lock_guard<std::mutex> lock(m_state->mutex);
        m_state->destination = nullptr;
        SetEvent(m_state->stopEvent);
    }
    if (m_work)
    {
        // Windows releases this object asynchronously after its outstanding
        // callback completes. Never wait on uncancellable discovery on the UI.
        CloseThreadpoolWork(m_work);
        m_work = nullptr;
    }
    m_state.reset();
    m_generation = 0;
    m_lastSequence = 0;
}

void CALLBACK NativeDashboardController::Run(PTP_CALLBACK_INSTANCE instance,
    void* context, PTP_WORK)
{
    std::unique_ptr<std::shared_ptr<State>> baton(
        static_cast<std::shared_ptr<State>*>(context));
    const auto state = *baton;
    baton.reset();
    CallbackMayRunLong(instance);
    try
    {
        NativeMonitoring::NetworkSampler sampler;
        while (WaitForSingleObject(state->stopEvent, 0) == WAIT_TIMEOUT)
        {
            auto snapshot = sampler.Sample();
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->latest = std::move(snapshot);
                state->WakeLocked();
            }
            if (WaitForSingleObject(state->stopEvent, state->interval) != WAIT_TIMEOUT) break;
        }
    }
    catch (...)
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->error = ERROR_UNHANDLED_EXCEPTION;
        state->WakeLocked();
    }
}

std::shared_ptr<const NativeMonitoring::Snapshot> NativeDashboardController::Take(WPARAM generation)
{
    if (!m_state || generation != m_generation) return {};
    std::lock_guard<std::mutex> lock(m_state->mutex);
    m_state->wakeQueued = false;
    if (!m_state->latest || m_state->latest->sequence <= m_lastSequence) return {};
    m_lastSequence = m_state->latest->sequence;
    return m_state->latest;
}

DWORD NativeDashboardController::WorkerError() const noexcept
{
    if (!m_state) return ERROR_SUCCESS;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return m_state->error;
}
