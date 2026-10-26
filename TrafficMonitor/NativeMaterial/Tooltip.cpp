#include "../stdafx.h"
#include "Tooltip.h"
#include "TextPainter.h"
#include <commctrl.h>
#include <oleacc.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "uuid.lib")

namespace NativeMaterial
{
    namespace
    {
        enum : UINT_PTR { ShowTimer = 1, FadeTimer = 2, PollTimer = 3 };
        int g_animationOverride = -1;

        // Values from oleacc.h; the import library does not export them.
        const CLSID AccPropServicesClass = { 0xb5f8350b, 0x0548, 0x48b1, { 0xa6, 0xee, 0x88, 0xbd, 0x00, 0xb4, 0xa5, 0xe7 } };
        const MSAAPROPID AccNameProperty = { 0x608d3df8, 0x8128, 0x4aa7, { 0xa4, 0x28, 0xf5, 0x5e, 0x49, 0x26, 0x72, 0x91 } };
        const MSAAPROPID AccDescriptionProperty = { 0x4d48dfe4, 0xbd3f, 0x491f, { 0xa6, 0x48, 0x49, 0x2d, 0x6f, 0x20, 0xc5, 0x88 } };
        const MSAAPROPID AccRoleProperty = { 0xcb905ff2, 0x7bd1, 0x4c05, { 0xb3, 0xc8, 0xe6, 0xc2, 0x41, 0x36, 0x4d, 0x70 } };
        const MSAAPROPID AccHelpProperty = { 0xc831e11f, 0x44db, 0x4a99, { 0x97, 0x68, 0xcb, 0x8f, 0x97, 0x8b, 0x72, 0x31 } };

        Microsoft::WRL::ComPtr<IAccPropServices> Services()
        {
            Microsoft::WRL::ComPtr<IAccPropServices> services;
            CoCreateInstance(AccPropServicesClass, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(services.GetAddressOf()));
            return services;
        }
        void SetText(HWND window, const MSAAPROPID& property, const CString& text)
        {
            if (auto services = Services()) services->SetHwndPropStr(window, OBJID_CLIENT, CHILDID_SELF, property, text.GetString());
        }
        bool AnimationEnabled()
        {
            if (g_animationOverride == 0) return false;
            if (g_animationOverride == 1) return true;
            BOOL enabled = TRUE;
            SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &enabled, 0);
            return enabled != FALSE;
        }
        int Px(float dip, UINT dpi) { return static_cast<int>(std::lround(dip * (dpi ? dpi : 96) / 96.0)); }
        bool NavigationKeyDown()
        {
            for (int key : { VK_TAB, VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_HOME, VK_END, VK_PRIOR, VK_NEXT })
                if (GetKeyState(key) & 0x8000) return true;
            return false;
        }
    }

    // ------------------------------------------------------------ geometry and drawing
    D2D1_SIZE_F MeasureTooltip(const TooltipContent& content)
    {
        using namespace TooltipMetrics;
        TextPainter painter(nullptr);
        if (!content.Rich())
        {
            const auto text = painter.Measure(content.text, 12.f, DWRITE_FONT_WEIGHT_NORMAL, PlainMaxTextWidth, true);
            return D2D1::SizeF(std::ceil(text.width) + 2 * PlainPaddingX, (std::max)(PlainMinHeight, std::ceil(text.height) + 2 * PlainPaddingY));
        }
        const auto title = painter.Measure(content.title, 14.f, DWRITE_FONT_WEIGHT_MEDIUM, RichMaxTextWidth, true);
        const auto text = painter.Measure(content.text, 14.f, DWRITE_FONT_WEIGHT_NORMAL, RichMaxTextWidth, true);
        const float inner = (std::max)(title.width, text.width);
        const float height = std::ceil(title.height) + (content.text.IsEmpty() ? 0.f : 4.f + std::ceil(text.height)) + 2 * RichPaddingY;
        return D2D1::SizeF((std::max)(RichMinWidth, std::ceil(inner) + 2 * RichPaddingX), height);
    }

    D2D1_COLOR_F TooltipFill(const Palette& palette, bool rich) { return rich ? palette.surfaceContainer : palette.onSurface; }

    void DrawTooltip(ID2D1RenderTarget* target, const Palette& palette, const TooltipContent& content)
    {
        using namespace TooltipMetrics;
        TextPainter painter(target);
        if (!painter.Ok()) return;
        const auto size = target->GetSize();
        target->Clear(TooltipFill(palette, content.Rich()));
        if (!content.Rich())
        {
            painter.Draw(content.text, D2D1::RectF(PlainPaddingX, PlainPaddingY, size.width - PlainPaddingX, size.height - PlainPaddingY), 12.f,
                DWRITE_FONT_WEIGHT_NORMAL, palette.surface, DWRITE_TEXT_ALIGNMENT_LEADING, true);
            return;
        }
        target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(.5f, .5f, size.width - .5f, size.height - .5f), RichRadius - .5f, RichRadius - .5f),
            painter.Brush(palette.outlineVariant), 1.f);
        const float inner = size.width - 2 * RichPaddingX;
        const float titleHeight = painter.Measure(content.title, 14.f, DWRITE_FONT_WEIGHT_MEDIUM, inner, true).height;
        painter.Draw(content.title, D2D1::RectF(RichPaddingX, RichPaddingY, size.width - RichPaddingX, RichPaddingY + std::ceil(titleHeight)), 14.f,
            DWRITE_FONT_WEIGHT_MEDIUM, palette.onSurfaceVariant, DWRITE_TEXT_ALIGNMENT_LEADING, true);
        painter.Draw(content.text, D2D1::RectF(RichPaddingX, RichPaddingY + std::ceil(titleHeight) + 4.f, size.width - RichPaddingX, size.height - RichPaddingY), 14.f,
            DWRITE_FONT_WEIGHT_NORMAL, palette.onSurfaceVariant, DWRITE_TEXT_ALIGNMENT_LEADING, true);
    }

    CRect PlaceTooltip(CSize size, CRect avoid, int anchorLeft, CRect workArea, int gap)
    {
        LONG left = (std::max)(workArea.left, (std::min)(static_cast<LONG>(anchorLeft), workArea.right - size.cx));
        LONG top = avoid.bottom + gap;
        if (top + size.cy > workArea.bottom)
        {
            const LONG above = avoid.top - gap - size.cy;
            top = above >= workArea.top ? above : workArea.bottom - size.cy;
        }
        top = (std::max)(workArea.top, top);
        return CRect(left, top, left + size.cx, top + size.cy);
    }

    // ------------------------------------------------------------ popup window
    BEGIN_MESSAGE_MAP(TooltipWindow, CWnd)
        ON_WM_PAINT()
        ON_WM_ERASEBKGND()
        ON_WM_NCHITTEST()
        ON_WM_MOUSEACTIVATE()
        ON_WM_TIMER()
        ON_MESSAGE(WM_PRINTCLIENT, &TooltipWindow::OnPrintClient)
        ON_MESSAGE(WM_PRINT, &TooltipWindow::OnPrintClient)
    END_MESSAGE_MAP()

    TooltipWindow::TooltipWindow() = default;

    BOOL TooltipWindow::CreatePopup(Tooltip* owner)
    {
        m_owner = owner;
        if (m_hWnd) return TRUE;
        const auto className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_ARROW));
        if (!CreateEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED, className, L"", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr)) return FALSE;
        SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
        if (auto services = Services())
        {
            VARIANT role{};
            role.vt = VT_I4; role.lVal = ROLE_SYSTEM_TOOLTIP;
            services->SetHwndProp(m_hWnd, OBJID_CLIENT, CHILDID_SELF, AccRoleProperty, role);
        }
        return TRUE;
    }

    BYTE TooltipWindow::Alpha() const
    {
        BYTE alpha = m_alpha;
        DWORD flags = 0;
        if (m_hWnd && ::GetLayeredWindowAttributes(m_hWnd, nullptr, &alpha, &flags) && (flags & LWA_ALPHA)) return alpha;
        return m_alpha;
    }
    void TooltipWindow::SetAlpha(BYTE alpha)
    {
        m_alpha = alpha;
        if (m_hWnd) SetLayeredWindowAttributes(0, alpha, LWA_ALPHA);
    }

    void TooltipWindow::Present(const TooltipContent& content, UINT dpi, const CRect& bounds, bool animate)
    {
        if (!m_hWnd) return;
        m_content = content;
        m_dpi = dpi ? dpi : 96;
        KillTimer(FadeTimer);
        SetAlpha(animate ? 0 : 255);
        const int radius = Px(content.Rich() ? TooltipMetrics::RichRadius : TooltipMetrics::PlainRadius, m_dpi);
        CRgn region;
        region.CreateRoundRectRgn(0, 0, bounds.Width() + 1, bounds.Height() + 1, 2 * radius, 2 * radius);
        SetWindowRgn(static_cast<HRGN>(region.Detach()), FALSE);
        const CString name = content.Rich() ? content.title + L". " + content.text : content.text;
        SetText(m_hWnd, AccNameProperty, name);
        SetWindowPos(&wndTopMost, bounds.left, bounds.top, bounds.Width(), bounds.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
        Invalidate(FALSE);
        UpdateWindow();
        if (!m_shown) ::NotifyWinEvent(EVENT_OBJECT_SHOW, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        m_shown = true;
        if (animate) SetTimer(FadeTimer, 16, nullptr);
    }
    void TooltipWindow::Dismiss()
    {
        if (!m_hWnd) return;
        KillTimer(FadeTimer);
        KillTimer(PollTimer);
        ShowWindow(SW_HIDE);
        SetAlpha(255);
        if (m_shown) ::NotifyWinEvent(EVENT_OBJECT_HIDE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        m_shown = false;
    }

    HRESULT TooltipWindow::PaintTo(HDC dc)
    {
        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
        HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
        const float dpi = static_cast<float>(m_dpi);
        auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpi, dpi);
        if (SUCCEEDED(result)) result = factory->CreateDCRenderTarget(&properties, target.GetAddressOf());
        CRect bounds;
        GetClientRect(bounds);
        if (SUCCEEDED(result)) result = target->BindDC(dc, bounds);
        if (SUCCEEDED(result))
        {
            target->BeginDraw();
            DrawTooltip(target.Get(), Palette::Resolve(m_appearance), m_content);
            result = target->EndDraw();
        }
        return result;
    }
    void TooltipWindow::OnPaint()
    {
        CPaintDC paint(this);
        PaintTo(paint.GetSafeHdc());
    }
    LRESULT TooltipWindow::OnPrintClient(WPARAM dc, LPARAM)
    {
        if (dc) PaintTo(reinterpret_cast<HDC>(dc));
        return 0;
    }
    void TooltipWindow::OnTimer(UINT_PTR id)
    {
        if (id == FadeTimer)
        {
            const int next = m_alpha + 51;
            SetAlpha(static_cast<BYTE>((std::min)(255, next)));
            if (next >= 255) KillTimer(FadeTimer);
            return;
        }
        if (m_owner) m_owner->OnTimer(id);
    }

    // ------------------------------------------------------------ the manager
    Tooltip::Tooltip() = default;
    Tooltip::~Tooltip()
    {
        RemoveAllTools();
        if (m_popup.m_hWnd) m_popup.DestroyWindow();
    }

    BOOL Tooltip::Create() { return m_popup.CreatePopup(this); }
    void Tooltip::SetAnimationOverride(int mode) { g_animationOverride = mode; }

    void Tooltip::AddTool(CWnd* window, const CString& text) { AddTool(window, CString(), text); }
    void Tooltip::AddTool(CWnd* window, const CString& title, const CString& text)
    {
        if (!window || !window->GetSafeHwnd() || !Create()) return;
        const HWND handle = window->m_hWnd;
        const bool known = m_tools.count(handle) != 0;
        m_tools[handle].content = TooltipContent{ title, text };
        if (!known) SetWindowSubclass(handle, &Tooltip::SubclassProc, reinterpret_cast<UINT_PTR>(this), reinterpret_cast<DWORD_PTR>(this));
        Annotate(handle, text);
    }
    void Tooltip::UpdateTipText(const CString& text, CWnd* window)
    {
        if (!window) return;
        const auto found = m_tools.find(window->GetSafeHwnd());
        if (found == m_tools.end()) return;
        found->second.content.text = text;
        Annotate(found->first, text);
        if (m_shownTool == found->first) ShowFor(window, m_shownByKeyboard);
    }
    void Tooltip::RemoveTool(CWnd* window) { if (window && window->GetSafeHwnd()) Drop(window->m_hWnd, true); }
    void Tooltip::RemoveAllTools()
    {
        std::vector<HWND> all;
        for (const auto& entry : m_tools) all.push_back(entry.first);
        for (HWND handle : all) Drop(handle, IsWindow(handle) != FALSE);
        Hide();
    }
    bool Tooltip::HasTool(const CWnd* window) const { return window && m_tools.count(window->m_hWnd) != 0; }
    CString Tooltip::GetTipText(const CWnd* window) const
    {
        const auto found = window ? m_tools.find(window->m_hWnd) : m_tools.end();
        return found == m_tools.end() ? CString() : found->second.content.text;
    }
    void Tooltip::SetAppearance(Appearance appearance) { m_appearance = appearance; m_popup.SetAppearance(appearance); }
    void Tooltip::Activate(bool active) { m_enabled = active; if (!active) Hide(); }

    void Tooltip::Annotate(HWND window, const CString& help)
    {
        SetText(window, AccHelpProperty, help);
        SetText(window, AccDescriptionProperty, help);
    }
    void Tooltip::Unannotate(HWND window)
    {
        if (!IsWindow(window)) return;
        if (auto services = Services())
        {
            MSAAPROPID ids[] = { AccHelpProperty, AccDescriptionProperty };
            services->ClearHwndProps(window, OBJID_CLIENT, CHILDID_SELF, ids, 2);
        }
    }
    void Tooltip::Drop(HWND window, bool unsubclass)
    {
        if (m_shownTool == window || m_candidate == window) Hide();
        if (m_hoverTool == window) m_hoverTool = nullptr;
        if (m_tools.erase(window) == 0) return;
        if (unsubclass && IsWindow(window))
        {
            RemoveWindowSubclass(window, &Tooltip::SubclassProc, reinterpret_cast<UINT_PTR>(this));
            Unannotate(window);
        }
    }

    void Tooltip::Disarm()
    {
        if (m_popup.m_hWnd) m_popup.KillTimer(ShowTimer);
        m_candidate = nullptr;
    }
    void Tooltip::Hide()
    {
        Disarm();
        m_popup.Dismiss();
        m_shownTool = nullptr;
        m_shownByKeyboard = false;
    }
    void Tooltip::Arm(CWnd* window, bool keyboard)
    {
        Disarm();
        m_candidate = window->m_hWnd;
        m_candidateKeyboard = keyboard;
        if (m_delay == 0) { Disarm(); ShowFor(window, keyboard); return; }
        m_popup.SetTimer(ShowTimer, (std::max)(10u, m_delay), nullptr);
    }

    bool Tooltip::ShowFor(CWnd* window, bool keyboard)
    {
        if (!m_enabled || !window || !window->GetSafeHwnd() || !Create()) return false;
        const HWND handle = window->m_hWnd;
        const auto found = m_tools.find(handle);
        if (found == m_tools.end()) return false;
        const TooltipContent& content = found->second.content;
        if (content.text.IsEmpty() && content.title.IsEmpty()) return false;
        if (!::IsWindowVisible(handle) || !::IsWindowEnabled(handle)) return false;
        UINT dpi = GetDpiForWindow(handle);
        if (!dpi) dpi = 96;
        const auto size = MeasureTooltip(content);
        const CSize physical(Px(size.width, dpi) + 1, Px(size.height, dpi) + 1);
        CRect tool;
        ::GetWindowRect(handle, tool);
        MONITORINFO monitor{ sizeof(monitor) };
        GetMonitorInfoW(MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST), &monitor);
        CRect avoid;
        int anchor;
        if (keyboard)
        {
            avoid = tool;
            anchor = (tool.left + tool.right - physical.cx) / 2;
        }
        else
        {
            avoid = CRect(m_pointer.x, m_pointer.y, m_pointer.x + 1, m_pointer.y + Px(TooltipMetrics::PointerHeight, dpi));
            anchor = m_pointer.x;
        }
        const CRect bounds = PlaceTooltip(physical, avoid, anchor, monitor.rcWork, Px(TooltipMetrics::Gap, dpi));
        const bool wasVisible = m_popup.m_shown;
        m_popup.SetAppearance(m_appearance);
        m_popup.Present(content, dpi, bounds, !wasVisible && AnimationEnabled());
        m_shownTool = handle;
        m_shownByKeyboard = keyboard;
        m_popup.SetTimer(PollTimer, 150, nullptr);
        return true;
    }

    void Tooltip::OnTimer(UINT_PTR id)
    {
        if (id == ShowTimer)
        {
            m_popup.KillTimer(ShowTimer);
            const HWND handle = m_candidate;
            const bool keyboard = m_candidateKeyboard;
            m_candidate = nullptr;
            if (!handle || !IsWindow(handle)) return;
            if (!keyboard)
            {
                CRect tool;
                ::GetWindowRect(handle, tool);
                if (!tool.PtInRect(m_pointer)) return;      // the pointer left without a leave message reaching us
            }
            ShowFor(CWnd::FromHandle(handle), keyboard);
        }
        else if (id == PollTimer)
        {
            if (!m_shownTool) { m_popup.KillTimer(PollTimer); return; }
            if (!IsWindow(m_shownTool) || !::IsWindowVisible(m_shownTool) || !::IsWindowEnabled(m_shownTool)) Hide();
        }
    }

    void Tooltip::NotifyPointer(CWnd* window, CPoint screenPoint)
    {
        if (!m_enabled || !HasTool(window)) return;
        m_pointer = screenPoint;
        m_hoverTool = window->m_hWnd;
        if (m_shownTool == window->m_hWnd) return;           // already showing for this tool
        if (m_shownTool) Hide();
        Arm(window, false);                                 // the pointer must rest for the full delay
    }
    void Tooltip::NotifyPointerLeft(CWnd* window)
    {
        if (!window) return;
        if (m_hoverTool == window->m_hWnd) m_hoverTool = nullptr;
        if (m_candidate == window->m_hWnd && !m_candidateKeyboard) Disarm();
        if (m_shownTool == window->m_hWnd && !m_shownByKeyboard) Hide();
    }
    void Tooltip::NotifyFocus(CWnd* window, bool keyboard)
    {
        if (!m_enabled || !HasTool(window)) return;
        if (!keyboard) { NotifyDismiss(window); return; }    // a click focus never shows a tip
        if (m_shownTool == window->m_hWnd) return;
        if (m_shownTool) Hide();
        Arm(window, true);
    }
    void Tooltip::NotifyFocusLost(CWnd* window)
    {
        if (!window) return;
        if (m_candidate == window->m_hWnd && m_candidateKeyboard) Disarm();
        if (m_shownTool == window->m_hWnd && m_shownByKeyboard) Hide();
    }
    void Tooltip::NotifyDismiss(CWnd* window)
    {
        if (!window) return;
        if (m_candidate == window->m_hWnd) Disarm();
        if (m_shownTool == window->m_hWnd) Hide();
    }

    bool Tooltip::IsVisible() const { return m_popup.m_hWnd && m_popup.m_shown && ::IsWindowVisible(m_popup.m_hWnd); }
    CWnd* Tooltip::ActiveTool() const { return m_shownTool ? CWnd::FromHandle(m_shownTool) : nullptr; }
    CRect Tooltip::Bounds() const
    {
        CRect bounds;
        if (m_popup.m_hWnd) ::GetWindowRect(m_popup.m_hWnd, bounds);
        return bounds;
    }
    BYTE Tooltip::Alpha() const { return m_popup.Alpha(); }
    CString Tooltip::ShownText() const { return IsVisible() ? m_popup.Content().text : CString(); }
    HWND Tooltip::PopupWindow() const { return m_popup.m_hWnd; }

    LRESULT CALLBACK Tooltip::SubclassProc(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR reference)
    {
        auto* self = reinterpret_cast<Tooltip*>(reference);
        CWnd* tool = CWnd::FromHandle(window);
        switch (message)
        {
        case WM_MOUSEMOVE:
        {
            POINT point{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            ::ClientToScreen(window, &point);
            // Leave tracking is requested only while the real pointer is over the tool: asking for it elsewhere makes the
            // operating system post WM_MOUSELEAVE at once, which would cancel every tip.
            POINT cursor{};
            const HWND under = GetCursorPos(&cursor) ? WindowFromPoint(cursor) : nullptr;
            if (under && (under == window || ::IsChild(window, under)))
            {
                TRACKMOUSEEVENT track{ sizeof(track), TME_LEAVE, window, 0 };
                TrackMouseEvent(&track);
            }
            self->NotifyPointer(tool, point);
            break;
        }
        case WM_MOUSELEAVE: self->NotifyPointerLeft(tool); break;
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN: case WM_XBUTTONDOWN:
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL: case WM_KEYDOWN: case WM_SYSKEYDOWN:
            self->NotifyDismiss(tool);
            break;
        case WM_SETFOCUS: self->NotifyFocus(tool, NavigationKeyDown()); break;
        case WM_KILLFOCUS: self->NotifyFocusLost(tool); break;
        case WM_ENABLE: if (!w) self->NotifyDismiss(tool); break;
        case WM_SHOWWINDOW: if (!w) self->NotifyDismiss(tool); break;
        case WM_NCDESTROY:
            self->Drop(window, false);
            RemoveWindowSubclass(window, &Tooltip::SubclassProc, reinterpret_cast<UINT_PTR>(self));
            break;
        default: break;
        }
        return DefSubclassProc(window, message, w, l);
    }
}
