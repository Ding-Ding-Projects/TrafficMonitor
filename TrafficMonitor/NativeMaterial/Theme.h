#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace NativeMaterial
{
    enum class Appearance { Light, Dark };

    struct Palette
    {
        D2D1_COLOR_F surface, container, onSurface, secondaryText;
        D2D1_COLOR_F primary, onPrimary, outline, error;
        bool highContrast{};
        static Palette Resolve(Appearance appearance);
    };

    // UI-thread-owned drawing resources. This class does not own its HWND.
    // Destroy it or call Detach before the associated window is destroyed.
    class Canvas final
    {
    public:
        Canvas() = default;
        Canvas(const Canvas&) = delete;
        Canvas& operator=(const Canvas&) = delete;
        HRESULT Attach(HWND window, float dpi);
        void Detach() noexcept;
        HRESULT Resize(UINT width, UINT height, float dpi);
        HRESULT Begin(const D2D1_COLOR_F& background);
        HRESULT End();
        HRESULT TextFormat(float size, DWRITE_FONT_WEIGHT weight,
            IDWriteTextFormat** format) const;
        ID2D1HwndRenderTarget* Target() const noexcept { return m_target.Get(); }
        ID2D1SolidColorBrush* Brush(const D2D1_COLOR_F& color) noexcept;
        bool UsesSoftwareRendering() const noexcept { return m_software; }

    private:
        HRESULT CreateTarget();
        void DiscardTarget() noexcept;
        HWND m_window{};
        float m_dpi{ 96.0f };
        bool m_drawing{};
        bool m_software{};
        Microsoft::WRL::ComPtr<ID2D1Factory> m_factory;
        Microsoft::WRL::ComPtr<IDWriteFactory> m_textFactory;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_target;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    };
}
