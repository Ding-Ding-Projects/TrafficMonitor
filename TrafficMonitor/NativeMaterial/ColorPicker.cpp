#include "../stdafx.h"
#include "ColorPicker.h"
#include "PickerDraw.h"
#include "InputControls.Internal.h"
#include <deque>
#include <mutex>

namespace NativeMaterial
{
    using PickerDraw::Painter;
    using PickerDraw::TextStyle;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        CString Words(const wchar_t* english, const wchar_t* cantonese) { return NativePreferences::InlinePair(english, cantonese); }
        double Clamp01(double value) { return (std::max)(0.0, (std::min)(1.0, value)); }

        int HexValue(wchar_t ch)
        {
            if (ch >= L'0' && ch <= L'9') return ch - L'0';
            if (ch >= L'a' && ch <= L'f') return ch - L'a' + 10;
            if (ch >= L'A' && ch <= L'F') return ch - L'A' + 10;
            return -1;
        }

        std::mutex g_recentLock;
        std::deque<COLORREF> g_recent;

        constexpr float AreaInset = 4.f, HueEdge = 12.f, HueTrack = 16.f, HueHandle = 11.f;

        // Dialog geometry in DIPs.
        constexpr D2D1_RECT_F AreaRect{ 24, 68, 344, 244 };
        constexpr D2D1_RECT_F HueRect{ 24, 248, 344, 288 };
        constexpr D2D1_RECT_F ChipsRect{ 24, 296, 344, 352 };
        constexpr D2D1_RECT_F PresetCaption{ 24, 360, 344, 376 };
        constexpr D2D1_RECT_F PresetRect{ 24, 378, 344, 440 };
        constexpr D2D1_RECT_F RecentCaption{ 24, 446, 344, 462 };
        constexpr D2D1_RECT_F RecentRect{ 24, 464, 344, 496 };
        constexpr D2D1_RECT_F HexRect{ 368, 68, 576, 154 };
        constexpr D2D1_RECT_F RedRect{ 368, 158, 576, 242 };
        constexpr D2D1_RECT_F GreenRect{ 368, 246, 576, 330 };
        constexpr D2D1_RECT_F BlueRect{ 368, 334, 576, 418 };
        constexpr D2D1_RECT_F OriginalRect{ 368, 430, 468, 486 };
        constexpr D2D1_RECT_F NewRect{ 476, 430, 576, 486 };

        COLORREF ReadableOn(COLORREF background) { return RelativeLuminance(background) > .4 ? RGB(0, 0, 0) : RGB(255, 255, 255); }
    }

    // ------------------------------------------------------------------ color math
    Hsv RgbToHsv(COLORREF color)
    {
        const double r = GetRValue(color) / 255.0, g = GetGValue(color) / 255.0, b = GetBValue(color) / 255.0;
        const double high = (std::max)({ r, g, b }), low = (std::min)({ r, g, b }), delta = high - low;
        Hsv out;
        out.v = high;
        out.s = high <= 0 ? 0 : delta / high;
        if (delta <= 0) out.h = 0;
        else if (high == r) out.h = 60.0 * std::fmod((g - b) / delta, 6.0);
        else if (high == g) out.h = 60.0 * ((b - r) / delta + 2.0);
        else out.h = 60.0 * ((r - g) / delta + 4.0);
        if (out.h < 0) out.h += 360.0;
        if (out.h >= 360.0) out.h -= 360.0;
        return out;
    }

    COLORREF HsvToRgb(const Hsv& hsv)
    {
        double h = std::fmod(hsv.h, 360.0);
        if (h < 0) h += 360.0;
        const double s = Clamp01(hsv.s), v = Clamp01(hsv.v);
        const double chroma = v * s, x = chroma * (1.0 - std::fabs(std::fmod(h / 60.0, 2.0) - 1.0)), m = v - chroma;
        double r = 0, g = 0, b = 0;
        switch (static_cast<int>(h / 60.0) % 6)
        {
        case 0: r = chroma; g = x; break;
        case 1: r = x; g = chroma; break;
        case 2: g = chroma; b = x; break;
        case 3: g = x; b = chroma; break;
        case 4: r = x; b = chroma; break;
        default: r = chroma; b = x; break;
        }
        auto channel = [&](double value) { return static_cast<BYTE>((std::min)(255.0, (std::max)(0.0, std::floor((value + m) * 255.0 + .5)))); };
        return RGB(channel(r), channel(g), channel(b));
    }

    bool ParseHexColor(const CString& text, COLORREF& color)
    {
        CString digits = text;
        digits.Trim();
        if (!digits.IsEmpty() && digits[0] == L'#') digits = digits.Mid(1);
        const int length = digits.GetLength();
        if (length != 6 && length != 3) return false;
        int value[6]{};
        for (int i = 0; i < length; ++i)
        {
            value[i] = HexValue(digits[i]);
            if (value[i] < 0) return false;
        }
        if (length == 3) color = RGB(value[0] * 17, value[1] * 17, value[2] * 17);
        else color = RGB(value[0] * 16 + value[1], value[2] * 16 + value[3], value[4] * 16 + value[5]);
        return true;
    }

    CString FormatHexColor(COLORREF color)
    {
        CString text;
        text.Format(L"#%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
        return text;
    }

    double RelativeLuminance(COLORREF color)
    {
        auto linear = [](int channel)
        {
            const double c = channel / 255.0;
            return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * linear(GetRValue(color)) + 0.7152 * linear(GetGValue(color)) + 0.0722 * linear(GetBValue(color));
    }

    double ContrastRatio(COLORREF a, COLORREF b)
    {
        const double la = RelativeLuminance(a), lb = RelativeLuminance(b);
        return ((std::max)(la, lb) + 0.05) / ((std::min)(la, lb) + 0.05);
    }

    const wchar_t* ContrastGrade(double ratio)
    {
        return ratio >= 4.5 ? L"AA" : ratio >= 3.0 ? L"AA large" : L"Low";
    }

    const std::vector<COLORREF>& PresetPalette()
    {
        static const std::vector<COLORREF> palette = {
            RGB(0xF4, 0x43, 0x36), RGB(0xE9, 0x1E, 0x63), RGB(0x9C, 0x27, 0xB0), RGB(0x67, 0x3A, 0xB7), RGB(0x3F, 0x51, 0xB5),
            RGB(0x21, 0x96, 0xF3), RGB(0x03, 0xA9, 0xF4), RGB(0x00, 0xBC, 0xD4), RGB(0x00, 0x96, 0x88), RGB(0x4C, 0xAF, 0x50),
            RGB(0x8B, 0xC3, 0x4A), RGB(0xFF, 0xEB, 0x3B), RGB(0xFF, 0xC1, 0x07), RGB(0xFF, 0x98, 0x00), RGB(0xFF, 0x57, 0x22),
            RGB(0x79, 0x55, 0x48), RGB(0x9E, 0x9E, 0x9E), RGB(0x60, 0x7D, 0x8B), RGB(0x00, 0x00, 0x00), RGB(0xFF, 0xFF, 0xFF) };
        return palette;
    }

    std::vector<COLORREF> RecentColors()
    {
        std::lock_guard<std::mutex> lock(g_recentLock);
        return std::vector<COLORREF>(g_recent.begin(), g_recent.end());
    }
    void SetRecentColors(const std::vector<COLORREF>& colors)
    {
        std::lock_guard<std::mutex> lock(g_recentLock);
        g_recent.clear();
        for (const COLORREF color : colors)
        {
            if (g_recent.size() >= RecentColorLimit) break;
            if (std::find(g_recent.begin(), g_recent.end(), color) == g_recent.end()) g_recent.push_back(color);
        }
    }
    void RememberColor(COLORREF color)
    {
        std::lock_guard<std::mutex> lock(g_recentLock);
        g_recent.erase(std::remove(g_recent.begin(), g_recent.end(), color), g_recent.end());
        g_recent.push_front(color);
        while (g_recent.size() > RecentColorLimit) g_recent.pop_back();
    }

    // ------------------------------------------------------------------ drawing
    namespace
    {
        ComPtr<ID2D1LinearGradientBrush> Gradient(ID2D1RenderTarget* target, const std::vector<D2D1_GRADIENT_STOP>& stops, D2D1_POINT_2F from, D2D1_POINT_2F to)
        {
            ComPtr<ID2D1GradientStopCollection> collection;
            ComPtr<ID2D1LinearGradientBrush> brush;
            if (FAILED(target->CreateGradientStopCollection(stops.data(), static_cast<UINT32>(stops.size()), collection.GetAddressOf()))) return brush;
            target->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(from, to), collection.Get(), brush.GetAddressOf());
            return brush;
        }
        void Handle(Painter& k, D2D1_POINT_2F center, float radius, COLORREF fill)
        {
            k.target->FillEllipse(D2D1::Ellipse(center, radius, radius), k(PickerDraw::FromRef(fill)));
            k.target->DrawEllipse(D2D1::Ellipse(center, radius, radius), k(D2D1::ColorF(0, 0, 0, .55f)), 3.f);
            k.target->DrawEllipse(D2D1::Ellipse(center, radius, radius), k(D2D1::ColorF(1, 1, 1, 1)), 1.5f);
        }
    }

    void DrawColorArea(ID2D1RenderTarget* target, const Palette& c, const Hsv& hsv, const InputState& state)
    {
        Painter k(target, c);
        if (!k.Ok()) return;
        const auto size = target->GetSize();
        const auto area = D2D1::RectF(AreaInset, AreaInset, size.width - AreaInset, size.height - AreaInset);
        if (area.right - area.left < 16 || area.bottom - area.top < 16) return;
        const auto shape = D2D1::RoundedRect(area, 8.f, 8.f);
        const COLORREF pure = HsvToRgb(Hsv{ hsv.h, 1.0, 1.0 });
        if (auto across = Gradient(target, { { 0.f, D2D1::ColorF(1, 1, 1, 1) }, { 1.f, PickerDraw::FromRef(pure) } }, D2D1::Point2F(area.left, 0), D2D1::Point2F(area.right, 0)))
            target->FillRoundedRectangle(shape, across.Get());
        if (auto down = Gradient(target, { { 0.f, D2D1::ColorF(0, 0, 0, 0) }, { 1.f, D2D1::ColorF(0, 0, 0, 1) } }, D2D1::Point2F(0, area.top), D2D1::Point2F(0, area.bottom)))
            target->FillRoundedRectangle(shape, down.Get());
        k.StrokeRounded(area, 8.f, c.outlineVariant, 1.f);
        const float x = area.left + static_cast<float>(Clamp01(hsv.s)) * (area.right - area.left);
        const float y = area.top + static_cast<float>(1.0 - Clamp01(hsv.v)) * (area.bottom - area.top);
        Handle(k, D2D1::Point2F(x, y), 9.f, HsvToRgb(hsv));
        if (state.focused) k.StrokeRounded(PickerDraw::Inflate(area, 2.f), 10.f, c.primary, 2.f);
    }

    void DrawHueSlider(ID2D1RenderTarget* target, const Palette& c, double hue, const InputState& state)
    {
        Painter k(target, c);
        if (!k.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 40 || size.height < 24) return;
        const float cy = size.height / 2, left = HueEdge, right = size.width - HueEdge;
        const auto track = D2D1::RectF(left - 4.f, cy - HueTrack / 2, right + 4.f, cy + HueTrack / 2);
        std::vector<D2D1_GRADIENT_STOP> stops;
        for (int i = 0; i <= 6; ++i) stops.push_back({ i / 6.f, PickerDraw::FromRef(HsvToRgb(Hsv{ i * 60.0, 1.0, 1.0 })) });
        if (auto rainbow = Gradient(target, stops, D2D1::Point2F(track.left, 0), D2D1::Point2F(track.right, 0)))
            target->FillRoundedRectangle(D2D1::RoundedRect(track, HueTrack / 2, HueTrack / 2), rainbow.Get());
        k.StrokeRounded(track, HueTrack / 2, c.outlineVariant, 1.f);
        const double fraction = (std::max)(0.0, (std::min)(359.0, hue)) / 359.0;
        const float x = left + static_cast<float>(fraction) * (right - left);
        if (state.enabled && (state.hover || state.pressed || state.focused))
            target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, cy), 20.f, 20.f), k(k.Layer(c.primary, state.pressed || state.focused ? .12f : .08f)));
        Handle(k, D2D1::Point2F(x, cy), HueHandle, HsvToRgb(Hsv{ hue, 1.0, 1.0 }));
        if (state.focused) target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, cy), 17.f, 17.f), k(c.primary), 2.f);
    }

    namespace
    {
        D2D1_POINT_2F SwatchCenter(int index, int columns)
        {
            const int column = index % columns, row = index / columns;
            return D2D1::Point2F(2.f + column * (SwatchSize + SwatchGap) + SwatchSize / 2, 2.f + row * (SwatchSize + SwatchGap) + SwatchSize / 2);
        }
    }

    void DrawSwatches(ID2D1RenderTarget* target, const Palette& c, const std::vector<COLORREF>& colors, int columns, int selected, int focus, int hover, const InputState& state)
    {
        Painter k(target, c);
        if (!k.Ok() || columns < 1) return;
        for (int i = 0; i < static_cast<int>(colors.size()); ++i)
        {
            const auto center = SwatchCenter(i, columns);
            const float radius = SwatchSize / 2;
            if (i == hover && state.enabled)
                target->FillEllipse(D2D1::Ellipse(center, radius + 4.f, radius + 4.f), k(k.Layer(c.onSurface, .08f)));
            target->FillEllipse(D2D1::Ellipse(center, radius, radius), k(PickerDraw::FromRef(colors[i])));
            target->DrawEllipse(D2D1::Ellipse(center, radius - .5f, radius - .5f), k(c.outlineVariant), 1.f);
            if (i == selected)
            {
                target->DrawEllipse(D2D1::Ellipse(center, radius + 3.f, radius + 3.f), k(c.primary), 2.f);
                const auto ink = PickerDraw::FromRef(ReadableOn(colors[i]));
                target->DrawLine(D2D1::Point2F(center.x - 6, center.y + .5f), D2D1::Point2F(center.x - 1.5f, center.y + 5), k(ink), 2.f);
                target->DrawLine(D2D1::Point2F(center.x - 1.5f, center.y + 5), D2D1::Point2F(center.x + 6.5f, center.y - 5), k(ink), 2.f);
            }
            else if (i == focus && state.focused)
                target->DrawEllipse(D2D1::Ellipse(center, radius + 3.f, radius + 3.f), k(c.onSurface), 2.f);
        }
    }

    void DrawContrastPreview(ID2D1RenderTarget* target, const Palette& c, COLORREF color, D2D1_RECT_F bounds)
    {
        Painter k(target, c);
        if (!k.Ok()) return;
        const float gap = 8.f, width = (bounds.right - bounds.left - gap) / 2;
        const Palette surfaces[2] = { Palette::Resolve(Appearance::Light), Palette::Resolve(Appearance::Dark) };
        const wchar_t* names[2] = { L"Light surface", L"Dark surface" };
        const wchar_t* namesZh[2] = { L"淺色背景", L"深色背景" };
        for (int i = 0; i < 2; ++i)
        {
            const auto chip = D2D1::RectF(bounds.left + i * (width + gap), bounds.top, bounds.left + i * (width + gap) + width, bounds.bottom);
            k.FillRounded(chip, 12.f, surfaces[i].surface);
            k.StrokeRounded(chip, 12.f, c.outlineVariant, 1.f);
            const COLORREF background = PickerDraw::ToRef(surfaces[i].surface);
            const double ratio = ContrastRatio(color, background);
            TextStyle sample;
            sample.size = 22.f;
            sample.weight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
            k.Text(CString(L"Aa"), D2D1::RectF(chip.left + 14, chip.top, chip.left + 58, chip.bottom), sample, PickerDraw::FromRef(color));
            TextStyle score;
            score.size = 13.f;
            score.weight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
            score.center = false;
            CString line;
            line.Format(L"%.1f:1  %s", ratio, ContrastGrade(ratio));
            k.Text(line, D2D1::RectF(chip.left + 62, chip.top + 10, chip.right - 8, chip.top + 28), score, PickerDraw::FromRef(color));
            TextStyle caption;
            caption.size = 11.f;
            caption.center = false;
            k.Text(Words(names[i], namesZh[i]), D2D1::RectF(chip.left + 62, chip.top + 30, chip.right - 8, chip.top + 48), caption, surfaces[i].onSurfaceVariant);
        }
    }

    // ------------------------------------------------------------------ ColorArea
    BOOL ColorArea::Create(const CString& name, const RECT& bounds, CWnd* parent, UINT id)
    {
        return CreateInput(InputKind::ColorArea, name, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void ColorArea::SetAccessibleName(const CString& name) { SetWindowText(name); Publish(); }
    void ColorArea::SetHsv(const Hsv& hsv)
    {
        m_hsv = hsv;
        m_hsv.s = Clamp01(m_hsv.s);
        m_hsv.v = Clamp01(m_hsv.v);
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    bool ColorArea::MoveTo(double saturation, double value)
    {
        saturation = Clamp01(saturation);
        value = Clamp01(value);
        if (std::fabs(saturation - m_hsv.s) < 1e-9 && std::fabs(value - m_hsv.v) < 1e-9) return false;
        m_hsv.s = saturation;
        m_hsv.v = value;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        Notify(EN_CHANGE);
        return true;
    }
    void ColorArea::PointToSv(float x, float y, float width, float height, double& saturation, double& value)
    {
        const double w = (std::max)(1.0, static_cast<double>(width) - 2 * AreaInset), h = (std::max)(1.0, static_cast<double>(height) - 2 * AreaInset);
        saturation = Clamp01((x - AreaInset) / w);
        value = 1.0 - Clamp01((y - AreaInset) / h);
    }
    void ColorArea::FromPoint(CPoint point)
    {
        CRect client;
        GetClientRect(client);
        double s = 0, v = 0;
        PointToSv(Dip(point.x), Dip(point.y), Dip(client.Width()), Dip(client.Height()), s, v);
        MoveTo(s, v);
    }
    void ColorArea::PointerPressed(CPoint point) { m_dragging = true; FromPoint(point); }
    void ColorArea::PointerMoved(CPoint point) { if (m_pressed && m_dragging) FromPoint(point); }
    void ColorArea::PointerReleased(CPoint point, bool) { if (!m_dragging) return; m_dragging = false; FromPoint(point); }
    bool ColorArea::KeyPressed(UINT key, bool)
    {
        const double step = (GetKeyState(VK_SHIFT) & 0x8000) ? .10 : .01;
        switch (key)
        {
        case VK_LEFT: MoveTo(m_hsv.s - step, m_hsv.v); return true;
        case VK_RIGHT: MoveTo(m_hsv.s + step, m_hsv.v); return true;
        case VK_UP: MoveTo(m_hsv.s, m_hsv.v + step); return true;
        case VK_DOWN: MoveTo(m_hsv.s, m_hsv.v - step); return true;
        case VK_HOME: MoveTo(0.0, m_hsv.v); return true;
        case VK_END: MoveTo(1.0, m_hsv.v); return true;
        case VK_PRIOR: MoveTo(m_hsv.s, 1.0); return true;
        case VK_NEXT: MoveTo(m_hsv.s, 0.0); return true;
        default: return false;
        }
    }
    void ColorArea::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state) { DrawColorArea(target, palette, m_hsv, state); }
    void ColorArea::FillAccess(InputData& data) const
    {
        CString text;
        text.Format(NativePreferences::InlinePair(L"Saturation %d percent, brightness %d percent", L"飽和度 %d%%，亮度 %d%%"),
            static_cast<int>(std::lround(m_hsv.s * 100)), static_cast<int>(std::lround(m_hsv.v * 100)));
        data.text = text.GetString();
    }

    // ------------------------------------------------------------------ HueSlider
    BOOL HueSlider::Create(const CString& name, const RECT& bounds, CWnd* parent, UINT id)
    {
        return CreateInput(InputKind::Slider, name, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void HueSlider::SetHue(int hue)
    {
        m_hue = ((hue % 360) + 360) % 360;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    bool HueSlider::MoveTo(int hue)
    {
        hue = (std::max)(0, (std::min)(359, hue));
        if (hue == m_hue) return false;
        m_hue = hue;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        Notify(EN_CHANGE);
        return true;
    }
    int HueSlider::HueAt(CPoint point) const
    {
        CRect client;
        GetClientRect(client);
        const double width = (std::max)(1.0, static_cast<double>(Dip(client.Width())) - 2 * HueEdge);
        return static_cast<int>(std::lround(Clamp01((Dip(point.x) - HueEdge) / width) * 359.0));
    }
    void HueSlider::PointerPressed(CPoint point) { m_dragging = true; MoveTo(HueAt(point)); }
    void HueSlider::PointerMoved(CPoint point) { if (m_pressed && m_dragging) MoveTo(HueAt(point)); }
    void HueSlider::PointerReleased(CPoint point, bool) { if (!m_dragging) return; m_dragging = false; MoveTo(HueAt(point)); }
    bool HueSlider::KeyPressed(UINT key, bool)
    {
        switch (key)
        {
        case VK_LEFT: case VK_DOWN: MoveTo(m_hue - 1); return true;
        case VK_RIGHT: case VK_UP: MoveTo(m_hue + 1); return true;
        case VK_NEXT: MoveTo(m_hue - 15); return true;
        case VK_PRIOR: MoveTo(m_hue + 15); return true;
        case VK_HOME: MoveTo(0); return true;
        case VK_END: MoveTo(359); return true;
        default: return false;
        }
    }
    void HueSlider::PerformAction(InputActionId action, int value) { if (action == InputActionId::SetValue) MoveTo(value); }
    void HueSlider::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state) { DrawHueSlider(target, palette, m_hue, state); }
    void HueSlider::FillAccess(InputData& data) const
    {
        data.value = m_hue; data.minimum = 0; data.maximum = 359; data.step = 1; data.page = 15;
        data.text = std::to_wstring(m_hue);
    }

    // ------------------------------------------------------------------ SwatchGrid
    BOOL SwatchGrid::Create(const CString& name, int columns, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_columns = (std::max)(1, columns);
        return CreateInput(InputKind::RadioGroup, name, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    int SwatchGrid::Rows() const { return (std::max)(1, (static_cast<int>(m_colors.size()) + m_columns - 1) / m_columns); }
    CSize SwatchGrid::NaturalSize() const
    {
        const int columns = (std::min)(m_columns, (std::max)(1, static_cast<int>(m_colors.size())));
        return CSize(static_cast<int>(std::ceil(columns * (SwatchSize + SwatchGap) - SwatchGap + 4.f)),
            static_cast<int>(std::ceil(Rows() * (SwatchSize + SwatchGap) - SwatchGap + 4.f)));
    }
    void SwatchGrid::SetColors(const std::vector<COLORREF>& colors)
    {
        m_colors = colors;
        if (m_selected >= static_cast<int>(m_colors.size())) m_selected = -1;
        m_focus = (std::max)(0, (std::min)(m_focus, static_cast<int>(m_colors.size()) - 1));
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void SwatchGrid::SetSelected(int index)
    {
        m_selected = index >= 0 && index < static_cast<int>(m_colors.size()) ? index : -1;
        if (m_selected >= 0) m_focus = m_selected;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    int SwatchGrid::IndexAt(CPoint point) const
    {
        const float x = Dip(point.x) - 2.f, y = Dip(point.y) - 2.f;
        if (x < 0 || y < 0) return -1;
        const float pitch = SwatchSize + SwatchGap;
        const int column = static_cast<int>(x / pitch), row = static_cast<int>(y / pitch);
        if (column >= m_columns || x - column * pitch > SwatchSize || y - row * pitch > SwatchSize) return -1;
        const int index = row * m_columns + column;
        return index < static_cast<int>(m_colors.size()) ? index : -1;
    }
    void SwatchGrid::Choose(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_colors.size())) return;
        m_focus = index;
        const bool changed = index != m_selected;
        m_selected = index;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        if (changed) Notify(CBN_SELCHANGE);
    }
    void SwatchGrid::PointerPressed(CPoint point) { m_hot = IndexAt(point); }
    void SwatchGrid::PointerReleased(CPoint point, bool inside)
    {
        const int index = IndexAt(point);
        if (inside && index >= 0 && index == m_hot) Choose(index);
    }
    void SwatchGrid::PointerMoved(CPoint point) { m_hot = IndexAt(point); }
    void SwatchGrid::PointerLeft() { m_hot = -1; }
    bool SwatchGrid::KeyPressed(UINT key, bool)
    {
        const int count = static_cast<int>(m_colors.size());
        if (count == 0) return false;
        int target = m_focus;
        switch (key)
        {
        case VK_LEFT: target = m_focus - 1; break;
        case VK_RIGHT: target = m_focus + 1; break;
        case VK_UP: target = m_focus - m_columns; break;
        case VK_DOWN: target = m_focus + m_columns; break;
        case VK_HOME: target = 0; break;
        case VK_END: target = count - 1; break;
        case VK_SPACE: Choose(m_focus); return true;
        default: return false;
        }
        Choose((std::max)(0, (std::min)(count - 1, target)));
        return true;
    }
    void SwatchGrid::PerformAction(InputActionId action, int value) { if (action == InputActionId::Select) Choose(value); }
    void SwatchGrid::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        DrawSwatches(target, palette, m_colors, m_columns, m_selected, m_focus, m_hot, state);
    }
    void SwatchGrid::FillAccess(InputData& data) const
    {
        for (const COLORREF color : m_colors) data.items.push_back(FormatHexColor(color).GetString());
        data.selected = m_selected;
    }

    // ------------------------------------------------------------------ ColorPickerDialog
    ColorPickerDialog::ColorPickerDialog() = default;
    ColorPickerDialog::~ColorPickerDialog() = default;

    ColorPickerDialog* ColorPickerDialog::FromHandle(HWND dialog) { return dynamic_cast<ColorPickerDialog*>(PickerDialog::FromWindow(dialog)); }

    bool ColorPickerDialog::Run(HWND owner, COLORREF& color, const CString& title)
    {
        m_original = color;
        m_color = color;
        m_hsv = RgbToHsv(color);
        m_hexValid = true;
        const CString heading = title.IsEmpty() ? Words(L"Choose color", L"揀選顏色") : title;
        const int result = RunModal(owner, heading, 600, 580);
        if (result != IDOK || !m_hexValid) return false;
        color = m_color;
        RememberColor(m_color);
        return true;
    }

    void ColorPickerDialog::SetColor(COLORREF color) { ApplyRgb(color, Source::Program); }

    void ColorPickerDialog::CreateContent()
    {
        const CRect none(0, 0, 10, 10);
        m_area.Create(Words(L"Saturation and brightness", L"飽和度同亮度"), none, this, IdArea);
        m_hue.Create(Words(L"Hue", L"色相"), none, this, IdHue);
        m_hex.Create(Words(L"Hex", L"十六進位"), none, this, IdHex);
        m_red.Create(Words(L"Red", L"紅"), none, this, IdRed);
        m_green.Create(Words(L"Green", L"綠"), none, this, IdGreen);
        m_blue.Create(Words(L"Blue", L"藍"), none, this, IdBlue);
        m_presets.Create(Words(L"Preset colors", L"預設顏色"), 10, none, this, IdPresets);
        m_recents.Create(Words(L"Recent colors", L"最近用過的顏色"), static_cast<int>(RecentColorLimit), none, this, IdRecents);
        Style(m_area); Style(m_hue); Style(m_hex); Style(m_red); Style(m_green); Style(m_blue); Style(m_presets); Style(m_recents);
        for (NumberStepper* stepper : { &m_red, &m_green, &m_blue }) { stepper->SetRange(0, 255); stepper->SetStep(1); }
        m_hex.SetSupportingText(CString(L"#RRGGBB"));
        m_presets.SetColors(PresetPalette());
        m_recents.SetColors(RecentColors());
        Sync(Source::Program);
    }

    void ColorPickerDialog::LayoutContent()
    {
        auto place = [&](CWnd& window, D2D1_RECT_F r) { MovePx(window, r.left, r.top, r.right - r.left, r.bottom - r.top); };
        place(m_area, AreaRect);
        place(m_hue, HueRect);
        place(m_hex, HexRect);
        place(m_red, RedRect);
        place(m_green, GreenRect);
        place(m_blue, BlueRect);
        const CSize presets = m_presets.NaturalSize(), recents = m_recents.NaturalSize();
        MovePx(m_presets, PresetRect.left, PresetRect.top, static_cast<float>(presets.cx), static_cast<float>(presets.cy));
        MovePx(m_recents, RecentRect.left, RecentRect.top, static_cast<float>(recents.cx), static_cast<float>(recents.cy));
    }

    void ColorPickerDialog::PaintContent(ID2D1RenderTarget* target, const Palette& palette)
    {
        DrawContrastPreview(target, palette, m_color, ChipsRect);
        Painter k(target, palette);
        if (!k.Ok()) return;
        TextStyle caption;
        caption.size = 12.f;
        caption.weight = DWRITE_FONT_WEIGHT_MEDIUM;
        k.Text(Words(L"Presets", L"預設"), PresetCaption, caption, palette.onSurfaceVariant);
        k.Text(Words(L"Recent", L"最近"), RecentCaption, caption, palette.onSurfaceVariant);
        if (m_recents.Colors().empty())
        {
            TextStyle hint;
            hint.size = 12.f;
            k.Text(Words(L"Colors you pick show up here", L"揀選過的顏色會顯示喺這裡"), RecentRect, hint, k.Dim());
        }
        auto pair = [&](D2D1_RECT_F r, COLORREF color, const CString& label)
        {
            k.FillRounded(r, 12.f, PickerDraw::FromRef(color));
            k.StrokeRounded(r, 12.f, palette.outlineVariant, 1.f);
            const auto ink = PickerDraw::FromRef(ReadableOn(color));
            TextStyle note;
            note.size = 11.f;
            note.center = false;
            k.Text(label, D2D1::RectF(r.left + 12, r.top + 8, r.right - 8, r.top + 24), note, ink);
            TextStyle code;
            code.size = 14.f;
            code.weight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
            code.center = false;
            k.Text(FormatHexColor(color), D2D1::RectF(r.left + 12, r.top + 28, r.right - 8, r.top + 48), code, ink);
        };
        pair(OriginalRect, m_original, Words(L"Original", L"原本"));
        pair(NewRect, m_color, Words(L"New", L"新"));
    }

    std::vector<HWND> ColorPickerDialog::ContentFocusOrder() const
    {
        std::vector<HWND> order{ m_area.GetSafeHwnd(), m_hue.GetSafeHwnd(), const_cast<LabeledTextField&>(m_hex).Field().GetSafeHwnd(),
            m_red.EditWindow(), m_green.EditWindow(), m_blue.EditWindow(), m_presets.GetSafeHwnd() };
        if (!m_recents.Colors().empty()) order.push_back(m_recents.GetSafeHwnd());
        return order;
    }

    bool ColorPickerDialog::CommitPending()
    {
        // Typed channel text that Enter has not applied yet is applied before the dialog accepts; rejected text keeps it open.
        const HWND focus = ::GetFocus();
        for (NumberStepper* stepper : { &m_red, &m_green, &m_blue })
            if (focus && focus == stepper->EditWindow() && !stepper->CommitText()) return false;
        return true;
    }

    void ColorPickerDialog::Sync(Source source)
    {
        m_updating = true;
        if (source != Source::Area) m_area.SetHsv(m_hsv);
        if (source != Source::Hue) m_hue.SetHue(static_cast<int>(std::lround(m_hsv.h)));
        if (source != Source::Hex)
        {
            m_hex.SetText(FormatHexColor(m_color));
            m_hex.SetErrorText(CString());
            m_hexValid = true;
        }
        if (source != Source::Rgb)
        {
            m_red.SetValue(GetRValue(m_color));
            m_green.SetValue(GetGValue(m_color));
            m_blue.SetValue(GetBValue(m_color));
        }
        auto indexOf = [&](const std::vector<COLORREF>& colors)
        {
            const auto found = std::find(colors.begin(), colors.end(), m_color);
            return found == colors.end() ? -1 : static_cast<int>(found - colors.begin());
        };
        if (source != Source::Swatch) { m_presets.SetSelected(indexOf(m_presets.Colors())); m_recents.SetSelected(indexOf(m_recents.Colors())); }
        SetOkEnabled(m_hexValid);
        if (m_hWnd) Invalidate(FALSE);
        m_updating = false;
    }

    void ColorPickerDialog::ApplyHsv(const Hsv& hsv, Source source)
    {
        m_hsv = hsv;
        m_color = HsvToRgb(hsv);
        Sync(source);
    }

    void ColorPickerDialog::ApplyRgb(COLORREF color, Source source)
    {
        Hsv next = RgbToHsv(color);
        if (next.v == 0.0) { next.h = m_hsv.h; next.s = m_hsv.s; }       // black keeps the chosen hue and saturation
        else if (next.s == 0.0) next.h = m_hsv.h;                        // grays keep the chosen hue
        m_hsv = next;
        m_color = color;
        Sync(source);
    }

    bool ColorPickerDialog::ContentCommand(UINT id, UINT code)
    {
        if (m_updating) return false;
        switch (id)
        {
        case IdArea:
            if (code != EN_CHANGE) return false;
            { Hsv next = m_hsv; next.s = m_area.GetHsv().s; next.v = m_area.GetHsv().v; ApplyHsv(next, Source::Area); }
            return true;
        case IdHue:
            if (code != EN_CHANGE) return false;
            { Hsv next = m_hsv; next.h = m_hue.GetHue(); ApplyHsv(next, Source::Hue); }
            return true;
        case IdHex:
        {
            if (code != EN_CHANGE) return false;
            COLORREF parsed{};
            if (ParseHexColor(m_hex.GetText(), parsed))
            {
                m_hex.SetErrorText(CString());
                m_hexValid = true;
                ApplyRgb(parsed, Source::Hex);
            }
            else
            {
                m_hexValid = false;
                m_hex.SetErrorText(Words(L"Use a color like #3A7BD5", L"請輸入如 #3A7BD5 的顏色"));
                SetOkEnabled(false);
            }
            return true;
        }
        case IdRed: case IdGreen: case IdBlue:
            if (code != EN_CHANGE) return false;
            ApplyRgb(RGB(m_red.GetValue(), m_green.GetValue(), m_blue.GetValue()), Source::Rgb);
            return true;
        case IdPresets: case IdRecents:
        {
            if (code != CBN_SELCHANGE) return false;
            SwatchGrid& grid = id == IdPresets ? m_presets : m_recents;
            const int index = grid.GetSelected();
            if (index >= 0 && index < static_cast<int>(grid.Colors().size())) ApplyRgb(grid.Colors()[index], Source::Swatch);
            // The swatch the user chose is selected in its own grid only when the color really matches; refresh the other grid.
            (id == IdPresets ? m_recents : m_presets).SetSelected(-1);
            return true;
        }
        default: return false;
        }
    }

    bool PickColor(HWND owner, COLORREF& inOut, const CString& title)
    {
        ColorPickerDialog dialog;
        return dialog.Run(owner, inOut, title);
    }
}
