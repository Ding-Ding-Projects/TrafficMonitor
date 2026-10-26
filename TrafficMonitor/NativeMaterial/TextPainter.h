#pragma once
#include "Theme.h"
#include <afxstr.h>
#include <algorithm>

// Small Direct2D / DirectWrite text helper shared by the self-drawn Material 3 components that do not
// need the larger input visual vocabulary (tabs and tooltips). One brush and one shared DirectWrite
// factory live for a single paint. All coordinates are device-independent pixels.
namespace NativeMaterial
{
    class TextPainter
    {
    public:
        explicit TextPainter(ID2D1RenderTarget* target) : m_target(target)
        {
            // A null target gives a measuring-only painter.
            if (target) target->CreateSolidColorBrush(D2D1::ColorF(0), m_brush.GetAddressOf());
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_factory.GetAddressOf()));
        }
        bool Ok() const { return m_target && m_brush && m_factory; }
        ID2D1SolidColorBrush* Brush(D2D1_COLOR_F color) { m_brush->SetColor(color); return m_brush.Get(); }

        // Width of a single line without wrapping.
        float Width(const CString& text, float size, DWRITE_FONT_WEIGHT weight, const wchar_t* face = L"Segoe UI") const
        {
            return Measure(text, size, weight, 4096.f, false, face).width;
        }
        // Size of the text laid out in maxWidth (wrapping when requested).
        D2D1_SIZE_F Measure(const CString& text, float size, DWRITE_FONT_WEIGHT weight, float maxWidth, bool wrap, const wchar_t* face = L"Segoe UI") const
        {
            Microsoft::WRL::ComPtr<IDWriteTextLayout> layout = Layout(text, size, weight, maxWidth, 4096.f, wrap, face);
            if (!layout) return D2D1::SizeF(0, 0);
            DWRITE_TEXT_METRICS metrics{};
            layout->GetMetrics(&metrics);
            return D2D1::SizeF(metrics.widthIncludingTrailingWhitespace, metrics.height);
        }
        // Draws text inside a rectangle. A single line is vertically centered and trimmed with an ellipsis;
        // wrapped text starts at the top.
        void Draw(const CString& text, D2D1_RECT_F rect, float size, DWRITE_FONT_WEIGHT weight, D2D1_COLOR_F color,
            DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING, bool wrap = false, const wchar_t* face = L"Segoe UI")
        {
            if (!Ok() || text.IsEmpty() || rect.right - rect.left < 2 || rect.bottom - rect.top < 2) return;
            auto layout = Layout(text, size, weight, rect.right - rect.left, rect.bottom - rect.top, wrap, face);
            if (!layout) return;
            layout->SetTextAlignment(align);
            layout->SetParagraphAlignment(wrap ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            if (!wrap)
            {
                Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
                Microsoft::WRL::ComPtr<IDWriteInlineObject> sign;
                if (SUCCEEDED(m_factory->CreateTextFormat(face, nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", format.GetAddressOf()))
                    && SUCCEEDED(m_factory->CreateEllipsisTrimmingSign(format.Get(), sign.GetAddressOf())))
                {
                    DWRITE_TRIMMING trimming{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                    layout->SetTrimming(&trimming, sign.Get());
                }
            }
            m_target->DrawTextLayout(D2D1::Point2F(rect.left, rect.top), layout.Get(), Brush(color), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    private:
        Microsoft::WRL::ComPtr<IDWriteTextLayout> Layout(const CString& text, float size, DWRITE_FONT_WEIGHT weight, float width, float height, bool wrap, const wchar_t* face) const
        {
            Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
            Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
            if (!m_factory || text.IsEmpty()) return layout;
            if (FAILED(m_factory->CreateTextFormat(face, nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", format.GetAddressOf()))) return layout;
            format->SetWordWrapping(wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
            m_factory->CreateTextLayout(text, text.GetLength(), format.Get(), (std::max)(1.f, width), (std::max)(1.f, height), layout.GetAddressOf());
            return layout;
        }
        ID2D1RenderTarget* m_target;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
        Microsoft::WRL::ComPtr<IDWriteFactory> m_factory;
    };
}
