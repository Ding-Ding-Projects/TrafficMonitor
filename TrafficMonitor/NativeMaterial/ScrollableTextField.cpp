#include "../stdafx.h"
#include "ScrollableTextField.h"
#include "../NativePreferences/Text.h"
namespace NativeMaterial
{
    void ScrollableTextField::SetAppearance(Appearance appearance)
    {
        TextField::SetAppearance(appearance);
        if (!m_rail.GetSafeHwnd())
        {
            m_rail.Create(GetParent(), GetDlgCtrlID() + 1200);
            m_rail.SetStep(1);
            m_rail.changed = [this](int position)
            {
                LineScroll(position - GetFirstVisibleLine());
                SyncScroll();
            };
        }
        m_rail.SetAppearance(appearance);
        m_rail.SetWindowText(NativePreferences::Pair(L"Scroll text",L"捲動文字"));
        SyncScroll();
    }
    void ScrollableTextField::SyncScroll()
    {
        if (m_sync || !m_rail.GetSafeHwnd() || !GetSafeHwnd()) return;
        m_sync = true;
        CRect bounds;
        GetWindowRect(bounds);
        GetParent()->ScreenToClient(bounds);
        const int railWidth = MulDiv(12, GetDpiForWindow(m_hWnd), 96);
        m_rail.MoveWindow(bounds.right - railWidth, bounds.top, railWidth, bounds.Height());
        CRect format;
        GetRect(format);
        CClientDC dc(this);
        CFont* previous = GetFont() ? dc.SelectObject(GetFont()) : nullptr;
        TEXTMETRIC metric{};
        dc.GetTextMetrics(&metric);
        if (previous) dc.SelectObject(previous);
        const int page = (std::max)(1, format.Height() / (std::max)(1, static_cast<int>(metric.tmHeight)));
        m_rail.SetMetrics(GetLineCount(), page, GetFirstVisibleLine());
        if (!(GetStyle() & WS_VISIBLE)) m_rail.ShowWindow(SW_HIDE);
        m_rail.EnableWindow(IsWindowEnabled());
        m_rail.BringWindowToTop();
        m_sync = false;
    }
    LRESULT ScrollableTextField::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == WM_NCCALCSIZE)
        {
            const auto result = TextField::WindowProc(message, w, l);
            RECT* rect = w ? &reinterpret_cast<NCCALCSIZE_PARAMS*>(l)->rgrc[0] : reinterpret_cast<RECT*>(l);
            rect->right = (std::max)(rect->left, rect->right - MulDiv(12, GetDpiForWindow(m_hWnd), 96));
            return result;
        }
        const auto result = TextField::WindowProc(message, w, l);
        switch (message)
        {
        case WM_WINDOWPOSCHANGED: case WM_SHOWWINDOW: case WM_ENABLE:
        case WM_SETFONT: case WM_SETTEXT: case WM_KEYDOWN: case WM_KEYUP:
        case WM_MOUSEWHEEL: case WM_VSCROLL: case WM_LBUTTONUP:
        case EM_SETSEL: case EM_REPLACESEL: case EM_SCROLLCARET:
            SyncScroll();
            break;
        case WM_DESTROY:
            if (m_rail.GetSafeHwnd()) m_rail.DestroyWindow();
            break;
        }
        return result;
    }
}
