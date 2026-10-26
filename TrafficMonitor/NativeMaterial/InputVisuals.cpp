#include "../stdafx.h"
#include "InputVisuals.h"
#include "LayoutObservations.h"
#include <algorithm>

namespace NativeMaterial
{
    using namespace InputMetrics;
    namespace
    {
        using Microsoft::WRL::ComPtr;

        // Owns one brush and one DirectWrite factory for a single paint.
        struct Ink
        {
            ID2D1RenderTarget* target;
            const Palette& p;
            ComPtr<ID2D1SolidColorBrush> brush;
            ComPtr<IDWriteFactory> text;
            Ink(ID2D1RenderTarget* t, const Palette& palette) : target(t), p(palette)
            {
                t->CreateSolidColorBrush(palette.primary, brush.GetAddressOf());
                DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(text.GetAddressOf()));
            }
            bool Ok() const { return brush && text; }
            ID2D1SolidColorBrush* operator()(D2D1_COLOR_F color) { brush->SetColor(color); return brush.Get(); }
            D2D1_COLOR_F Dim() const
            {
                if (p.highContrast) { const COLORREF g = GetSysColor(COLOR_GRAYTEXT); return D2D1::ColorF(GetRValue(g) / 255.f, GetGValue(g) / 255.f, GetBValue(g) / 255.f); }
                return BlendColor(p.onSurface, p.surface, .38f);
            }
            D2D1_COLOR_F Faint() const { return p.highContrast ? Dim() : BlendColor(p.onSurface, p.surface, .12f); }
            D2D1_COLOR_F Layer(D2D1_COLOR_F color, float opacity) const { color.a = opacity; return color; }
        };

        D2D1_RECT_F Inflate(D2D1_RECT_F r, float d) { return D2D1::RectF(r.left - d, r.top - d, r.right + d, r.bottom + d); }
        D2D1_POINT_2F Center(D2D1_RECT_F r) { return D2D1::Point2F((r.left + r.right) / 2, (r.top + r.bottom) / 2); }

        void Check(Ink& k, D2D1_POINT_2F c, float s, D2D1_COLOR_F color, float width)
        {
            k.target->DrawLine(D2D1::Point2F(c.x - .30f * s, c.y + .02f * s), D2D1::Point2F(c.x - .08f * s, c.y + .26f * s), k(color), width);
            k.target->DrawLine(D2D1::Point2F(c.x - .08f * s, c.y + .26f * s), D2D1::Point2F(c.x + .32f * s, c.y - .22f * s), k(color), width);
        }

        float TextWidth(Ink& k, const CString& s, float size, DWRITE_FONT_WEIGHT weight)
        {
            ComPtr<IDWriteTextFormat> format; ComPtr<IDWriteTextLayout> layout;
            if (s.IsEmpty() || FAILED(k.text->CreateTextFormat(L"Segoe UI", nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", format.GetAddressOf()))) return 0;
            if (FAILED(k.text->CreateTextLayout(s, s.GetLength(), format.Get(), 4096, 64, layout.GetAddressOf()))) return 0;
            DWRITE_TEXT_METRICS metrics{}; layout->GetMetrics(&metrics); return metrics.widthIncludingTrailingWhitespace;
        }

        void Text(Ink& k, const CString& s, D2D1_RECT_F r, float size, DWRITE_FONT_WEIGHT weight, D2D1_COLOR_F color,
            DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING, bool wrap = false, bool observe = false)
        {
            if (s.IsEmpty() || r.right - r.left < 2 || r.bottom - r.top < 2) return;
            ComPtr<IDWriteTextFormat> format; ComPtr<IDWriteTextLayout> layout;
            if (FAILED(k.text->CreateTextFormat(L"Segoe UI", nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", format.GetAddressOf()))) return;
            format->SetTextAlignment(align);
            format->SetParagraphAlignment(wrap ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            format->SetWordWrapping(wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
            if (!wrap)
            {
                ComPtr<IDWriteInlineObject> sign;
                if (SUCCEEDED(k.text->CreateEllipsisTrimmingSign(format.Get(), sign.GetAddressOf())))
                {
                    DWRITE_TRIMMING trimming{ DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                    format->SetTrimming(&trimming, sign.Get());
                }
            }
            if (FAILED(k.text->CreateTextLayout(s, s.GetLength(), format.Get(), r.right - r.left, r.bottom - r.top, layout.GetAddressOf()))) return;
            const auto origin = D2D1::Point2F(r.left, r.top);
            k.target->DrawTextLayout(origin, layout.Get(), k(color), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            if (observe) NativeLayoutObservation::ObserveDirectWrite(k.target, layout.Get(), format.Get(), origin);
        }

        void StateLayerCircle(Ink& k, D2D1_POINT_2F c, D2D1_COLOR_F color, const InputState& s)
        {
            if (!s.enabled || !(s.hover || s.pressed || s.focused)) return;
            k.target->FillEllipse(D2D1::Ellipse(c, StateLayer / 2, StateLayer / 2), k(k.Layer(color, s.pressed || s.focused ? .12f : .08f)));
        }

        void FocusRing(Ink& k, D2D1_RECT_F r, float radius)
        {
            k.target->DrawRoundedRectangle(D2D1::RoundedRect(r, radius, radius), k(k.p.primary), 2.f);
        }

        // Notched outline used by the stepper and the dropdown; the floating label sits on the top border.
        void OutlinedField(Ink& k, D2D1_RECT_F field, const CString& label, const InputState& s, bool expanded)
        {
            const auto& p = k.p;
            const bool active = s.focused || expanded;
            const float width = (active || s.error) ? 2.f : 1.f;
            D2D1_COLOR_F line = !s.enabled ? k.Faint() : s.error ? p.error : active ? p.primary : s.hover ? p.onSurface : p.outline;
            if (!s.enabled && p.highContrast) line = k.Dim();
            const auto shape = D2D1::RoundedRect(D2D1::RectF(field.left + width / 2, field.top + width / 2, field.right - width / 2, field.bottom - width / 2), 4.f, 4.f);
            k.target->DrawRoundedRectangle(shape, k(line), width);
            if (!label.IsEmpty())
            {
                const float w = TextWidth(k, label, 12.f, DWRITE_FONT_WEIGHT_NORMAL);
                const float x = field.left + 12.f;
                k.target->FillRectangle(D2D1::RectF(x - 4, field.top - 2, x + w + 4, field.top + 3), k(p.surface));
                const auto color = !s.enabled ? k.Dim() : s.error ? p.error : active ? p.primary : p.onSurfaceVariant;
                Text(k, label, D2D1::RectF(x, field.top - 8, (std::min)(field.right - 12, x + w + 6), field.top + 8), 12.f, DWRITE_FONT_WEIGHT_NORMAL, color);
            }
        }

        void MessageLine(Ink& k, D2D1_RECT_F r, const CString& message, const InputState& s)
        {
            Text(k, message, r, 12.f, DWRITE_FONT_WEIGHT_NORMAL, !s.enabled ? k.Dim() : s.error ? k.p.error : k.p.onSurfaceVariant);
        }
    }

    void DrawSwitch(ID2D1RenderTarget* target, const Palette& c, const CString& label, bool on, bool showIcon, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 8 || size.height < 8) return;
        const float cy = size.height / 2, left = 4.f;
        const auto track = D2D1::RectF(left, cy - SwitchTrackHeight / 2, left + SwitchTrackWidth, cy + SwitchTrackHeight / 2);
        const float handle = s.pressed ? 28.f : (on || showIcon) ? 24.f : 16.f;
        const float hx = on ? track.right - 16.f : track.left + 16.f;
        const auto round = D2D1::RoundedRect(track, 16, 16);
        if (on) target->FillRoundedRectangle(round, k(s.enabled ? c.primary : k.Faint()));
        else
        {
            target->FillRoundedRectangle(round, k(s.enabled ? c.surfaceContainer : c.surface));
            target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(track.left + 1, track.top + 1, track.right - 1, track.bottom - 1), 15, 15),
                k(s.enabled ? c.outline : k.Faint()), 2.f);
        }
        const auto handleColor = on ? (s.enabled ? c.onPrimary : c.surface) : (s.enabled ? (s.hover || s.pressed ? c.onSurfaceVariant : c.outline) : k.Dim());
        StateLayerCircle(k, D2D1::Point2F(hx, cy), on ? c.primary : c.onSurface, s);
        target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(hx, cy), handle / 2, handle / 2), k(handleColor));
        if (showIcon)
        {
            const auto ink = on ? (s.enabled ? c.primary : k.Faint()) : (s.enabled ? c.surfaceContainer : c.surface);
            if (on) Check(k, D2D1::Point2F(hx, cy), 14.f, ink, 2.f);
            else
            {
                target->DrawLine(D2D1::Point2F(hx - 4, cy - 4), D2D1::Point2F(hx + 4, cy + 4), k(ink), 2.f);
                target->DrawLine(D2D1::Point2F(hx - 4, cy + 4), D2D1::Point2F(hx + 4, cy - 4), k(ink), 2.f);
            }
        }
        if (s.focused) FocusRing(k, Inflate(track, 3.f), 19.f);
        Text(k, label, D2D1::RectF(track.right + ToggleGap, 0, size.width - 4, size.height), 16.f, DWRITE_FONT_WEIGHT_NORMAL, s.enabled ? c.onSurface : k.Dim(),
            DWRITE_TEXT_ALIGNMENT_LEADING, false, true);
    }

    void DrawCheckbox(ID2D1RenderTarget* target, const Palette& c, const CString& label, CheckState state, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 8 || size.height < 8) return;
        const auto center = D2D1::Point2F(4.f + StateLayer / 2, size.height / 2);
        const auto box = D2D1::RectF(center.x - CheckboxSize / 2, center.y - CheckboxSize / 2, center.x + CheckboxSize / 2, center.y + CheckboxSize / 2);
        const D2D1_COLOR_F accent = s.error ? c.error : c.primary;
        StateLayerCircle(k, center, state == CheckState::Unchecked ? (s.error ? c.error : c.onSurface) : accent, s);
        if (state == CheckState::Unchecked)
        {
            const auto line = !s.enabled ? k.Dim() : s.error ? c.error : c.onSurfaceVariant;
            target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(box.left + 1, box.top + 1, box.right - 1, box.bottom - 1), 2, 2), k(line), 2.f);
        }
        else
        {
            target->FillRoundedRectangle(D2D1::RoundedRect(box, 2, 2), k(s.enabled ? accent : k.Dim()));
            const auto mark = s.enabled ? (s.error ? c.onError : c.onPrimary) : c.surface;
            if (state == CheckState::Checked) Check(k, center, 18.f, mark, 2.f);
            else target->DrawLine(D2D1::Point2F(center.x - 5, center.y), D2D1::Point2F(center.x + 5, center.y), k(mark), 2.f);
        }
        if (s.focused) FocusRing(k, Inflate(D2D1::RectF(center.x - StateLayer / 2, center.y - StateLayer / 2, center.x + StateLayer / 2, center.y + StateLayer / 2), 1.f), 6.f);
        Text(k, label, D2D1::RectF(4.f + StateLayer + 8, 0, size.width - 4, size.height), 16.f, DWRITE_FONT_WEIGHT_NORMAL,
            s.enabled ? (s.error ? c.error : c.onSurface) : k.Dim(), DWRITE_TEXT_ALIGNMENT_LEADING, false, true);
    }

    namespace
    {
        void RadioGlyph(Ink& k, D2D1_POINT_2F center, bool checked, const InputState& s)
        {
            const auto& c = k.p;
            StateLayerCircle(k, center, checked ? c.primary : c.onSurface, s);
            const auto line = !s.enabled ? k.Dim() : checked ? c.primary : c.onSurfaceVariant;
            k.target->DrawEllipse(D2D1::Ellipse(center, RadioSize / 2 - 1, RadioSize / 2 - 1), k(line), 2.f);
            if (checked) k.target->FillEllipse(D2D1::Ellipse(center, 5.f, 5.f), k(line));
        }
    }

    void DrawRadio(ID2D1RenderTarget* target, const Palette& c, const CString& label, bool checked, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 8 || size.height < 8) return;
        const auto center = D2D1::Point2F(4.f + StateLayer / 2, size.height / 2);
        RadioGlyph(k, center, checked, s);
        if (s.focused) FocusRing(k, D2D1::RectF(center.x - StateLayer / 2 - 1, center.y - StateLayer / 2 - 1, center.x + StateLayer / 2 + 1, center.y + StateLayer / 2 + 1), 21.f);
        Text(k, label, D2D1::RectF(4.f + StateLayer + 8, 0, size.width - 4, size.height), 16.f, DWRITE_FONT_WEIGHT_NORMAL,
            s.enabled ? c.onSurface : k.Dim(), DWRITE_TEXT_ALIGNMENT_LEADING, false, true);
    }

    D2D1_RECT_F RadioRowRect(float width, int row)
    {
        return D2D1::RectF(0, row * RowHeight, width, (row + 1) * RowHeight);
    }

    void DrawRadioGroup(ID2D1RenderTarget* target, const Palette& c, const std::vector<CString>& labels, int selected, int focusRow, int hoverRow, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        for (size_t i = 0; i < labels.size(); ++i)
        {
            const auto row = RadioRowRect(size.width, static_cast<int>(i));
            if (row.top >= size.height) break;
            InputState item = s;
            item.hover = s.hover && hoverRow == static_cast<int>(i);
            item.focused = s.focused && focusRow == static_cast<int>(i);
            item.pressed = s.pressed && hoverRow == static_cast<int>(i);
            const auto center = D2D1::Point2F(row.left + 4.f + StateLayer / 2, (row.top + row.bottom) / 2);
            RadioGlyph(k, center, static_cast<int>(i) == selected, item);
            if (item.focused) FocusRing(k, D2D1::RectF(center.x - StateLayer / 2 - 1, center.y - StateLayer / 2 - 1, center.x + StateLayer / 2 + 1, center.y + StateLayer / 2 + 1), 21.f);
            Text(k, labels[i], D2D1::RectF(row.left + 4.f + StateLayer + 8, row.top, row.right - 4, row.bottom), 16.f, DWRITE_FONT_WEIGHT_NORMAL,
                s.enabled ? c.onSurface : k.Dim());
        }
    }

    std::vector<D2D1_RECT_F> SegmentRects(float width, float height, int count)
    {
        std::vector<D2D1_RECT_F> rects;
        if (count <= 0) return rects;
        const float top = (std::max)(0.f, (height - SegmentHeight) / 2), left = 3.f, right = (std::max)(left + 1, width - 3.f);
        const float each = (right - left) / count;
        for (int i = 0; i < count; ++i)
            rects.push_back(D2D1::RectF(left + each * i, top, i == count - 1 ? right : left + each * (i + 1), top + SegmentHeight));
        return rects;
    }

    void DrawSegments(ID2D1RenderTarget* target, const Palette& c, const std::vector<CString>& labels, int selected, int hoverSegment, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        const int count = static_cast<int>(labels.size());
        const auto rects = SegmentRects(size.width, size.height, count);
        if (rects.empty()) return;
        const auto container = D2D1::RectF(rects.front().left, rects.front().top, rects.back().right, rects.back().bottom);
        auto fillSegment = [&](int i, D2D1_COLOR_F color)
        {
            auto r = rects[i]; r.top += 1; r.bottom -= 1;
            if (i == 0) r.left += 1;
            if (i == count - 1) r.right -= 1;
            const float radius = (std::min)(19.f, (r.right - r.left) / 2);
            target->FillRoundedRectangle(D2D1::RoundedRect(r, radius, radius), k(color));
            if (i > 0) target->FillRectangle(D2D1::RectF(r.left, r.top, (std::min)(r.left + radius, r.right), r.bottom), k(color));
            if (i < count - 1) target->FillRectangle(D2D1::RectF((std::max)(r.right - radius, r.left), r.top, r.right, r.bottom), k(color));
        };
        const auto line = s.enabled ? c.outline : k.Faint();
        for (int i = 0; i < count; ++i)
            if (i == selected) fillSegment(i, s.enabled ? c.secondaryContainer : k.Faint());
        for (int i = 0; i < count; ++i)
            if (s.enabled && (i == hoverSegment || (s.focused && i == selected)))
                fillSegment(i, k.Layer(i == selected ? c.onSecondaryContainer : c.onSurface, s.pressed || s.focused ? .12f : .08f));
        target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(container.left + .5f, container.top + .5f, container.right - .5f, container.bottom - .5f), 19.5f, 19.5f), k(line), 1.f);
        for (int i = 1; i < count; ++i)
            target->DrawLine(D2D1::Point2F(rects[i].left, container.top), D2D1::Point2F(rects[i].left, container.bottom), k(line), 1.f);
        for (int i = 0; i < count; ++i)
        {
            const bool isSelected = i == selected;
            const auto ink = !s.enabled ? k.Dim() : isSelected ? c.onSecondaryContainer : c.onSurface;
            const float cy = (rects[i].top + rects[i].bottom) / 2;
            if (isSelected) Check(k, D2D1::Point2F(rects[i].left + 22, cy), 18.f, ink, 2.f);
            Text(k, labels[i], D2D1::RectF(rects[i].left + (isSelected ? 36.f : 8.f), rects[i].top, rects[i].right - 8, rects[i].bottom), 14.f,
                DWRITE_FONT_WEIGHT_MEDIUM, ink, DWRITE_TEXT_ALIGNMENT_CENTER, false, i == 0);
        }
        if (s.focused) FocusRing(k, Inflate(container, 3.f), 22.f);
    }

    StepperLayout LayoutStepper(float width, float height)
    {
        StepperLayout l{};
        l.field = D2D1::RectF(0, FieldTop, width, FieldTop + FieldHeight);
        const float cy = (l.field.top + l.field.bottom) / 2;
        l.minus = D2D1::RectF(l.field.left + 4, cy - StepperButton / 2, l.field.left + 4 + StepperButton, cy + StepperButton / 2);
        l.plus = D2D1::RectF(l.field.right - 4 - StepperButton, cy - StepperButton / 2, l.field.right - 4, cy + StepperButton / 2);
        l.edit = D2D1::RectF(l.minus.right + 4, cy - 12, (std::max)(l.minus.right + 5, l.plus.left - 4), cy + 12);
        l.message = D2D1::RectF(16, l.field.bottom + 2, (std::max)(17.f, width - 16), (std::max)(l.field.bottom + 3, height));
        return l;
    }

    void DrawStepper(ID2D1RenderTarget* target, const Palette& c, const CString& label, const CString& message, bool minusEnabled, bool plusEnabled,
        StepperPart hover, StepperPart pressed, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 120 || size.height < FieldTop + FieldHeight) return;
        const auto l = LayoutStepper(size.width, size.height);
        OutlinedField(k, l.field, label, s, false);
        auto button = [&](D2D1_RECT_F r, StepperPart part, bool enabled)
        {
            const bool usable = s.enabled && enabled;
            InputState b{ usable, hover == part, pressed == part, false, false };
            StateLayerCircle(k, Center(r), c.onSurface, b);
            const auto ink = usable ? c.onSurfaceVariant : k.Dim();
            const auto m = Center(r);
            target->DrawLine(D2D1::Point2F(m.x - 6, m.y), D2D1::Point2F(m.x + 6, m.y), k(ink), 2.f);
            if (part == StepperPart::Plus) target->DrawLine(D2D1::Point2F(m.x, m.y - 6), D2D1::Point2F(m.x, m.y + 6), k(ink), 2.f);
        };
        button(l.minus, StepperPart::Minus, minusEnabled);
        button(l.plus, StepperPart::Plus, plusEnabled);
        MessageLine(k, l.message, message, s);
    }

    DropdownLayout LayoutDropdown(float width, float height)
    {
        DropdownLayout l{};
        l.field = D2D1::RectF(0, FieldTop, width, FieldTop + FieldHeight);
        const float cy = (l.field.top + l.field.bottom) / 2;
        l.arrow = D2D1::RectF(l.field.right - 12 - StepperButton, cy - StepperButton / 2, l.field.right - 12, cy + StepperButton / 2);
        l.text = D2D1::RectF(l.field.left + 16, l.field.top, (std::max)(l.field.left + 17, l.arrow.left), l.field.bottom);
        l.message = D2D1::RectF(16, l.field.bottom + 2, (std::max)(17.f, width - 16), (std::max)(l.field.bottom + 3, height));
        return l;
    }

    void DrawDropdownField(ID2D1RenderTarget* target, const Palette& c, const CString& label, const CString& text, const CString& message,
        bool placeholder, bool expanded, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 80 || size.height < FieldTop + FieldHeight) return;
        const auto l = LayoutDropdown(size.width, size.height);
        OutlinedField(k, l.field, label, s, expanded);
        if (s.enabled && s.hover && !s.focused) target->FillRoundedRectangle(D2D1::RoundedRect(l.field, 4, 4), k(k.Layer(c.onSurface, .04f)));
        Text(k, text, l.text, 16.f, DWRITE_FONT_WEIGHT_NORMAL, !s.enabled ? k.Dim() : placeholder ? c.onSurfaceVariant : c.onSurface, DWRITE_TEXT_ALIGNMENT_LEADING, false, true);
        const auto m = Center(l.arrow);
        const auto ink = s.enabled ? c.onSurfaceVariant : k.Dim();
        const float dy = expanded ? 2.f : -2.f;
        target->DrawLine(D2D1::Point2F(m.x - 5, m.y + dy), D2D1::Point2F(m.x, m.y - dy), k(ink), 2.f);
        target->DrawLine(D2D1::Point2F(m.x, m.y - dy), D2D1::Point2F(m.x + 5, m.y + dy), k(ink), 2.f);
        MessageLine(k, l.message, message, s);
    }

    PopupLayout LayoutPopup(float width, int rowCount, bool hasError)
    {
        PopupLayout l{};
        const float w = (std::max)(width, 120.f);
        l.search = D2D1::RectF(8, 8, w - 8, 56);
        l.regex = D2D1::RectF(l.search.right - 84, l.search.top + 8, l.search.right - 8, l.search.bottom - 8);
        l.edit = D2D1::RectF(l.search.left + 16, l.search.top + 12, (std::max)(l.search.left + 40, l.regex.left - 8), l.search.bottom - 12);
        l.error = hasError ? D2D1::RectF(16, l.search.bottom + 2, w - 16, l.search.bottom + 22) : D2D1::RectF(0, 0, 0, 0);
        const float listTop = l.search.bottom + (hasError ? 26.f : 8.f);
        l.rows = (std::min)((std::max)(rowCount, 1), PopupMaximumRows);
        l.list = D2D1::RectF(0, listTop, w, listTop + l.rows * RowHeight);
        l.frame = D2D1::RectF(0, 0, w, l.list.bottom + 8);
        l.height = l.frame.bottom;
        return l;
    }

    void DrawDropdownPopup(ID2D1RenderTarget* target, const Palette& c, const PopupModel& m)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        const auto l = LayoutPopup(size.width, static_cast<int>(m.rows.size()), !m.error.IsEmpty());
        target->FillRectangle(D2D1::RectF(0, 0, size.width, size.height), k(c.surfaceContainer));
        target->DrawRectangle(D2D1::RectF(.5f, .5f, size.width - .5f, size.height - .5f), k(c.outlineVariant), 1.f);
        target->FillRoundedRectangle(D2D1::RoundedRect(l.search, 24, 24), k(c.surface));
        target->DrawRoundedRectangle(D2D1::RoundedRect(l.search, 24, 24), k(c.outline), 1.f);
        const auto chip = D2D1::RoundedRect(l.regex, 16, 16);
        if (m.regex) target->FillRoundedRectangle(chip, k(c.secondaryContainer));
        else target->DrawRoundedRectangle(chip, k(c.outline), 1.f);
        if (m.regexHot) target->FillRoundedRectangle(chip, k(k.Layer(m.regex ? c.onSecondaryContainer : c.onSurface, .08f)));
        Text(k, m.regexLabel, l.regex, 12.f, DWRITE_FONT_WEIGHT_MEDIUM, m.regex ? c.onSecondaryContainer : c.onSurfaceVariant, DWRITE_TEXT_ALIGNMENT_CENTER);
        if (!m.error.IsEmpty()) Text(k, m.error, l.error, 12.f, DWRITE_FONT_WEIGHT_NORMAL, c.error);
        if (m.rows.empty())
        {
            Text(k, m.emptyText, D2D1::RectF(l.list.left + 16, l.list.top, l.list.right - 16, l.list.top + RowHeight), 14.f, DWRITE_FONT_WEIGHT_NORMAL, c.onSurfaceVariant);
            return;
        }
        for (int i = 0; i < l.rows; ++i)
        {
            const int row = m.firstRow + i;
            if (row < 0 || row >= static_cast<int>(m.rows.size())) break;
            const auto r = D2D1::RectF(l.list.left, l.list.top + i * RowHeight, l.list.right, l.list.top + (i + 1) * RowHeight);
            const bool selected = row == m.selectedRow;
            if (selected) target->FillRectangle(r, k(c.secondaryContainer));
            if (row == m.hotRow) target->FillRectangle(r, k(k.Layer(selected ? c.onSecondaryContainer : c.onSurface, .08f)));
            if (row == m.hotRow) target->DrawRectangle(D2D1::RectF(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1), k(c.primary), 2.f);
            Text(k, m.rows[row], D2D1::RectF(r.left + 16, r.top, r.right - 16, r.bottom), 16.f, DWRITE_FONT_WEIGHT_NORMAL, selected ? c.onSecondaryContainer : c.onSurface);
        }
    }

    LabeledFieldLayout LayoutLabeledField(float width, float height)
    {
        LabeledFieldLayout l{};
        l.label = D2D1::RectF(0, 0, width, 20);
        l.edit = D2D1::RectF(0, 22, (std::max)(1.f, width - StepperButton + 8), 62);
        l.clear = D2D1::RectF((std::max)(0.f, width - StepperButton), 22, width, 62);
        l.message = D2D1::RectF(12, 64, (std::max)(13.f, width - 12), (std::max)(65.f, height));
        return l;
    }

    void DrawLabeledFieldChrome(ID2D1RenderTarget* target, const Palette& c, const CString& label, const CString& message, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 60 || size.height < 40) return;
        const auto l = LayoutLabeledField(size.width, size.height);
        Text(k, label, l.label, 14.f, DWRITE_FONT_WEIGHT_MEDIUM, !s.enabled ? k.Dim() : s.error ? c.error : s.focused ? c.primary : c.onSurfaceVariant,
            DWRITE_TEXT_ALIGNMENT_LEADING, false, true);
        MessageLine(k, l.message, message, s);
    }

    void DrawClearButton(ID2D1RenderTarget* target, const Palette& c, bool fieldFocused, const InputState& s)
    {
        Ink k(target, c); if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 12 || size.height < 12) return;
        target->FillRectangle(D2D1::RectF(0, 0, size.width, size.height), k(c.container));
        const auto line = D2D1::Point2F(0, size.height - 1);
        target->DrawLine(line, D2D1::Point2F((std::max)(0.f, size.width - 8), size.height - 1), k(fieldFocused ? c.primary : c.outline), fieldFocused ? 2.f : 1.f);
        const auto m = D2D1::Point2F(size.width / 2, (size.height - 2) / 2);
        InputState layer = s; layer.focused = s.focused;
        StateLayerCircle(k, m, c.onSurface, layer);
        const auto ink = s.enabled ? c.onSurfaceVariant : k.Dim();
        target->DrawLine(D2D1::Point2F(m.x - 5, m.y - 5), D2D1::Point2F(m.x + 5, m.y + 5), k(ink), 2.f);
        target->DrawLine(D2D1::Point2F(m.x - 5, m.y + 5), D2D1::Point2F(m.x + 5, m.y - 5), k(ink), 2.f);
        if (s.focused) FocusRing(k, D2D1::RectF(m.x - 21, m.y - 21, m.x + 21, m.y + 21), 21.f);
    }
}
