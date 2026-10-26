#include "../stdafx.h"
#include "TitleBar.h"
#include "TitleBar.Accessibility.h"
#include <CommCtrl.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <algorithm>
#pragma comment(lib, "uiautomationcore.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

namespace NativeMaterial
{
    namespace
    {
        constexpr wchar_t BarClass[] = L"TrafficMonitorMaterialTitleBar";
        constexpr UINT_PTR SubclassId = 0x7B17;

        D2D1_RECT_F ToDip(const RECT& r, float scale)
        { return D2D1::RectF(r.left * scale, r.top * scale, r.right * scale, r.bottom * scale); }
    }

    TitleBar::Geometry TitleBar::Compute(int width, UINT dpi, bool icon, bool minimize, bool maximize, bool close)
    {
        if (!dpi) dpi = 96;
        const auto px = [&](int dp) { return MulDiv(dp, static_cast<int>(dpi), 96); };
        Geometry g;
        const int height = px(HeightDp), cell = px(ButtonWidthDp);
        g.bar = CRect(0, 0, (std::max)(0, width), height);
        int right = g.bar.right;
        if (close) { g.hasClose = true; g.close = CRect(right - cell, 0, right, height); right -= cell; }
        if (maximize) { g.hasMaximize = true; g.maximize = CRect(right - cell, 0, right, height); right -= cell; }
        if (minimize) { g.hasMinimize = true; g.minimize = CRect(right - cell, 0, right, height); right -= cell; }
        int left = px(12);
        if (icon)
        {
            const int size = px(16);
            g.hasIcon = true;
            g.icon = CRect(left, (height - size) / 2, left + size, (height - size) / 2 + size);
            left += size + px(12);
        }
        else left = px(16);
        const int textRight = (std::max)(left, right - px(8));
        g.text = CRect(left, 0, textRight, height);
        return g;
    }

    struct TitleBar::Impl
    {
        HWND host{}, bar{};
        UINT dpi{ 96 };
        Appearance appearance{ Appearance::Light };
        Labels labels;
        Part hot{ Part::None }, pressed{ Part::None };
        bool tracking{}, active{ true };
        std::shared_ptr<TitleBarAccess> access{ std::make_shared<TitleBarAccess>() };

        // ---- state -------------------------------------------------------------------
        bool Has(DWORD box) const
        { return (GetWindowLongW(host, GWL_STYLE) & (box | WS_SYSMENU)) == (box | WS_SYSMENU); }
        bool HasMinimize() const { return Has(WS_MINIMIZEBOX); }
        bool HasMaximize() const { return Has(WS_MAXIMIZEBOX); }
        bool HasClose() const { return (GetWindowLongW(host, GWL_STYLE) & WS_SYSMENU) != 0; }
        HICON Icon() const
        {
            DWORD_PTR icon = 0;
            if (SendMessageTimeoutW(host, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 50, &icon) && icon) return reinterpret_cast<HICON>(icon);
            if (SendMessageTimeoutW(host, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 50, &icon) && icon) return reinterpret_cast<HICON>(icon);
            return reinterpret_cast<HICON>(GetClassLongPtrW(host, GCLP_HICONSM));
        }
        CString Title() const
        {
            const int length = GetWindowTextLengthW(host);
            CString text;
            if (length > 0) { GetWindowTextW(host, text.GetBuffer(length + 1), length + 1); text.ReleaseBuffer(); }
            return text;
        }
        int Metric(int index) const { return GetSystemMetricsForDpi(index, dpi); }
        int FrameX() const { return Metric(SM_CXSIZEFRAME) + Metric(SM_CXPADDEDBORDER); }
        int FrameY() const { return Metric(SM_CYSIZEFRAME) + Metric(SM_CXPADDEDBORDER); }
        int HeightPx() const { return MulDiv(HeightDp, static_cast<int>(dpi), 96); }

        Geometry Measure() const
        {
            RECT client{};
            GetClientRect(host, &client);
            return Compute(client.right, dpi, Icon() != nullptr, HasMinimize(), HasMaximize(), HasClose());
        }

        static Part PartFromHit(WPARAM hit)
        {
            switch (hit)
            {
            case HTMINBUTTON: return Part::Minimize;
            case HTMAXBUTTON: return Part::Maximize;
            case HTCLOSE: return Part::Close;
            case HTCAPTION: return Part::Caption;
            default: return Part::None;
            }
        }

        // Window-relative hit test in screen coordinates. Everything the title bar does not
        // own keeps the native result.
        LRESULT HitTest(HWND hwnd, POINT screen, WNDPROC fallback) const
        {
            RECT window{};
            GetWindowRect(hwnd, &window);
            if (!PtInRect(&window, screen)) return fallback ? CallWindowProcW(fallback, hwnd, WM_NCHITTEST, 0, MAKELPARAM(screen.x, screen.y)) : HTNOWHERE;
            const bool zoomed = IsZoomed(hwnd) != FALSE;
            const DWORD style = static_cast<DWORD>(GetWindowLongW(hwnd, GWL_STYLE));
            const int x = screen.x - window.left, y = screen.y - window.top;
            const int width = window.right - window.left, height = window.bottom - window.top;
            if (!zoomed && (style & WS_THICKFRAME))
            {
                // Side and bottom borders are the real non-client borders Windows kept.
                RECT inner{};
                GetClientRect(hwnd, &inner);
                POINT innerOrigin{ 0, 0 };
                ClientToScreen(hwnd, &innerOrigin);
                const int borderLeft = (std::max)(1, static_cast<int>(innerOrigin.x - window.left));
                const int borderRight = (std::max)(1, static_cast<int>(window.right - (innerOrigin.x + inner.right)));
                const int borderBottom = (std::max)(1, static_cast<int>(window.bottom - (innerOrigin.y + inner.bottom)));
                const int top = Metric(SM_CYSIZEFRAME), corner = (std::max)(borderLeft, borderRight);
                const bool left = x < borderLeft, right = x >= width - borderRight, bottom = y >= height - borderBottom;
                const bool topEdge = y < top;
                if ((left || right) && y < corner) return left ? HTTOPLEFT : HTTOPRIGHT;
                if ((left || right) && y >= height - corner) return left ? HTBOTTOMLEFT : HTBOTTOMRIGHT;
                if (topEdge) return HTTOP;
                if (left) return HTLEFT;
                if (right) return HTRIGHT;
                if (bottom) return HTBOTTOM;
            }
            POINT client = screen;
            ScreenToClient(hwnd, &client);
            const Geometry g = Measure();
            if (client.y < 0 || client.y >= g.bar.bottom) return HTCLIENT;
            if (g.hasMinimize && client.x >= g.minimize.left && client.x < g.minimize.right) return HTMINBUTTON;
            if (g.hasMaximize && client.x >= g.maximize.left && client.x < g.maximize.right) return HTMAXBUTTON;
            if (g.hasClose && client.x >= g.close.left && client.x < g.close.right) return HTCLOSE;
            return HTCAPTION;
        }

        // ---- synchronization -----------------------------------------------------------
        void UpdateAccess()
        {
            const Geometry g = Measure();
            const bool zoomed = IsZoomed(host) != FALSE;
            std::lock_guard<std::mutex> lock(access->mutex);
            access->host = host;
            access->bar = bar;
            access->title = Title().GetString();
            const bool present[3] = { g.hasMinimize, g.hasMaximize, g.hasClose };
            const CRect* cells[3] = { &g.minimize, &g.maximize, &g.close };
            const CString* names[3] = { &labels.minimize, zoomed ? &labels.restore : &labels.maximize, &labels.close };
            for (int i = 0; i < 3; ++i)
            {
                access->present[i] = present[i];
                access->names[i] = names[i]->GetString();
                access->cells[i] = *cells[i];
            }
        }
        void Refresh()
        {
            if (!bar || !IsWindow(bar)) return;
            UpdateAccess();
            InvalidateRect(bar, nullptr, FALSE);
        }
        void Reposition()
        {
            if (!bar || !IsWindow(host)) return;
            RECT client{};
            GetClientRect(host, &client);
            SetWindowPos(bar, HWND_TOP, 0, 0, client.right, HeightPx(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
            Refresh();
        }
        // The owner's own WM_CONTEXTMENU handling must not hide the system menu on the caption.
        void ShowSystemMenu(HWND hwnd, POINT screen)
        {
            HMENU menu = GetSystemMenu(hwnd, FALSE);
            if (!menu) return;
            const DWORD style = static_cast<DWORD>(GetWindowLongW(hwnd, GWL_STYLE));
            const bool zoomed = IsZoomed(hwnd) != FALSE;
            EnableMenuItem(menu, SC_RESTORE, zoomed ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, SC_MOVE, zoomed ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(menu, SC_SIZE, (zoomed || !(style & WS_THICKFRAME)) ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(menu, SC_MAXIMIZE, (zoomed || !(style & WS_MAXIMIZEBOX)) ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(menu, SC_MINIMIZE, (style & WS_MINIMIZEBOX) ? MF_ENABLED : MF_GRAYED);
            SetForegroundWindow(hwnd);
            const UINT command = static_cast<UINT>(TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen.x, screen.y, 0, hwnd, nullptr));
            if (command) PostMessageW(hwnd, WM_SYSCOMMAND, command, 0);
        }
        void Command(Part part)
        {
            WPARAM command = SC_CLOSE;
            if (part == Part::Minimize) command = SC_MINIMIZE;
            else if (part == Part::Maximize) command = IsZoomed(host) ? SC_RESTORE : SC_MAXIMIZE;
            PostMessageW(host, WM_SYSCOMMAND, command, 0);
        }

        // ---- window procedures -----------------------------------------------------------
        static LRESULT CALLBACK HostProc(HWND hwnd, UINT message, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR ref)
        {
            auto* self = reinterpret_cast<Impl*>(ref);
            switch (message)
            {
            case WM_NCCALCSIZE:
            {
                // Let Windows compute every border, then give the caption back to the client.
                RECT* area = w ? &reinterpret_cast<NCCALCSIZE_PARAMS*>(l)->rgrc[0] : reinterpret_cast<RECT*>(l);
                const LONG top = area->top;
                const LRESULT result = DefSubclassProc(hwnd, message, w, l);
                if (!IsIconic(hwnd)) area->top = top + (IsZoomed(hwnd) ? self->FrameY() : 0);
                return result;
            }
            case WM_NCHITTEST:
            {
                const POINT screen{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
                const LRESULT native = DefSubclassProc(hwnd, message, w, l);
                // Keep non-client results that do not belong to the removed caption.
                if (native == HTNOWHERE || native == HTERROR || native == HTTRANSPARENT) return native;
                return self->HitTest(hwnd, screen, nullptr);
            }
            case WM_NCMOUSEMOVE:
            {
                const Part part = PartFromHit(w);
                if (part != self->hot) { self->hot = part; self->Refresh(); }
                if (!self->tracking)
                {
                    TRACKMOUSEEVENT track{ sizeof(track), TME_LEAVE | TME_NONCLIENT, hwnd, 0 };
                    self->tracking = TrackMouseEvent(&track) != FALSE;
                }
                break;
            }
            case WM_NCMOUSELEAVE:
                self->tracking = false;
                self->hot = self->pressed = Part::None;
                self->Refresh();
                break;
            case WM_NCLBUTTONDOWN:
            {
                const Part part = PartFromHit(w);
                if (part == Part::Minimize || part == Part::Maximize || part == Part::Close)
                {
                    // Handled here so Windows does not run its own caption button loop.
                    self->pressed = self->hot = part;
                    self->Refresh();
                    return 0;
                }
                break;
            }
            case WM_NCLBUTTONUP:
                if (self->pressed != Part::None)
                {
                    const Part pressed = self->pressed;
                    self->pressed = Part::None;
                    self->Refresh();
                    if (PartFromHit(w) == pressed) self->Command(pressed);
                    return 0;
                }
                break;
            case WM_NCLBUTTONDBLCLK:
            {
                const Part part = PartFromHit(w);
                if (part == Part::Caption)
                {
                    if (self->HasMaximize()) self->Command(Part::Maximize);
                    return 0;
                }
                if (part == Part::Minimize || part == Part::Maximize || part == Part::Close) return 0;
                break;
            }
            case WM_NCRBUTTONUP:
                if (w == HTCAPTION)
                {
                    self->ShowSystemMenu(hwnd, POINT{ GET_X_LPARAM(l), GET_Y_LPARAM(l) });
                    return 0;
                }
                break;
            case WM_NCACTIVATE:
                self->active = w != FALSE;
                self->Refresh();
                break;
            case WM_DPICHANGED:
                if (HIWORD(w)) self->dpi = HIWORD(w);
                break;
            case WM_NCDESTROY:
                self->Detach(hwnd, true);
                return DefSubclassProc(hwnd, message, w, l);
            default:
                break;
            }
            const LRESULT result = DefSubclassProc(hwnd, message, w, l);
            switch (message)
            {
            case WM_SIZE:
            case WM_DPICHANGED:
            case WM_WINDOWPOSCHANGED:
            case WM_STYLECHANGED:
                self->Reposition();
                break;
            case WM_SETTEXT:
            case WM_SETICON:
                self->Refresh();
                break;
            default:
                break;
            }
            return result;
        }

        void Detach(HWND hwnd, bool destroying)
        {
            if (hwnd) RemoveWindowSubclass(hwnd, HostProc, SubclassId);
            {
                std::lock_guard<std::mutex> lock(access->mutex);
                access->host = nullptr;
                access->bar = nullptr;
            }
            if (bar && !destroying && IsWindow(bar)) DestroyWindow(bar);
            bar = nullptr;
            host = nullptr;
        }

        static LRESULT CALLBACK BarProc(HWND hwnd, UINT message, WPARAM w, LPARAM l)
        {
            Impl* self = nullptr;
            if (message == WM_NCCREATE)
            {
                self = static_cast<Impl*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            else self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!self) return DefWindowProcW(hwnd, message, w, l);
            switch (message)
            {
            case WM_NCHITTEST: return HTTRANSPARENT;
            case WM_ERASEBKGND: return 1;
            case WM_PAINT:
            {
                PAINTSTRUCT paint{};
                HDC dc = BeginPaint(hwnd, &paint);
                if (dc) self->Draw(dc);
                EndPaint(hwnd, &paint);
                return 0;
            }
            case WM_PRINTCLIENT:
                if (w) self->Draw(reinterpret_cast<HDC>(w));
                return 0;
            case WM_GETOBJECT:
                if (static_cast<LONG>(l) == static_cast<LONG>(UiaRootObjectId))
                {
                    auto* provider = new TitleBarProvider(self->access, -1);
                    const LRESULT result = UiaReturnRawElementProvider(hwnd, w, l, provider);
                    provider->Release();
                    return result;
                }
                break;
            case WM_NCDESTROY:
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                break;
            default:
                break;
            }
            return DefWindowProcW(hwnd, message, w, l);
        }

        void Draw(HDC dc)
        {
            RECT client{};
            GetClientRect(bar, &client);
            Microsoft::WRL::ComPtr<ID2D1Factory> factory;
            Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
            if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf()))) return;
            const float dpiValue = static_cast<float>(dpi);
            const auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiValue, dpiValue);
            if (FAILED(factory->CreateDCRenderTarget(&properties, target.GetAddressOf())) || FAILED(target->BindDC(dc, &client))) return;

            const Palette c = Palette::Resolve(appearance);
            const Geometry g = Measure();
            const float scale = 96.f / dpiValue;
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
            target->BeginDraw();
            target->Clear(c.surfaceContainerLow);
            if (FAILED(target->CreateSolidColorBrush(c.onSurface, brush.GetAddressOf()))) { target->EndDraw(); return; }
            const auto color = [&](D2D1_COLOR_F value) { brush->SetColor(value); return brush.Get(); };

            const auto size = target->GetSize();
            target->DrawLine(D2D1::Point2F(0, size.height - .5f), D2D1::Point2F(size.width, size.height - .5f),
                color(c.highContrast ? c.outline : c.outlineVariant), 1.f);

            const D2D1_COLOR_F ink = active ? c.onSurface : BlendColor(c.onSurface, c.surfaceContainerLow, .6f);
            const bool zoomed = IsZoomed(host) != FALSE;
            struct Cell { Part part; const CRect* rect; bool present; };
            const Cell cells[3] = { { Part::Minimize, &g.minimize, g.hasMinimize }, { Part::Maximize, &g.maximize, g.hasMaximize }, { Part::Close, &g.close, g.hasClose } };
            for (const Cell& cell : cells)
            {
                if (!cell.present) continue;
                const bool isHot = hot == cell.part, isPressed = pressed == cell.part;
                const bool close = cell.part == Part::Close;
                D2D1_COLOR_F glyph = ink;
                const auto rect = ToDip(*cell.rect, scale);
                const auto layer = D2D1::RoundedRect(D2D1::RectF(rect.left + 4, rect.top + 4, rect.right - 4, rect.bottom - 4), 8, 8);
                if (isHot || isPressed)
                {
                    if (c.highContrast) { target->FillRoundedRectangle(layer, color(c.primary)); glyph = c.onPrimary; }
                    else if (close) { target->FillRoundedRectangle(layer, color(isPressed ? BlendColor(c.error, c.onError, .88f) : c.error)); glyph = c.onError; }
                    else { auto state = c.onSurface; state.a = isPressed ? .12f : .08f; target->FillRoundedRectangle(layer, color(state)); }
                }
                const float cx = (rect.left + rect.right) / 2, cy = (rect.top + rect.bottom) / 2;
                auto* stroke = color(glyph);
                if (cell.part == Part::Minimize)
                    target->DrawLine(D2D1::Point2F(cx - 5, cy), D2D1::Point2F(cx + 5, cy), stroke, 1.5f);
                else if (cell.part == Part::Maximize && !zoomed)
                    target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(cx - 5, cy - 5, cx + 5, cy + 5), 1.5f, 1.5f), stroke, 1.5f);
                else if (cell.part == Part::Maximize)
                {
                    target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(cx - 5, cy - 2, cx + 3, cy + 6), 1.5f, 1.5f), stroke, 1.5f);
                    target->DrawLine(D2D1::Point2F(cx - 2, cy - 2), D2D1::Point2F(cx - 2, cy - 5), stroke, 1.5f);
                    target->DrawLine(D2D1::Point2F(cx - 2, cy - 5), D2D1::Point2F(cx + 6, cy - 5), stroke, 1.5f);
                    target->DrawLine(D2D1::Point2F(cx + 6, cy - 5), D2D1::Point2F(cx + 6, cy + 3), stroke, 1.5f);
                    target->DrawLine(D2D1::Point2F(cx + 6, cy + 3), D2D1::Point2F(cx + 3, cy + 3), stroke, 1.5f);
                }
                else
                {
                    target->DrawLine(D2D1::Point2F(cx - 5, cy - 5), D2D1::Point2F(cx + 5, cy + 5), stroke, 1.5f);
                    target->DrawLine(D2D1::Point2F(cx - 5, cy + 5), D2D1::Point2F(cx + 5, cy - 5), stroke, 1.5f);
                }
            }

            // Window title: the same face, weight and size the Material buttons use.
            const CString title = Title();
            const auto textArea = ToDip(g.text, scale);
            Microsoft::WRL::ComPtr<IDWriteFactory> text;
            Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
            if (textArea.right > textArea.left && !title.IsEmpty() &&
                SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(text.GetAddressOf()))) &&
                SUCCEEDED(text->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14, L"", format.GetAddressOf())))
            {
                format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                Microsoft::WRL::ComPtr<IDWriteInlineObject> ellipsis;
                if (SUCCEEDED(text->CreateEllipsisTrimmingSign(format.Get(), ellipsis.GetAddressOf())))
                {
                    const DWRITE_TRIMMING trimming{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                    format->SetTrimming(&trimming, ellipsis.Get());
                }
                Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
                if (SUCCEEDED(text->CreateTextLayout(title, title.GetLength(), format.Get(), textArea.right - textArea.left, textArea.bottom - textArea.top, layout.GetAddressOf())))
                    target->DrawTextLayout(D2D1::Point2F(textArea.left, textArea.top), layout.Get(), color(ink));
            }
            const HRESULT result = target->EndDraw();
            if (SUCCEEDED(result) && g.hasIcon)
                if (HICON icon = Icon()) DrawIconEx(dc, g.icon.left, g.icon.top, icon, g.icon.Width(), g.icon.Height(), 0, nullptr, DI_NORMAL);
        }
    };

    TitleBar::TitleBar() = default;
    TitleBar::~TitleBar() { Detach(); }
    bool TitleBar::IsAttached() const noexcept { return m_impl && m_impl->host && IsWindow(m_impl->host); }
    HWND TitleBar::BarWindow() const { return m_impl ? m_impl->bar : nullptr; }

    HRESULT TitleBar::Attach(CWnd* window, Appearance appearance, const Labels& labels)
    {
        if (!window || !IsWindow(window->GetSafeHwnd())) return E_INVALIDARG;
        if (m_impl) return E_UNEXPECTED;
        HWND hwnd = window->GetSafeHwnd();
        auto impl = std::make_unique<Impl>();
        impl->host = hwnd;
        impl->appearance = appearance;
        impl->labels = labels;
        impl->dpi = GetDpiForWindow(hwnd);
        if (!impl->dpi) impl->dpi = 96;
        impl->active = GetActiveWindow() == hwnd || !IsWindowVisible(hwnd);

        WNDCLASSEXW barClass{ sizeof(barClass) };
        if (!GetClassInfoExW(GetModuleHandleW(nullptr), BarClass, &barClass))
        {
            barClass = { sizeof(barClass) };
            barClass.lpfnWndProc = Impl::BarProc;
            barClass.hInstance = GetModuleHandleW(nullptr);
            barClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            barClass.lpszClassName = BarClass;
            if (!RegisterClassExW(&barClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return HRESULT_FROM_WIN32(GetLastError());
        }
        RECT client{};
        GetClientRect(hwnd, &client);
        impl->bar = CreateWindowExW(0, BarClass, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, client.right, impl->HeightPx(),
            hwnd, nullptr, GetModuleHandleW(nullptr), impl.get());
        if (!impl->bar) return HRESULT_FROM_WIN32(GetLastError());
        if (!SetWindowSubclass(hwnd, Impl::HostProc, SubclassId, reinterpret_cast<DWORD_PTR>(impl.get())))
        {
            DestroyWindow(impl->bar);
            return E_FAIL;
        }
        m_impl = std::move(impl);
        // A one pixel frame extension keeps the DWM shadow and rounded corners on a client-only window.
        const MARGINS margins{ 0, 0, 1, 0 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        m_impl->Reposition();
        return S_OK;
    }

    void TitleBar::Detach()
    {
        if (!m_impl) return;
        m_impl->Detach(m_impl->host && IsWindow(m_impl->host) ? m_impl->host : nullptr, false);
        m_impl.reset();
    }

    void TitleBar::SetAppearance(Appearance appearance)
    {
        if (!m_impl) return;
        m_impl->appearance = appearance;
        m_impl->Refresh();
    }

    void TitleBar::SetLabels(const Labels& labels)
    {
        if (!m_impl) return;
        m_impl->labels = labels;
        m_impl->Refresh();
    }

    int TitleBar::HeightPx() const { return m_impl ? m_impl->HeightPx() : 0; }
    TitleBar::Geometry TitleBar::Measure() const { return m_impl && m_impl->host ? m_impl->Measure() : Geometry{}; }
    TitleBar::Part TitleBar::PartAt(POINT screen) const
    {
        if (!IsAttached()) return Part::None;
        return Impl::PartFromHit(m_impl->HitTest(m_impl->host, screen, nullptr));
    }

    SIZE TitleBar::OuterSizeForContent(int contentWidthPx, int contentHeightPx) const
    {
        if (!IsAttached()) return { contentWidthPx, contentHeightPx };
        RECT window{}, client{};
        GetWindowRect(m_impl->host, &window);
        GetClientRect(m_impl->host, &client);
        return { (window.right - window.left) - client.right + contentWidthPx,
            (window.bottom - window.top) - client.bottom + contentHeightPx + m_impl->HeightPx() };
    }
}
