#include "../stdafx.h"
#include "InputControls.Internal.h"
#include "Snackbar.h"
#include <algorithm>

namespace NativeMaterial
{
    namespace
    {
        constexpr UINT_PTR ClockTimer = 0x5C1;
        constexpr UINT ClockIntervalMs = 250;

        std::function<ULONGLONG()>& ClockSource() { static std::function<ULONGLONG()> source; return source; }
        Snackbar::NotificationObserver& Observer() { static Snackbar::NotificationObserver observer; return observer; }
        ULONGLONG Now() { const auto& source = ClockSource(); return source ? source() : GetTickCount64(); }
    }

    void Snackbar::SetClock(std::function<ULONGLONG()> clock) { ClockSource() = std::move(clock); }
    void Snackbar::SetNotificationObserver(NotificationObserver observer) { Observer() = std::move(observer); }

    unsigned Snackbar::EffectiveTimeout(const SnackbarMessage& message)
    {
        if (message.persistent) return 0;
        // A message with an action stays longest: the person has to read it and then reach the action.
        if (message.timeoutMs == 0) return message.action.IsEmpty() ? 5000u : SnackbarMaximumTimeoutMs;
        return (std::max)(SnackbarMinimumTimeoutMs, (std::min)(SnackbarMaximumTimeoutMs, message.timeoutMs));
    }

    BOOL Snackbar::Create(CWnd* host, UINT id)
    {
        if (!host || !host->GetSafeHwnd()) return FALSE;
        // Not visible until a message is shown; clipped against siblings so it can sit above them.
        if (!CreateInput(InputKind::Snackbar, CString(), WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS, 0, CRect(0, 0, 344, 48), host, id)) return FALSE;
        SetTimer(ClockTimer, ClockIntervalMs, nullptr);
        return TRUE;
    }

    unsigned Snackbar::Show(const SnackbarMessage& message)
    {
        if (!m_hWnd || message.text.IsEmpty()) return 0;
        Entry entry{ m_nextId++, message };
        if (m_nextId == 0) m_nextId = 1;
        if (!m_showing) { Present(std::move(entry)); return m_current.id; }
        if (m_queue.size() >= SnackbarMaximumQueued)
        {
            Entry dropped = std::move(m_queue.front());
            m_queue.pop_front();
            m_lastResult = SnackbarResult::Dropped; m_lastId = dropped.id;
            if (dropped.message.onResult) dropped.message.onResult(dropped.id, SnackbarResult::Dropped);
        }
        const unsigned id = entry.id;
        m_queue.push_back(std::move(entry));
        return id;
    }

    void Snackbar::Present(Entry entry)
    {
        m_current = std::move(entry);
        m_showing = true;
        m_held = false;
        m_focusPart = SnackbarPart::None; m_hoverPart = SnackbarPart::None; m_pressedPart = SnackbarPart::None;
        m_timeout = EffectiveTimeout(m_current.message);
        m_deadline = Now() + m_timeout;
        SetWindowText(m_current.message.text);
        Reposition();
        Publish();
        ::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        Invalidate(FALSE);
        Announce();
    }

    void Snackbar::Announce()
    {
        CString display = m_current.message.text;
        if (!m_current.message.action.IsEmpty()) display += L". " + m_current.message.action;
        const bool important = m_current.message.persistent;
        if (const auto& observer = Observer()) observer(display, important);
        Microsoft::WRL::ComPtr<IRawElementProviderSimple> provider;
        if (FAILED(AutomationProvider(provider.GetAddressOf())) || !provider) { m_notifyResult = E_UNEXPECTED; return; }
        BSTR text = SysAllocString(display.GetString());
        BSTR activity = SysAllocString(L"MaterialSnackbar");
        // Polite: queued behind what the reader is saying. Persistent warnings and errors are important.
        m_notifyResult = UiaRaiseNotificationEvent(provider.Get(), NotificationKind_Other,
            important ? NotificationProcessing_ImportantAll : NotificationProcessing_All, text, activity);
        SysFreeString(text); SysFreeString(activity);
    }

    void Snackbar::Finish(SnackbarResult result)
    {
        if (!m_showing) return;
        Entry done = std::move(m_current);
        m_current = Entry();
        if (result == SnackbarResult::Action) Notify(SnackbarNotifyAction);
        m_showing = false;
        m_lastResult = result; m_lastId = done.id;
        const bool hadFocus = m_hasFocus;
        ::ShowWindow(m_hWnd, SW_HIDE);
        if (hadFocus) LeaveFocus();
        if (done.message.onResult) done.message.onResult(done.id, result);
        Notify(SnackbarNotifyDismissed);
        if (!m_showing && !m_queue.empty())
        {
            Entry next = std::move(m_queue.front());
            m_queue.pop_front();
            Present(std::move(next));
        }
    }

    void Snackbar::Dismiss() { Finish(SnackbarResult::Dismissed); }

    void Snackbar::Clear()
    {
        while (!m_queue.empty())
        {
            Entry dropped = std::move(m_queue.front());
            m_queue.pop_front();
            m_lastResult = SnackbarResult::Dropped; m_lastId = dropped.id;
            if (dropped.message.onResult) dropped.message.onResult(dropped.id, SnackbarResult::Dropped);
        }
        Finish(SnackbarResult::Dismissed);
    }

    void Snackbar::Tick()
    {
        if (!m_showing || m_timeout == 0) return;
        // Pointer over the message or keyboard focus inside it holds the countdown; it restarts afterwards.
        if (m_hover || m_hasFocus) { m_held = true; return; }
        if (m_held) { m_held = false; m_deadline = Now() + m_timeout; return; }
        if (Now() >= m_deadline) Finish(SnackbarResult::Timeout);
    }

    void Snackbar::Reposition()
    {
        if (!m_hWnd || !m_showing) return;
        CWnd* host = GetParent();
        if (!host) return;
        CRect hostClient;
        host->GetClientRect(hostClient);
        const float width = SnackbarWidthFor(Dip(hostClient.Width()));
        const float action = SnackbarActionWidth(m_current.message.action);
        m_twoLines = SnackbarNeedsTwoLines(m_current.message.text, width, action, m_current.message.closable);
        const float height = m_twoLines ? FeedbackMetrics::SnackbarTwoLines : FeedbackMetrics::SnackbarOneLine;
        const int w = Px(width), h = Px(height);
        const int x = Px(FeedbackMetrics::SnackbarMargin);
        const int y = (std::max)(0, static_cast<int>(hostClient.bottom) - Px(FeedbackMetrics::SnackbarMargin) - h);
        ::SetWindowPos(m_hWnd, HWND_TOP, x, y, w, h, SWP_NOACTIVATE);
        // Rounded corners: the window itself is cut to the container shape.
        const int diameter = 2 * Px(FeedbackMetrics::SnackbarCorner);
        if (HRGN region = CreateRoundRectRgn(0, 0, w + 1, h + 1, diameter, diameter)) SetWindowRgn(region, TRUE);
        Publish();
        Invalidate(FALSE);
    }

    SnackbarLayout Snackbar::CurrentLayout() const
    {
        CRect client;
        GetClientRect(client);
        return LayoutSnackbar(Dip(client.Width()), m_twoLines, SnackbarActionWidth(m_current.message.action), m_current.message.closable);
    }

    CRect Snackbar::PartRect(SnackbarPart part) const
    {
        if (!m_showing || part == SnackbarPart::None) return CRect();
        const auto layout = CurrentLayout();
        const auto& r = part == SnackbarPart::Action ? layout.action : layout.close;
        if ((part == SnackbarPart::Action && !layout.hasAction) || (part == SnackbarPart::Close && !layout.hasClose)) return CRect();
        return CRect(Px(r.left), Px(r.top), Px(r.right), Px(r.bottom));
    }

    SnackbarPart Snackbar::PartAt(CPoint point) const
    {
        if (PartRect(SnackbarPart::Action).PtInRect(point)) return SnackbarPart::Action;
        if (PartRect(SnackbarPart::Close).PtInRect(point)) return SnackbarPart::Close;
        return SnackbarPart::None;
    }

    void Snackbar::Activate(SnackbarPart part)
    {
        if (!m_showing) return;
        if (part == SnackbarPart::Action && !m_current.message.action.IsEmpty()) Finish(SnackbarResult::Action);
        else Finish(SnackbarResult::Close);
    }

    void Snackbar::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        SnackbarModel model;
        model.text = m_current.message.text; model.action = m_current.message.action; model.closable = m_current.message.closable;
        model.twoLines = m_twoLines; model.focusPart = m_focusPart; model.hoverPart = m_hoverPart; model.pressedPart = m_pressedPart;
        DrawSnackbar(target, palette, model, state);
    }

    CSize Snackbar::MinimumLogicalSize() const { return CSize(200, static_cast<int>(FeedbackMetrics::SnackbarOneLine)); }

    void Snackbar::FillAccess(InputData& data) const
    {
        data.help = m_current.message.action.GetString();
        data.shortcut = L"F6";
    }

    void Snackbar::PointerPressed(CPoint point) { m_pressedPart = PartAt(point); }
    void Snackbar::PointerReleased(CPoint point, bool)
    {
        const auto pressed = m_pressedPart;
        m_pressedPart = SnackbarPart::None;
        if (pressed != SnackbarPart::None && PartAt(point) == pressed) Activate(pressed);
    }
    void Snackbar::PointerMoved(CPoint point) { m_hoverPart = PartAt(point); }
    void Snackbar::PointerLeft() { m_hoverPart = SnackbarPart::None; }
    void Snackbar::PointerCanceled() { m_pressedPart = SnackbarPart::None; }

    bool Snackbar::WantsKey(UINT key) const
    {
        return key == VK_ESCAPE || key == VK_LEFT || key == VK_RIGHT || key == VK_RETURN || key == VK_SPACE || key == VK_F6;
    }
    UINT Snackbar::DialogCode() const { return m_hasFocus ? DLGC_WANTARROWS : 0; }

    bool Snackbar::KeyPressed(UINT key, bool)
    {
        if (!m_showing) return false;
        if (key == VK_ESCAPE) { Finish(SnackbarResult::Close); return true; }
        if (key == VK_F6) { LeaveFocus(); return true; }
        if (key == VK_LEFT || key == VK_RIGHT)
        {
            std::vector<SnackbarPart> parts;
            if (!m_current.message.action.IsEmpty()) parts.push_back(SnackbarPart::Action);
            if (m_current.message.closable) parts.push_back(SnackbarPart::Close);
            if (parts.empty()) return true;
            int index = -1;
            for (size_t i = 0; i < parts.size(); ++i) if (parts[i] == m_focusPart) index = static_cast<int>(i);
            const int count = static_cast<int>(parts.size());
            index = index < 0 ? (key == VK_RIGHT ? 0 : count - 1) : (index + (key == VK_RIGHT ? 1 : count - 1)) % count;
            m_focusPart = parts[index];
            return true;
        }
        if (key == VK_RETURN || key == VK_SPACE)
        {
            m_pressed = true;
            m_pressedPart = m_focusPart;
            return true;
        }
        return false;
    }

    bool Snackbar::KeyReleased(UINT key)
    {
        if (!m_showing || (key != VK_RETURN && key != VK_SPACE) || !m_pressed) return false;
        m_pressed = false;
        m_pressedPart = SnackbarPart::None;
        SnackbarPart part = m_focusPart;
        if (part == SnackbarPart::None) part = m_current.message.action.IsEmpty() ? SnackbarPart::Close : SnackbarPart::Action;
        Activate(part);
        return true;
    }

    bool Snackbar::TranslateShortcut(const MSG& message)
    {
        if (!m_showing || message.message != WM_KEYDOWN || message.wParam != VK_F6) return false;
        if (m_hasFocus) { LeaveFocus(); return true; }
        return FocusMessage();
    }

    bool Snackbar::FocusMessage()
    {
        if (!m_showing || !m_hWnd) return false;
        HWND previous = ::GetFocus();
        if (previous != m_hWnd) m_returnFocus = previous;
        if (m_focusPart == SnackbarPart::None)
            m_focusPart = !m_current.message.action.IsEmpty() ? SnackbarPart::Action : m_current.message.closable ? SnackbarPart::Close : SnackbarPart::None;
        ::SetFocus(m_hWnd);
        return true;
    }

    void Snackbar::LeaveFocus()
    {
        HWND target = (m_returnFocus && ::IsWindow(m_returnFocus) && m_returnFocus != m_hWnd) ? m_returnFocus : (GetParent() ? GetParent()->GetSafeHwnd() : nullptr);
        m_returnFocus = nullptr;
        m_hasFocus = false;
        if (target) ::SetFocus(target);
    }

    void Snackbar::DpiChanged() { Reposition(); }

    void Snackbar::PerformAction(InputActionId action, int)
    {
        // Invoke: the action when there is one, otherwise dismiss.
        if (action == InputActionId::Toggle && m_showing) Activate(m_current.message.action.IsEmpty() ? SnackbarPart::Close : SnackbarPart::Action);
    }

    LRESULT Snackbar::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == WM_TIMER && w == ClockTimer) { Tick(); return 0; }
        if (message == WM_SETFOCUS) { m_hasFocus = true; if (m_showing && m_focusPart == SnackbarPart::None) m_focusPart = !m_current.message.action.IsEmpty() ? SnackbarPart::Action : m_current.message.closable ? SnackbarPart::Close : SnackbarPart::None; }
        if (message == WM_KILLFOCUS) { m_hasFocus = false; m_pressed = false; m_pressedPart = SnackbarPart::None; }
        // A click must not move keyboard focus: the message is not a place to type.
        if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
        return InputWindow::WindowProc(message, w, l);
    }
}
