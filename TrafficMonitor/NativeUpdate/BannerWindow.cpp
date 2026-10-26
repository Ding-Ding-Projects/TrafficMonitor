#include "../stdafx.h"
#include "BannerWindow.h"
#include "../NativePreferences/Text.h"
#include <dwmapi.h>
#include <commctrl.h>

#pragma comment(lib, "dwmapi.lib")

namespace NativeUpdate
{
    namespace
    {
        // The panel covers the whole surface; letting hit tests fall through to the banner makes the empty surface draggable.
        LRESULT CALLBACK PassThroughHitTest(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR)
        {
            if (message == WM_NCHITTEST) return HTTRANSPARENT;
            if (message == WM_NCDESTROY) RemoveWindowSubclass(hwnd, PassThroughHitTest, id);
            return DefSubclassProc(hwnd, message, wparam, lparam);
        }
    }

    BEGIN_MESSAGE_MAP(BannerWindow, CWnd)
        ON_WM_CLOSE()
    END_MESSAGE_MAP()

    BOOL BannerWindow::Create()
    {
        const CString title = NativePreferences::Pair(L"TrafficMonitor update", L"TrafficMonitor 更新");
        // No owner: the notice stays visible even when the main monitor window is hidden.
        if (!CreateEx(WS_EX_TOPMOST | WS_EX_CONTROLPARENT, AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW)),
            title, WS_POPUP | WS_CLIPCHILDREN, CRect(0, 0, 600, 200), nullptr, 0))
            return FALSE;
        // Frameless Material surface: rounded corners and the DWM shadow replace the system caption and border.
        const DWORD corner = 2; // DWMWCP_ROUND
        DwmSetWindowAttribute(m_hWnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));
        const MARGINS shadow{ 0, 0, 1, 0 };
        DwmExtendFrameIntoClientArea(m_hWnd, &shadow);
        const CRect box(0, 0, 100, 40);
        const DWORD child = WS_CHILD | WS_VISIBLE;
        if (!m_panel.Create(this, 61100)) return FALSE;
        m_panel.SetFlat(true);
        m_headline.Create(L"", child, box, &m_panel);
        m_detail.Create(L"", child, box, &m_panel);
        m_primary.Create(L"", box, &m_panel, Primary);
        m_secondary.Create(L"", box, &m_panel, Secondary);
        m_notes.Create(L"", box, &m_panel, ReleaseNotes);
        m_dismiss.Create(L"", box, &m_panel, Dismiss);
        SetWindowSubclass(m_panel.GetSafeHwnd(), PassThroughHitTest, 1, 0);
        m_secondary.SetVariant(NativeMaterial::ButtonVariant::Outlined);
        m_notes.SetVariant(NativeMaterial::ButtonVariant::Text);
        m_dismiss.SetVariant(NativeMaterial::ButtonVariant::Text);
        return TRUE;
    }

    void BannerWindow::Fonts()
    {
        m_dpi = GetDpiForWindow(m_hWnd);
        if (!m_dpi) m_dpi = 96;
        LOGFONT font{};
        NONCLIENTMETRICS metrics{ sizeof(metrics) };
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) font = metrics.lfMessageFont;
        else wcscpy_s(font.lfFaceName, L"Segoe UI");
        font.lfHeight = -Px(14);
        m_font.DeleteObject();
        m_font.CreateFontIndirect(&font);
        font.lfWeight = FW_SEMIBOLD;
        font.lfHeight = -Px(17);
        m_bold.DeleteObject();
        m_bold.CreateFontIndirect(&font);
        m_headline.SetFont(&m_bold);
        m_detail.SetFont(&m_font);
        m_primary.SetFont(&m_font);
        m_secondary.SetFont(&m_font);
        m_notes.SetFont(&m_font);
        m_dismiss.SetFont(&m_font);
    }

    int BannerWindow::TextHeight(const CString& text, int width, CFont& font)
    {
        CClientDC dc(this);
        auto* old = dc.SelectObject(&font);
        CRect measure(0, 0, width, 0);
        dc.DrawText(text, measure, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        dc.SelectObject(old);
        return measure.Height();
    }

    void BannerWindow::Present(const Content& content)
    {
        if (!m_hWnd) return;
        SetWindowText(NativePreferences::Pair(L"TrafficMonitor update", L"TrafficMonitor 更新"));
        const auto appearance = NativePreferences::EffectiveAppearance();
        NativeMaterial::ApplyNativeFrameAppearance(m_hWnd, appearance);
        m_panel.SetAppearance(appearance);
        m_primary.SetAppearance(appearance);
        m_secondary.SetAppearance(appearance);
        m_notes.SetAppearance(appearance);
        m_dismiss.SetAppearance(appearance);
        {
            const auto c = NativeMaterial::Palette::Resolve(appearance).outline;
            m_outline = RGB(BYTE(c.r * 255), BYTE(c.g * 255), BYTE(c.b * 255));
        }
        Fonts();
        m_url = content.url;
        m_headline.SetWindowText(content.headline);
        m_detail.SetWindowText(content.detail);
        m_primary.SetLabel(content.primary);
        m_secondary.SetLabel(content.secondary);
        m_notes.SetLabel(content.notes);
        m_dismiss.SetLabel(L"\u2715");

        const int margin = Px(16), buttonHeight = Px(36), gap = Px(10), clientWidth = Px(560);
        const int textWidth = clientWidth - 2 * margin;
        const int dismissSize = Px(32), edge = (std::max)(1, Px(1));
        const int headlineWidth = textWidth - dismissSize;
        const int headlineHeight = TextHeight(content.headline, headlineWidth, m_bold);
        const int detailHeight = TextHeight(content.detail, textWidth, m_font);
        const int clientHeight = margin + headlineHeight + gap + detailHeight + gap + buttonHeight + margin;

        // No system frame: the client area is the whole window.
        RECT frame{ 0, 0, clientWidth, clientHeight };
        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        const int width = frame.right - frame.left, height = frame.bottom - frame.top;
        SetWindowPos(&wndTopMost, work.right - width - Px(16), work.bottom - height - Px(16), width, height, SWP_NOACTIVATE);

        // A thin outline is drawn by this window around the inset panel; the panel draws its own bottom line.
        m_panel.MoveWindow(edge, edge, clientWidth - 2 * edge, clientHeight - edge);
        m_dismiss.MoveWindow(clientWidth - Px(8) - dismissSize - edge, Px(8) - edge, dismissSize, dismissSize);
        m_headline.MoveWindow(margin - edge, margin - edge, headlineWidth, headlineHeight);
        m_detail.MoveWindow(margin - edge, margin - edge + headlineHeight + gap, textWidth, detailHeight);
        const int top = clientHeight - margin - buttonHeight - edge;
        int right = clientWidth - margin - edge;
        auto place = [&](NativeMaterial::Button& button, const CString& label, int w)
        {
            button.ShowWindow(label.IsEmpty() ? SW_HIDE : SW_SHOWNOACTIVATE);
            if (label.IsEmpty()) return;
            right -= w;
            button.MoveWindow(right, top, w, buttonHeight);
            right -= gap;
        };
        place(m_primary, content.primary, Px(230));
        place(m_secondary, content.secondary, Px(110));
        place(m_notes, content.notes, Px(150));
        m_panel.Invalidate();
        // Never activate: the person may be typing elsewhere. The banner stays until a button or close is used.
        ShowWindow(SW_SHOWNOACTIVATE);
    }

    void BannerWindow::Hide()
    {
        if (m_hWnd) ShowWindow(SW_HIDE);
    }

    BOOL BannerWindow::PreTranslateMessage(MSG* message)
    {
        if (message->message >= WM_KEYFIRST && message->message <= WM_KEYLAST
            && (message->hwnd == m_hWnd || ::IsChild(m_hWnd, message->hwnd))
            && ::IsDialogMessage(m_hWnd, message))
            return TRUE;
        return CWnd::PreTranslateMessage(message);
    }

    BOOL BannerWindow::OnCommand(WPARAM wparam, LPARAM lparam)
    {
        UINT id = LOWORD(wparam);
        if (id == IDCANCEL || id == Dismiss) id = Secondary; // Escape means Later or Dismiss, never a restart.
        if (id == ReleaseNotes)
        {
            if (!m_url.IsEmpty())
                ::ShellExecuteW(m_hWnd, L"open", m_url, nullptr, nullptr, SW_SHOWNORMAL);
            return TRUE;
        }
        if ((id == Primary || id == Secondary) && onCommand)
        {
            onCommand(id);
            return TRUE;
        }
        return CWnd::OnCommand(wparam, lparam);
    }

    LRESULT BannerWindow::WindowProc(UINT message, WPARAM wparam, LPARAM lparam)
    {
        if (message == WM_ERASEBKGND)
        {
            CRect bounds;
            GetClientRect(bounds);
            CDC::FromHandle(reinterpret_cast<HDC>(wparam))->FillSolidRect(bounds, m_outline);
            return TRUE;
        }
        if (message == WM_NCHITTEST)
        {
            const LRESULT hit = CWnd::WindowProc(message, wparam, lparam);
            return hit == HTCLIENT ? HTCAPTION : hit;
        }
        return CWnd::WindowProc(message, wparam, lparam);
    }

    void BannerWindow::OnClose()
    {
        // Closing the notice is the same explicit choice as the secondary button.
        if (onCommand) onCommand(Secondary);
        else Hide();
    }
}
