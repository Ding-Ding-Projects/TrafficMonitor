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
            ModifyStyle(0, WS_CLIPCHILDREN);
            m_rail.Create(this, GetDlgCtrlID() + 1200);
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
        CRect client;
        GetClientRect(client);
        const int railWidth = MulDiv(12, GetDpiForWindow(m_hWnd), 96);
        CRect format;
        GetRect(format);
        const int right = (std::max)(format.left + 1, client.right - railWidth - 4);
        if (format.right != right)
        {
            format.right = right;
            SetRectNP(format);
        }
        m_rail.MoveWindow(client.right - railWidth, 0, railWidth, client.Height());
        CClientDC dc(this);
        CFont* previous = GetFont() ? dc.SelectObject(GetFont()) : nullptr;
        TEXTMETRIC metric{};
        dc.GetTextMetrics(&metric);
        if (previous) dc.SelectObject(previous);
        const int page = (std::max)(1, format.Height() / (std::max)(1, static_cast<int>(metric.tmHeight)));
        m_rail.SetMetrics(GetLineCount(), page, GetFirstVisibleLine());
        m_rail.EnableWindow(IsWindowEnabled());
        m_sync = false;
    }
    LRESULT ScrollableTextField::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        const auto result = TextField::WindowProc(message, w, l);
        switch (message)
        {
        case WM_WINDOWPOSCHANGED: case WM_ENABLE:
        case WM_SETFONT: case WM_SETTEXT: case WM_KEYDOWN: case WM_KEYUP:
        case WM_MOUSEWHEEL: case WM_VSCROLL: case WM_LBUTTONUP:
        case EM_SETSEL: case EM_REPLACESEL: case EM_SCROLLCARET:
            SyncScroll();
            break;
        }
        return result;
    }
}
