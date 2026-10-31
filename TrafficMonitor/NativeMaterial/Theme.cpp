#include "../NativeMaterial/ResponsivenessDiagnostics.h"
#include "Theme.h"
#include <cmath>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace NativeMaterial
{
    namespace
    {
        D2D1_COLOR_F SystemColor(int index)
        {
            const COLORREF value = GetSysColor(index);
            return D2D1::ColorF(GetRValue(value) / 255.0f,
                GetGValue(value) / 255.0f, GetBValue(value) / 255.0f);
        }
    }

    Palette Palette::Resolve(Appearance appearance)
    {
        Palette c{};
        HIGHCONTRASTW contrast{sizeof(contrast)};
        if(SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,0)
            &&(contrast.dwFlags&HCF_HIGHCONTRASTON))
        {
            c.highContrast=true;
            c.surface=c.surfaceContainerLow=c.surfaceContainer=SystemColor(COLOR_WINDOW);
            c.onSurface=c.onSurfaceVariant=c.outline=c.outlineVariant=SystemColor(COLOR_WINDOWTEXT);
            c.primary=c.primaryContainer=c.secondaryContainer=SystemColor(COLOR_HIGHLIGHT);
            c.onPrimary=c.onPrimaryContainer=c.onSecondaryContainer=SystemColor(COLOR_HIGHLIGHTTEXT);
            c.error=SystemColor(COLOR_WINDOWTEXT);c.onError=SystemColor(COLOR_WINDOW);
        }
        else if(appearance==Appearance::Dark)
        {
            // Signal redesign tokens (design/gui-redesign/HANDOFF.md).
            c.surface=D2D1::ColorF(0x0e1116);c.surfaceContainerLow=D2D1::ColorF(0x161b22);c.surfaceContainer=D2D1::ColorF(0x1f2630);
            c.onSurface=D2D1::ColorF(0xe6edf3);c.onSurfaceVariant=D2D1::ColorF(0x9aa7b4);
            c.outline=D2D1::ColorF(0x7c8896);c.outlineVariant=D2D1::ColorF(0x3a4552);
            c.primary=D2D1::ColorF(0x8ec0ff);c.onPrimary=D2D1::ColorF(0x0a2a55);
            c.primaryContainer=D2D1::ColorF(0x22324a);c.onPrimaryContainer=D2D1::ColorF(0xdce9ff);
            c.secondaryContainer=D2D1::ColorF(0x2a3442);c.onSecondaryContainer=D2D1::ColorF(0xe6edf3);
            c.error=D2D1::ColorF(0xff9aa2);c.onError=D2D1::ColorF(0x4a0710);
        }
        else
        {
            // Signal redesign tokens (design/gui-redesign/HANDOFF.md).
            c.surface=D2D1::ColorF(0xf4f6f8);c.surfaceContainerLow=D2D1::ColorF(0xffffff);c.surfaceContainer=D2D1::ColorF(0xebeff3);
            c.onSurface=D2D1::ColorF(0x111820);c.onSurfaceVariant=D2D1::ColorF(0x46525f);
            c.outline=D2D1::ColorF(0x7a8693);c.outlineVariant=D2D1::ColorF(0xc9d2dc);
            c.primary=D2D1::ColorF(0x1d5fd1);c.onPrimary=D2D1::ColorF(0xffffff);
            c.primaryContainer=D2D1::ColorF(0xd6e3fa);c.onPrimaryContainer=D2D1::ColorF(0x0f2f66);
            c.secondaryContainer=D2D1::ColorF(0xe3ecfb);c.onSecondaryContainer=D2D1::ColorF(0x111820);
            c.error=D2D1::ColorF(0xa3262f);c.onError=D2D1::ColorF(0xffffff);
        }
        // Existing owners keep their field names while shared components use
        // explicit semantic roles. No owner-local palette is introduced.
        c.container=c.surfaceContainerLow;c.secondaryText=c.onSurfaceVariant;
        return c;
    }
    HRESULT ApplyNativeFrameAppearance(HWND window, Appearance appearance)
    {
        NativeDiagnostics::Scope diagnostic(NativeDiagnostics::Operation::FrameAppearance);
        if (!IsWindow(window)) return E_INVALIDARG;
        const auto palette = Palette::Resolve(appearance);
        const BOOL dark = !palette.highContrast && appearance == Appearance::Dark;
        // Attributes 20 and 34..36 are supported on modern Windows. Older hosts
        // reject unsupported attributes and continue using their native frame.
        const HRESULT modeResult = DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
        const auto rgb = [](D2D1_COLOR_F value) {
            return RGB(BYTE(value.r * 255), BYTE(value.g * 255), BYTE(value.b * 255));
        };
        const COLORREF defaultColor = 0xffffffff;
        const COLORREF caption = palette.highContrast ? defaultColor : rgb(palette.surface);
        const COLORREF text = palette.highContrast ? defaultColor : rgb(palette.onSurface);
        const COLORREF border = palette.highContrast ? defaultColor : rgb(palette.outline);
        DwmSetWindowAttribute(window, 35, &caption, sizeof(caption));
        DwmSetWindowAttribute(window, 36, &text, sizeof(text));
        DwmSetWindowAttribute(window, 34, &border, sizeof(border));
        RedrawWindow(window, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
        return modeResult;
    }

    HRESULT Canvas::Attach(HWND window, float dpi)
    {
        Detach();
        if (!IsWindow(window) || !std::isfinite(dpi) || dpi <= 0) return E_INVALIDARG;
        m_window = window;
        m_dpi = dpi;
        HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_factory.ReleaseAndGetAddressOf());
        if (SUCCEEDED(result))
            result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(m_textFactory.ReleaseAndGetAddressOf()));
        if (SUCCEEDED(result)) result = CreateTarget();
        if (FAILED(result)) Detach();
        return result;
    }

    void Canvas::DiscardTarget() noexcept
    {
        m_brush.Reset();
        m_target.Reset();
        m_drawing = false;
    }

    void Canvas::Detach() noexcept
    {
        DiscardTarget();
        m_textFactory.Reset();
        m_factory.Reset();
        m_window = nullptr;
        m_software = false;
    }

    HRESULT Canvas::CreateTarget()
    {
        if (!m_factory || !IsWindow(m_window)) return E_UNEXPECTED;
        RECT client{};
        if (!GetClientRect(m_window, &client)) return HRESULT_FROM_WIN32(GetLastError());
        auto properties = D2D1::RenderTargetProperties();
        properties.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
        properties.dpiX = properties.dpiY = m_dpi;
        const auto windowProperties = D2D1::HwndRenderTargetProperties(m_window,
            D2D1::SizeU(static_cast<UINT>(client.right), static_cast<UINT>(client.bottom)));
        m_software = false;
        HRESULT result = m_factory->CreateHwndRenderTarget(properties, windowProperties,
            m_target.ReleaseAndGetAddressOf());
        if (FAILED(result))
        {
            properties.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;
            result = m_factory->CreateHwndRenderTarget(properties, windowProperties,
                m_target.ReleaseAndGetAddressOf());
            m_software = SUCCEEDED(result);
        }
        if (SUCCEEDED(result)) result = m_target->CreateSolidColorBrush(
            D2D1::ColorF(0x000000), m_brush.ReleaseAndGetAddressOf());
        if (FAILED(result)) DiscardTarget();
        return result;
    }

    HRESULT Canvas::Resize(UINT width, UINT height, float dpi)
    {
        if (!std::isfinite(dpi) || dpi <= 0 || m_drawing) return E_INVALIDARG;
        m_dpi = dpi;
        if (!m_target) return S_OK;
        m_target->SetDpi(dpi, dpi);
        HRESULT result = m_target->Resize(D2D1::SizeU(width, height));
        if (FAILED(result)) DiscardTarget();
        return result;
    }

    HRESULT Canvas::Begin(const D2D1_COLOR_F& background)
    {
        if (m_drawing) return E_UNEXPECTED;
        HRESULT result = m_target ? S_OK : CreateTarget();
        if (FAILED(result)) return result;
        m_target->BeginDraw();
        m_target->Clear(background);
        m_drawing = true;
        return S_OK;
    }

    HRESULT Canvas::End()
    {
        if (!m_drawing || !m_target) return E_UNEXPECTED;
        m_drawing = false;
        HRESULT result = m_target->EndDraw();
        // The next paint recreates resources. The caller must schedule that paint.
        if (FAILED(result)) DiscardTarget();
        return result;
    }

    ID2D1SolidColorBrush* Canvas::Brush(const D2D1_COLOR_F& color) noexcept
    {
        if (!m_brush) return nullptr;
        m_brush->SetColor(color);
        return m_brush.Get();
    }

    HRESULT Canvas::TextFormat(float size, DWRITE_FONT_WEIGHT weight,
        IDWriteTextFormat** format) const
    {
        if (!format) return E_POINTER;
        *format = nullptr;
        if (!m_textFactory || !std::isfinite(size) || size <= 0) return E_INVALIDARG;
        // DirectWrite supplies system font fallback for scripts absent from this face.
        return m_textFactory->CreateTextFormat(L"Segoe UI", nullptr, weight,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", format);
    }
}
