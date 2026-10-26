#pragma once
#include "Theme.h"
#include <afxstr.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>

// Small Direct2D / DirectWrite painter shared by the Material 3 picker components (color picker, font picker,
// calendar). Every coordinate is a 96-dpi device-independent pixel; render targets carry the window DPI.
namespace NativeMaterial
{
    namespace PickerDraw
    {
        inline D2D1_COLOR_F FromRef(COLORREF color, float alpha = 1.f)
        {
            return D2D1::ColorF(GetRValue(color) / 255.f, GetGValue(color) / 255.f, GetBValue(color) / 255.f, alpha);
        }
        inline COLORREF ToRef(D2D1_COLOR_F c)
        {
            return RGB(BYTE((std::min)(255.f, c.r * 255 + .5f)), BYTE((std::min)(255.f, c.g * 255 + .5f)), BYTE((std::min)(255.f, c.b * 255 + .5f)));
        }
        inline D2D1_RECT_F Inflate(D2D1_RECT_F r, float d) { return D2D1::RectF(r.left - d, r.top - d, r.right + d, r.bottom + d); }
        inline D2D1_POINT_2F Center(D2D1_RECT_F r) { return D2D1::Point2F((r.left + r.right) / 2, (r.top + r.bottom) / 2); }
        inline bool Contains(D2D1_RECT_F r, float x, float y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }

        struct TextStyle
        {
            float size{ 14.f };
            DWRITE_FONT_WEIGHT weight{ DWRITE_FONT_WEIGHT_NORMAL };
            DWRITE_TEXT_ALIGNMENT align{ DWRITE_TEXT_ALIGNMENT_LEADING };
            bool center{ true };            // vertical centering inside the rectangle
            const wchar_t* family{ L"Segoe UI" };
            bool italic{}, underline{}, strike{};
        };

        // Owns one brush and one DirectWrite factory for a single paint.
        struct Painter
        {
            ID2D1RenderTarget* target;
            Palette p;
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
            Microsoft::WRL::ComPtr<IDWriteFactory> text;
            Painter(ID2D1RenderTarget* t, const Palette& palette) : target(t), p(palette)
            {
                t->CreateSolidColorBrush(palette.primary, brush.GetAddressOf());
                DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(text.GetAddressOf()));
            }
            bool Ok() const { return brush && text; }
            ID2D1SolidColorBrush* operator()(D2D1_COLOR_F color) { brush->SetColor(color); return brush.Get(); }
            D2D1_COLOR_F Dim() const
            {
                if (p.highContrast) { const COLORREF g = GetSysColor(COLOR_GRAYTEXT); return FromRef(g); }
                return BlendColor(p.onSurface, p.surface, .38f);
            }
            D2D1_COLOR_F Layer(D2D1_COLOR_F color, float opacity) const { color.a = opacity; return color; }

            void FillRounded(D2D1_RECT_F r, float radius, D2D1_COLOR_F color)
            {
                target->FillRoundedRectangle(D2D1::RoundedRect(r, radius, radius), (*this)(color));
            }
            void StrokeRounded(D2D1_RECT_F r, float radius, D2D1_COLOR_F color, float width)
            {
                const float h = width / 2;
                target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left + h, r.top + h, r.right - h, r.bottom - h), radius, radius), (*this)(color), width);
            }
            void FocusRing(D2D1_RECT_F r, float radius) { StrokeRounded(Inflate(r, 2.f), radius + 2.f, p.primary, 2.f); }

            // Measures the natural single-line width of a string.
            float Width(const CString& s, const TextStyle& style)
            {
                Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
                Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
                if (s.IsEmpty() || FAILED(Format(style, format))) return 0;
                if (FAILED(text->CreateTextLayout(s, s.GetLength(), format.Get(), 4096, 256, layout.GetAddressOf()))) return 0;
                DWRITE_TEXT_METRICS metrics{};
                layout->GetMetrics(&metrics);
                return metrics.widthIncludingTrailingWhitespace;
            }

            void Text(const CString& s, D2D1_RECT_F r, const TextStyle& style, D2D1_COLOR_F color)
            {
                if (s.IsEmpty() || r.right - r.left < 2 || r.bottom - r.top < 2) return;
                Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
                Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
                if (FAILED(Format(style, format))) return;
                format->SetTextAlignment(style.align);
                format->SetParagraphAlignment(style.center ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                Microsoft::WRL::ComPtr<IDWriteInlineObject> sign;
                if (SUCCEEDED(text->CreateEllipsisTrimmingSign(format.Get(), sign.GetAddressOf())))
                {
                    DWRITE_TRIMMING trimming{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                    format->SetTrimming(&trimming, sign.Get());
                }
                if (FAILED(text->CreateTextLayout(s, s.GetLength(), format.Get(), r.right - r.left, r.bottom - r.top, layout.GetAddressOf()))) return;
                const DWRITE_TEXT_RANGE all{ 0, static_cast<UINT32>(s.GetLength()) };
                if (style.underline) layout->SetUnderline(TRUE, all);
                if (style.strike) layout->SetStrikethrough(TRUE, all);
                target->DrawTextLayout(D2D1::Point2F(r.left, r.top), layout.Get(), (*this)(color), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        private:
            HRESULT Format(const TextStyle& style, Microsoft::WRL::ComPtr<IDWriteTextFormat>& format)
            {
                return text->CreateTextFormat(style.family, nullptr, style.weight, style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL, style.size, L"", format.GetAddressOf());
            }
        };
    }
}
