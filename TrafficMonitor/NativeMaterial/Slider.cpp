#include "../stdafx.h"
#include "Slider.h"
#include "InputControls.Internal.h"
#include "LayoutObservations.h"

namespace NativeMaterial
{
    namespace
    {
        constexpr float HandleRadius = 10.f, TrackHeight = 4.f, EdgePadding = 20.f, ValueColumn = 44.f;
        int Clamp(int value, int low, int high) { return (std::max)(low, (std::min)(high, value)); }
    }

    SliderLayout LayoutSlider(float width, float height, bool showValue)
    {
        SliderLayout layout{};
        layout.left = EdgePadding;
        layout.right = (std::max)(layout.left + 8.f, width - EdgePadding - (showValue ? ValueColumn : 0.f));
        layout.centerY = height / 2;
        layout.value = showValue ? D2D1::RectF(layout.right + EdgePadding - 4.f, 0, width - 4.f, height) : D2D1::RectF(0, 0, 0, 0);
        return layout;
    }
    float SliderHandleX(const SliderLayout& layout, int value, int minimum, int maximum)
    {
        if (maximum <= minimum) return layout.left;
        const float fraction = static_cast<float>(Clamp(value, minimum, maximum) - minimum) / static_cast<float>(maximum - minimum);
        return layout.left + fraction * (layout.right - layout.left);
    }
    int SliderSnap(int value, int minimum, int maximum, int step)
    {
        value = Clamp(value, minimum, maximum);
        if (step > 1)
        {
            const long long offset = static_cast<long long>(value) - minimum;
            const long long snapped = (offset + step / 2) / step * step;
            value = static_cast<int>((std::min)(static_cast<long long>(maximum), static_cast<long long>(minimum) + snapped));
        }
        return value;
    }
    int SliderValueAt(const SliderLayout& layout, float x, int minimum, int maximum, int step)
    {
        if (maximum <= minimum || layout.right <= layout.left) return minimum;
        const float fraction = (std::max)(0.f, (std::min)(1.f, (x - layout.left) / (layout.right - layout.left)));
        const int value = minimum + static_cast<int>(std::lround(fraction * static_cast<float>(maximum - minimum)));
        return SliderSnap(value, minimum, maximum, step);
    }

    void DrawSlider(ID2D1RenderTarget* target, const Palette& c, int value, int minimum, int maximum, int step, bool discrete,
        bool showValue, const CString& valueText, const InputState& s)
    {
        const auto size = target->GetSize();
        if (size.width < 40 || size.height < 24) return;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        Microsoft::WRL::ComPtr<IDWriteFactory> text;
        if (FAILED(target->CreateSolidColorBrush(c.primary, brush.GetAddressOf()))) return;
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(text.GetAddressOf()));
        auto ink = [&](D2D1_COLOR_F color) { brush->SetColor(color); return brush.Get(); };
        const COLORREF gray = GetSysColor(COLOR_GRAYTEXT);
        const D2D1_COLOR_F dim = c.highContrast ? D2D1::ColorF(GetRValue(gray) / 255.f, GetGValue(gray) / 255.f, GetBValue(gray) / 255.f) : BlendColor(c.onSurface, c.surface, .38f);
        const D2D1_COLOR_F faint = c.highContrast ? dim : BlendColor(c.onSurface, c.surface, .12f);

        const auto layout = LayoutSlider(size.width, size.height, showValue);
        const float handleX = SliderHandleX(layout, value, minimum, maximum), cy = layout.centerY;
        const D2D1_COLOR_F activeColor = s.enabled ? c.primary : dim;
        const D2D1_COLOR_F inactiveColor = !s.enabled ? faint : c.highContrast ? c.outline : c.secondaryContainer;
        target->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(layout.left - 2.f, cy - TrackHeight / 2, layout.right + 2.f, cy + TrackHeight / 2), 2.f, 2.f), ink(inactiveColor));
        if (handleX > layout.left)
            target->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(layout.left - 2.f, cy - TrackHeight / 2, handleX, cy + TrackHeight / 2), 2.f, 2.f), ink(activeColor));
        if (discrete && step > 0 && maximum > minimum && (maximum - minimum) / step <= 100)
        {
            for (long long stop = minimum; stop <= maximum; stop += step)
            {
                const float x = SliderHandleX(layout, static_cast<int>(stop), minimum, maximum);
                if (std::fabs(x - handleX) < HandleRadius + 3.f) continue;
                const bool covered = x < handleX;
                target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, cy), 2.f, 2.f), ink(covered ? (s.enabled ? c.onPrimary : c.surface) : (s.enabled ? c.onSecondaryContainer : dim)));
            }
        }
        const auto center = D2D1::Point2F(handleX, cy);
        if (s.enabled && (s.hover || s.pressed || s.focused))
        {
            auto layer = c.primary; layer.a = s.pressed || s.focused ? .12f : .08f;
            target->FillEllipse(D2D1::Ellipse(center, InputMetrics::StateLayer / 2, InputMetrics::StateLayer / 2), ink(layer));
        }
        target->FillEllipse(D2D1::Ellipse(center, HandleRadius, HandleRadius), ink(activeColor));
        if (s.focused)
            target->DrawEllipse(D2D1::Ellipse(center, InputMetrics::StateLayer / 2 + 1.f, InputMetrics::StateLayer / 2 + 1.f), ink(c.primary), 2.f);
        if (showValue && !valueText.IsEmpty() && text)
        {
            Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
            Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
            if (SUCCEEDED(text->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.f, L"", format.GetAddressOf())))
            {
                format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                const auto r = layout.value;
                if (SUCCEEDED(text->CreateTextLayout(valueText, valueText.GetLength(), format.Get(), (std::max)(1.f, r.right - r.left), (std::max)(1.f, r.bottom - r.top), textLayout.GetAddressOf())))
                {
                    const auto origin = D2D1::Point2F(r.left, r.top);
                    target->DrawTextLayout(origin, textLayout.Get(), ink(s.enabled ? c.onSurface : dim), D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    NativeLayoutObservation::ObserveDirectWrite(target, textLayout.Get(), format.Get(), origin);
                }
            }
        }
    }

    // ---------------------------------------------------------------- Slider
    BOOL Slider::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        return CreateInput(InputKind::Slider, label, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void Slider::SetRange(int minimum, int maximum)
    {
        if (minimum > maximum) std::swap(minimum, maximum);
        m_minimum = minimum; m_maximum = maximum;
        m_value = SliderSnap(m_value, m_minimum, m_maximum, m_step);
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void Slider::SetStep(int step)
    {
        m_step = (std::max)(1, step);
        m_value = SliderSnap(m_value, m_minimum, m_maximum, m_step);
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void Slider::SetPageStep(int page) { m_page = (std::max)(0, page); Publish(); }
    void Slider::SetDiscrete(bool discrete) { m_discrete = discrete; if (m_hWnd) Invalidate(FALSE); }
    void Slider::SetShowValue(bool show) { m_showValue = show; if (m_hWnd) Invalidate(FALSE); }
    void Slider::SetLabel(const CString& label) { SetWindowText(label); Publish(); if (m_hWnd) Invalidate(FALSE); }
    void Slider::SetValue(int value)
    {
        m_value = SliderSnap(value, m_minimum, m_maximum, m_step);
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    CString Slider::GetValueText() const { CString text; text.Format(L"%d", m_value); return text; }

    SliderLayout Slider::CurrentLayout() const
    {
        CRect client;
        GetClientRect(client);
        return LayoutSlider(Dip(client.Width()), Dip(client.Height()), m_showValue);
    }
    int Slider::ValueAtClientX(int x) const
    {
        return SliderValueAt(CurrentLayout(), Dip(x), m_minimum, m_maximum, m_step);
    }

    void Slider::NotifyScroll(UINT code)
    {
        if (!m_hWnd || !IsWindowEnabled() || !GetParent()) return;
        const WORD position = (code == SB_THUMBTRACK || code == SB_THUMBPOSITION) ? static_cast<WORD>(m_value & 0xFFFF) : 0;
        GetParent()->SendMessage(WM_HSCROLL, MAKEWPARAM(code, position), reinterpret_cast<LPARAM>(m_hWnd));
    }
    bool Slider::MoveTo(int value, UINT scrollCode)
    {
        value = Clamp(value, m_minimum, m_maximum);
        if (value == m_value) return false;
        m_value = value;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        Notify(EN_CHANGE);
        NotifyScroll(scrollCode);
        return true;
    }

    void Slider::PointerPressed(CPoint point)
    {
        m_dragging = true;
        MoveTo(ValueAtClientX(point.x), SB_THUMBTRACK);
    }
    void Slider::PointerMoved(CPoint point)
    {
        if (m_pressed && m_dragging) MoveTo(ValueAtClientX(point.x), SB_THUMBTRACK);
    }
    void Slider::PointerReleased(CPoint point, bool)
    {
        if (!m_dragging) return;
        m_dragging = false;
        MoveTo(ValueAtClientX(point.x), SB_THUMBTRACK);
        NotifyScroll(SB_THUMBPOSITION);
        NotifyScroll(SB_ENDSCROLL);
    }
    void Slider::PointerCanceled()
    {
        if (!m_dragging) return;
        m_dragging = false;
        NotifyScroll(SB_ENDSCROLL);
    }

    bool Slider::KeyPressed(UINT key, bool)
    {
        const int page = m_page > 0 ? m_page : (std::max)(m_step, (m_maximum - m_minimum) / 10);
        UINT code = 0; int target = m_value;
        switch (key)
        {
        case VK_LEFT: case VK_DOWN: code = SB_LINELEFT; target = m_value - m_step; break;
        case VK_RIGHT: case VK_UP: code = SB_LINERIGHT; target = m_value + m_step; break;
        case VK_NEXT: code = SB_PAGELEFT; target = m_value - page; break;
        case VK_PRIOR: code = SB_PAGERIGHT; target = m_value + page; break;
        case VK_HOME: code = SB_LEFT; target = m_minimum; break;
        case VK_END: code = SB_RIGHT; target = m_maximum; break;
        default: return false;
        }
        if (MoveTo(target, code)) NotifyScroll(SB_ENDSCROLL);
        return true;
    }
    void Slider::PerformAction(InputActionId action, int value)
    {
        if (action != InputActionId::SetValue || value < m_minimum || value > m_maximum) return;
        if (MoveTo(SliderSnap(value, m_minimum, m_maximum, m_step), SB_THUMBPOSITION)) NotifyScroll(SB_ENDSCROLL);
    }

    void Slider::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        DrawSlider(target, palette, m_value, m_minimum, m_maximum, m_step, m_discrete, m_showValue, GetValueText(), state);
    }
    CSize Slider::MinimumLogicalSize() const
    {
        return CSize(m_showValue ? 168 : 128, static_cast<int>(InputMetrics::MinimumTarget));
    }
    void Slider::FillAccess(InputData& data) const
    {
        data.value = m_value; data.minimum = m_minimum; data.maximum = m_maximum; data.step = m_step; data.page = m_page;
        data.text = std::to_wstring(m_value);
    }
}