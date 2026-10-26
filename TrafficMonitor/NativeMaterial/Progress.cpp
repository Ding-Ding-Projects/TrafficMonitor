#include "../stdafx.h"
#include "InputControls.Internal.h"
#include "Progress.h"
#include <algorithm>
#include <cmath>

namespace NativeMaterial
{
    namespace
    {
        constexpr UINT_PTR ClockTimerId = 1;
        constexpr UINT ClockIntervalMs = 33;
        std::function<bool()>& MotionProbe() { static std::function<bool()> probe; return probe; }
        std::vector<Progress*>& Members() { static std::vector<Progress*> members; return members; }
        HWND g_clockWindow{};

        LRESULT CALLBACK ClockProc(HWND window, UINT message, WPARAM w, LPARAM l)
        {
            if (message == WM_TIMER && w == ClockTimerId)
            {
                const ULONGLONG now = GetTickCount64();
                const auto members = Members();    // a copy: an indicator may leave while advancing
                for (auto* member : members) if (member->IsAnimating()) member->Advance(now);
                return 0;
            }
            return DefWindowProcW(window, message, w, l);
        }

        void EnsureClock()
        {
            if (g_clockWindow) return;
            WNDCLASSW cls{};
            cls.lpfnWndProc = ClockProc;
            cls.hInstance = GetModuleHandleW(nullptr);
            cls.lpszClassName = L"TrafficMonitorMaterialClock";
            RegisterClassW(&cls);    // fails harmlessly when it already exists
            g_clockWindow = CreateWindowExW(0, cls.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, cls.hInstance, nullptr);
            if (g_clockWindow) SetTimer(g_clockWindow, ClockTimerId, ClockIntervalMs, nullptr);
        }
        void ReleaseClock()
        {
            if (!g_clockWindow || !Members().empty()) return;
            KillTimer(g_clockWindow, ClockTimerId);
            DestroyWindow(g_clockWindow);
            g_clockWindow = nullptr;
        }
    }

    void Progress::SetReducedMotionProbe(std::function<bool()> probe) { MotionProbe() = std::move(probe); }
    int Progress::RegisteredIndicators() { return static_cast<int>(Members().size()); }
    bool Progress::ClockRunning() { return g_clockWindow != nullptr; }

    Progress::Progress() = default;
    Progress::~Progress() { LeaveClock(); }

    void Progress::JoinClock()
    {
        if (m_joined) return;
        EnsureClock();
        Members().push_back(this);
        m_joined = true;
    }
    void Progress::LeaveClock()
    {
        if (!m_joined) return;
        auto& members = Members();
        members.erase(std::remove(members.begin(), members.end(), this), members.end());
        m_joined = false;
        ReleaseClock();
    }

    bool Progress::ReducedMotion() const
    {
        if (const auto& probe = MotionProbe()) return probe();
        BOOL animations = TRUE;
        return SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0) && !animations;
    }

    BOOL Progress::Create(const CString& name, ProgressShape shape, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_shape = shape;
        if (!CreateInput(InputKind::Progress, name, WS_CHILD | WS_VISIBLE, 0, bounds, parent, id)) return FALSE;
        if (m_indeterminate) JoinClock();
        return TRUE;
    }

    void Progress::SetName(const CString& name) { SetWindowText(name); Publish(); }

    float Progress::Fraction() const
    {
        const int span = m_maximum - m_minimum;
        return span <= 0 ? 0.f : static_cast<float>(m_value - m_minimum) / static_cast<float>(span);
    }

    void Progress::SetRange(int minimum, int maximum)
    {
        m_minimum = minimum;
        m_maximum = maximum > minimum ? maximum : minimum + 1;
        m_value = (std::max)(m_minimum, (std::min)(m_maximum, m_value));
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }

    void Progress::SetValue(int value)
    {
        const int next = (std::max)(m_minimum, (std::min)(m_maximum, value));
        if (next == m_value) return;
        m_value = next;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }

    void Progress::SetIndeterminate(bool indeterminate)
    {
        if (m_indeterminate == indeterminate) return;
        m_indeterminate = indeterminate;
        if (m_indeterminate && m_hWnd) JoinClock(); else LeaveClock();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }

    void Progress::SetStatusText(const CString& text)
    {
        m_status = text;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }

    CString Progress::EffectiveStatusText() const
    {
        if (!m_status.IsEmpty() || !m_indeterminate || !ReducedMotion()) return m_status;
        return NativePreferences::Pair(L"Working…", L"處理緊…");
    }

    void Progress::Advance(ULONGLONG now)
    {
        const ULONGLONG period = m_shape == ProgressShape::Linear ? 1750 : 1500;
        m_phase = static_cast<float>(now % period) / static_cast<float>(period);
        if (m_hWnd && IsWindowVisible()) Invalidate(FALSE);
    }

    void Progress::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        ProgressModel model;
        model.shape = m_shape;
        model.indeterminate = m_indeterminate;
        model.fraction = Fraction();
        model.phase = m_phase;
        model.reducedMotion = ReducedMotion();
        model.status = EffectiveStatusText();
        DrawProgress(target, palette, model, state);
    }

    CSize Progress::MinimumLogicalSize() const
    {
        if (m_shape == ProgressShape::Circular) return CSize(static_cast<int>(FeedbackMetrics::CircularMinimum), static_cast<int>(FeedbackMetrics::CircularMinimum));
        return CSize(64, static_cast<int>(m_status.IsEmpty() ? FeedbackMetrics::LinearBare : FeedbackMetrics::LinearWithText));
    }

    void Progress::FillAccess(InputData& data) const
    {
        data.value = m_value; data.minimum = m_minimum; data.maximum = m_maximum; data.step = 1;
        data.indeterminate = m_indeterminate;
        data.help = EffectiveStatusText().GetString();
    }

    LRESULT Progress::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == WM_DESTROY) LeaveClock();
        return InputWindow::WindowProc(message, w, l);
    }
}
