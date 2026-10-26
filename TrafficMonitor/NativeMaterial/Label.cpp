#include "../stdafx.h"
#include "Label.h"
#include "LayoutObservations.h"
#include <cmath>

namespace NativeMaterial
{
    BEGIN_MESSAGE_MAP(Label, CWnd)
        ON_WM_PAINT()
        ON_WM_ERASEBKGND()
        ON_WM_DESTROY()
        ON_MESSAGE(WM_PRINTCLIENT, &Label::OnPrintClient)
        ON_MESSAGE(WM_SETFONT, &Label::OnSetFont)
        ON_MESSAGE(WM_GETFONT, &Label::OnGetFont)
    END_MESSAGE_MAP()

    BOOL Label::Create(LPCWSTR text, DWORD style, const RECT& bounds, CWnd* parent, UINT id)
    {
        const auto className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_ARROW));
        return CWnd::Create(className, text, (style | WS_CHILD) & ~WS_TABSTOP, bounds, parent, id);
    }
    void Label::SetAppearance(Appearance appearance) { m_appearance = appearance; if (m_hWnd) Invalidate(FALSE); }
    void Label::SetRole(LabelRole role) { m_role = role; if (m_hWnd) Invalidate(FALSE); }
    void Label::SetSurfaceColor(COLORREF color) { m_surfaceColor = color; m_customSurface = true; if (m_hWnd) Invalidate(FALSE); }

    LRESULT Label::OnSetFont(WPARAM font, LPARAM redraw)
    {
        m_font = reinterpret_cast<HFONT>(font);
        NativeLayoutObservation::ObserveNativeText(m_hWnd);
        if (redraw && m_hWnd) Invalidate(FALSE);
        return 0;
    }
    LRESULT Label::OnGetFont(WPARAM, LPARAM) { return reinterpret_cast<LRESULT>(m_font); }

    LRESULT Label::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        const auto result = CWnd::WindowProc(message, w, l);
        if (message == WM_SETTEXT)
        {
            if (m_hWnd) Invalidate(FALSE);
            ::NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        }
        return result;
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> Label::Format(IDWriteFactory* factory, float dpi) const
    {
        LOGFONTW face{};
        bool known = m_font && GetObjectW(m_font, sizeof(face), &face) == sizeof(face);
        const wchar_t* name = known && face.lfFaceName[0] ? face.lfFaceName : L"Segoe UI";
        float size = known && face.lfHeight ? std::fabs(static_cast<float>(face.lfHeight)) * 96.f / dpi : 14.f;
        const auto weight = static_cast<DWRITE_FONT_WEIGHT>(known && face.lfWeight > 0 ? face.lfWeight : FW_NORMAL);
        Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
        if (FAILED(factory->CreateTextFormat(name, nullptr, weight, known && face.lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"", format.GetAddressOf()))) return nullptr;
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        return format;
    }

    int Label::MeasureHeight(int widthPx) const
    {
        if (!m_hWnd) return 0;
        UINT dpi = GetDpiForWindow(m_hWnd);
        if (!dpi) dpi = 96;
        Microsoft::WRL::ComPtr<IDWriteFactory> factory;
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf())))) return 0;
        CString text;
        GetWindowText(text);
        auto format = Format(factory.Get(), static_cast<float>(dpi));
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        if (!format || FAILED(factory->CreateTextLayout(text.IsEmpty() ? L" " : text.GetString(), text.IsEmpty() ? 1 : text.GetLength(), format.Get(),
            (std::max)(1.f, widthPx * 96.f / dpi), 4096.f, layout.GetAddressOf()))) return 0;
        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);
        return static_cast<int>(std::ceil(metrics.height * dpi / 96.f));
    }

    void Label::OnPaint()
    {
        CPaintDC paint(this);
        m_drawResult = PaintTo(paint.GetSafeHdc(), false);
    }
    LRESULT Label::OnPrintClient(WPARAM dc, LPARAM flags)
    {
        if (!dc || ((flags & PRF_CHECKVISIBLE) && !IsWindowVisible())) return 0;
        m_drawResult = PaintTo(reinterpret_cast<HDC>(dc), true);
        return 0;
    }
    void Label::OnDestroy()
    {
        NativeLayoutObservation::Unregister(m_hWnd);
        CWnd::OnDestroy();
    }

    HRESULT Label::PaintTo(HDC dc, bool printing)
    {
        NativeLayoutObservation::PaintScope observation(m_hWnd, dc, printing ? NativeLayoutObservation::Route::PrintClient : NativeLayoutObservation::Route::WindowPaint);
        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
        HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
        UINT dpi = GetDpiForWindow(m_hWnd);
        if (!dpi) dpi = 96;
        auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), static_cast<float>(dpi), static_cast<float>(dpi));
        if (SUCCEEDED(result)) result = factory->CreateDCRenderTarget(&properties, target.GetAddressOf());
        CRect bounds;
        GetClientRect(bounds);
        if (SUCCEEDED(result)) result = target->BindDC(dc, bounds);
        if (SUCCEEDED(result))
        {
            const auto palette = Palette::Resolve(m_appearance);
            target->BeginDraw();
            target->Clear(m_customSurface
                ? D2D1::ColorF(GetRValue(m_surfaceColor) / 255.f, GetGValue(m_surfaceColor) / 255.f, GetBValue(m_surfaceColor) / 255.f)
                : palette.surface);
            CString text;
            GetWindowText(text);
            Microsoft::WRL::ComPtr<IDWriteFactory> writer;
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
            if (!text.IsEmpty() && SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(writer.GetAddressOf()))))
            {
                const D2D1_COLOR_F ink = m_role == LabelRole::Error ? palette.error : m_role == LabelRole::Secondary ? palette.onSurfaceVariant : palette.onSurface;
                auto format = Format(writer.Get(), static_cast<float>(dpi));
                Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
                const float width = bounds.Width() * 96.f / dpi, height = bounds.Height() * 96.f / dpi;
                if (format && SUCCEEDED(target->CreateSolidColorBrush(ink, brush.GetAddressOf()))
                    && SUCCEEDED(writer->CreateTextLayout(text, text.GetLength(), format.Get(), (std::max)(1.f, width), (std::max)(1.f, height), layout.GetAddressOf())))
                {
                    const auto origin = D2D1::Point2F(0, 0);
                    target->DrawTextLayout(origin, layout.Get(), brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    NativeLayoutObservation::ObserveDirectWrite(target.Get(), layout.Get(), format.Get(), origin);
                }
            }
            result = target->EndDraw();
        }
        observation.Complete(result);
        return result;
    }
}