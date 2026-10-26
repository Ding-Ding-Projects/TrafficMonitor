#include "../stdafx.h"
#include "Tabs.h"
#include "TextPainter.h"
#include "InputControls.Internal.h"
#include <algorithm>
#include <cmath>

namespace NativeMaterial
{
    namespace
    {
        constexpr float IconBox = 24.f, IconGap = 8.f;
        float Clamp(float value, float low, float high) { return (std::max)(low, (std::min)(high, value)); }
        bool AnyIcon(const std::vector<wchar_t>& icons)
        {
            for (wchar_t icon : icons) if (icon) return true;
            return false;
        }
        D2D1_COLOR_F Dim(const Palette& p)
        {
            if (p.highContrast) { const COLORREF g = GetSysColor(COLOR_GRAYTEXT); return D2D1::ColorF(GetRValue(g) / 255.f, GetGValue(g) / 255.f, GetBValue(g) / 255.f); }
            return BlendColor(p.onSurface, p.surface, .38f);
        }
        D2D1_COLOR_F Layer(D2D1_COLOR_F color, float opacity) { color.a = opacity; return color; }
    }

    // ------------------------------------------------------------ geometry
    std::vector<float> MeasureLabelWidths(const std::vector<CString>& labels)
    {
        TextPainter painter(nullptr);
        std::vector<float> widths;
        for (const auto& label : labels) widths.push_back(painter.Width(label, 14.f, DWRITE_FONT_WEIGHT_MEDIUM));
        return widths;
    }

    std::vector<float> MeasureTabWidths(const std::vector<CString>& labels, const std::vector<wchar_t>& icons, TabStyle style)
    {
        const auto label = MeasureLabelWidths(labels);
        std::vector<float> widths;
        for (size_t i = 0; i < label.size(); ++i)
        {
            const bool icon = i < icons.size() && icons[i] != 0;
            float content = label[i];
            if (icon) content = style == TabStyle::Primary ? (std::max)(content, IconBox) : (labels[i].IsEmpty() ? IconBox : content + IconBox + IconGap);
            widths.push_back(Clamp(content + 2 * TabMetrics::Padding, TabMetrics::MinimumWidth, TabMetrics::MaximumWidth));
        }
        return widths;
    }

    float TabsBarHeight(bool anyIcon, TabStyle style)
    {
        return anyIcon && style == TabStyle::Primary ? TabMetrics::HeightWithIcon : TabMetrics::Height;
    }

    TabsLayout LayoutTabs(float width, float height, const std::vector<float>& intrinsic, const std::vector<float>& labelWidths,
        float scroll, TabStyle style, int selected)
    {
        TabsLayout layout;
        const size_t count = intrinsic.size();
        if (count == 0 || width <= 0) return layout;
        float sum = 0, widest = 0;
        for (float w : intrinsic) { sum += w; widest = (std::max)(widest, w); }
        std::vector<float> widths(intrinsic);
        const float n = static_cast<float>(count);
        if (widest * n <= width) widths.assign(count, width / n);                  // fixed: equal tabs
        else if (sum <= width) { const float extra = (width - sum) / n; for (float& w : widths) w += extra; }
        else layout.scrollable = true;                                              // each tab keeps its own width
        layout.contentWidth = 0;
        for (float w : widths) layout.contentWidth += w;
        layout.maxScroll = layout.scrollable ? (std::max)(0.f, layout.contentWidth - width) : 0.f;
        scroll = Clamp(scroll, 0.f, layout.maxScroll);
        float x = -scroll;
        for (float w : widths)
        {
            layout.tabs.push_back(D2D1::RectF(x, 0, x + w, height));
            x += w;
        }
        if (selected >= 0 && selected < static_cast<int>(count))
        {
            const auto& tab = layout.tabs[selected];
            if (style == TabStyle::Secondary)
                layout.indicator = D2D1::RectF(tab.left, height - TabMetrics::SecondaryIndicator, tab.right, height);
            else
            {
                const float label = selected < static_cast<int>(labelWidths.size()) ? labelWidths[selected] : 0.f;
                const float span = Clamp((std::max)(label, 24.f), 24.f, (std::max)(24.f, tab.right - tab.left - 8.f));
                const float centre = (tab.left + tab.right) / 2;
                layout.indicator = D2D1::RectF(centre - span / 2, height - TabMetrics::PrimaryIndicator, centre + span / 2, height);
            }
        }
        return layout;
    }

    float TabScrollToReveal(const TabsLayout& unscrolled, int index, float viewWidth, float current)
    {
        if (!unscrolled.scrollable || index < 0 || index >= static_cast<int>(unscrolled.tabs.size())) return Clamp(current, 0.f, unscrolled.maxScroll);
        const auto& tab = unscrolled.tabs[index];                                   // content coordinates (scroll 0)
        float scroll = current;
        if (tab.left < scroll) scroll = tab.left;
        else if (tab.right > scroll + viewWidth) scroll = tab.right - viewWidth;
        return Clamp(scroll, 0.f, unscrolled.maxScroll);
    }

    // ------------------------------------------------------------ drawing
    void DrawTabs(ID2D1RenderTarget* target, const Palette& p, const TabsModel& model, const InputState& state)
    {
        TextPainter painter(target);
        if (!painter.Ok()) return;
        const auto size = target->GetSize();
        if (size.width < 8 || size.height < 8) return;
        const auto intrinsic = MeasureTabWidths(model.labels, model.icons, model.style);
        const auto labelWidths = MeasureLabelWidths(model.labels);
        const auto layout = LayoutTabs(size.width, size.height, intrinsic, labelWidths, model.scroll, model.style, model.selected);
        const bool enabled = state.enabled;
        target->PushAxisAlignedClip(D2D1::RectF(0, 0, size.width, size.height), D2D1_ANTIALIAS_MODE_ALIASED);
        target->DrawLine(D2D1::Point2F(0, size.height - .5f), D2D1::Point2F(size.width, size.height - .5f), painter.Brush(p.outlineVariant), 1.f);
        for (size_t i = 0; i < layout.tabs.size(); ++i)
        {
            const auto r = layout.tabs[i];
            if (r.right < 0 || r.left > size.width) continue;
            const int index = static_cast<int>(i);
            const bool active = index == model.selected;
            const bool primary = model.style == TabStyle::Primary;
            const D2D1_COLOR_F ink = !enabled ? Dim(p) : active ? (primary ? p.primary : p.onSurface) : p.onSurfaceVariant;
            const D2D1_COLOR_F layer = active && primary ? p.primary : p.onSurface;
            const bool pressed = index == model.pressed, hover = index == model.hot, focus = state.focused && active;
            if (enabled && (pressed || hover || focus))
                target->FillRectangle(D2D1::RectF(r.left, r.top, r.right, r.bottom - 1.f), painter.Brush(Layer(layer, pressed || focus ? .12f : .08f)));
            const wchar_t icon = i < model.icons.size() ? model.icons[i] : 0;
            const auto& label = model.labels[i];
            const float cx = (r.left + r.right) / 2;
            if (icon && primary)
            {
                const float top = r.top + 10.f;
                painter.Draw(CString(icon), D2D1::RectF(cx - IconBox / 2, top, cx + IconBox / 2, top + IconBox), 20.f, DWRITE_FONT_WEIGHT_NORMAL, ink,
                    DWRITE_TEXT_ALIGNMENT_CENTER, false, L"Segoe MDL2 Assets");
                painter.Draw(label, D2D1::RectF(r.left + TabMetrics::Padding / 2, top + IconBox + 2, r.right - TabMetrics::Padding / 2, r.bottom - 6.f), 14.f,
                    DWRITE_FONT_WEIGHT_MEDIUM, ink, DWRITE_TEXT_ALIGNMENT_CENTER);
            }
            else if (icon)
            {
                const float labelWidth = i < labelWidths.size() ? labelWidths[i] : 0.f;
                const float total = IconBox + (label.IsEmpty() ? 0.f : IconGap + labelWidth);
                const float left = (std::max)(r.left + 4.f, cx - total / 2), cy = (r.top + r.bottom) / 2;
                painter.Draw(CString(icon), D2D1::RectF(left, cy - IconBox / 2, left + IconBox, cy + IconBox / 2), 20.f, DWRITE_FONT_WEIGHT_NORMAL, ink,
                    DWRITE_TEXT_ALIGNMENT_CENTER, false, L"Segoe MDL2 Assets");
                painter.Draw(label, D2D1::RectF(left + IconBox + IconGap, r.top, r.right - 4.f, r.bottom), 14.f, DWRITE_FONT_WEIGHT_MEDIUM, ink);
            }
            else
                painter.Draw(label, D2D1::RectF(r.left + TabMetrics::Padding, r.top, r.right - TabMetrics::Padding, r.bottom), 14.f, DWRITE_FONT_WEIGHT_MEDIUM, ink,
                    DWRITE_TEXT_ALIGNMENT_CENTER);
            if (focus)
                target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(r.left + 3.f, r.top + 3.f, r.right - 3.f, r.bottom - 4.f), 8.f, 8.f), painter.Brush(p.primary), 2.f);
        }
        if (model.selected >= 0 && model.selected < static_cast<int>(layout.tabs.size()))
        {
            const auto color = enabled ? p.primary : Dim(p);
            const auto bar = layout.indicator;
            if (model.style == TabStyle::Primary)
                target->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bar.left, bar.top, bar.right, bar.bottom + 3.f), 3.f, 3.f), painter.Brush(color));
            else
                target->FillRectangle(bar, painter.Brush(color));
        }
        target->PopAxisAlignedClip();
    }

    // ------------------------------------------------------------ the window
    BOOL Tabs::Create(const RECT& bounds, CWnd* parent, UINT id, TabStyle style)
    {
        m_style = style;
        return CreateInput(InputKind::Tabs, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }

    int Tabs::AddTab(const CString& text, wchar_t icon)
    {
        m_labels.push_back(text);
        m_icons.push_back(icon);
        m_pages.push_back(nullptr);
        if (m_selected < 0) m_selected = 0;
        Remeasure();
        ApplyPages();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        return static_cast<int>(m_labels.size()) - 1;
    }
    void Tabs::RemoveAllTabs()
    {
        m_labels.clear(); m_icons.clear(); m_pages.clear();
        m_selected = m_hot = m_pressedTab = -1;
        m_scroll = 0;
        Remeasure();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    CString Tabs::GetTabText(int index) const { return index >= 0 && index < GetTabCount() ? m_labels[index] : CString(); }
    void Tabs::SetTabText(int index, const CString& text)
    {
        if (index < 0 || index >= GetTabCount()) return;
        m_labels[index] = text;
        Remeasure();
        ClampScroll();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void Tabs::SetStyle(TabStyle style)
    {
        m_style = style;
        Remeasure();
        if (m_hWnd) Invalidate(FALSE);
    }
    void Tabs::SetAccessibleName(const CString& name) { m_name = name; Publish(); }

    void Tabs::Remeasure()
    {
        m_intrinsic = MeasureTabWidths(m_labels, m_icons, m_style);
        m_labelWidths = MeasureLabelWidths(m_labels);
    }

    TabsLayout Tabs::CurrentLayout(float scroll) const
    {
        CRect client;
        if (m_hWnd) GetClientRect(client);
        return LayoutTabs(Dip(client.Width()), Dip(client.Height()), m_intrinsic, m_labelWidths, scroll, m_style, m_selected);
    }
    bool Tabs::IsScrollable() const { return CurrentLayout(0).scrollable; }
    float Tabs::MaximumScroll() const { return CurrentLayout(0).maxScroll; }
    void Tabs::ClampScroll() { m_scroll = Clamp(m_scroll, 0.f, MaximumScroll()); }
    void Tabs::RevealSelected()
    {
        if (!m_hWnd) return;
        CRect client;
        GetClientRect(client);
        m_scroll = TabScrollToReveal(CurrentLayout(0), m_selected, Dip(client.Width()), m_scroll);
    }
    int Tabs::PreferredHeight() const { return Px(TabsBarHeight(AnyIcon(m_icons), m_style)); }

    CRect Tabs::TabRect(int index) const
    {
        const auto layout = CurrentLayout(m_scroll);
        if (index < 0 || index >= static_cast<int>(layout.tabs.size())) return CRect();
        const auto& r = layout.tabs[index];
        return CRect(Px(r.left), Px(r.top), Px(r.right), Px(r.bottom));
    }
    CRect Tabs::IndicatorRect() const
    {
        const auto layout = CurrentLayout(m_scroll);
        const auto& r = layout.indicator;
        return CRect(Px(r.left), Px(r.top), Px(r.right), Px(r.bottom));
    }
    int Tabs::TabAtPoint(CPoint point) const
    {
        const auto layout = CurrentLayout(m_scroll);
        const float x = Dip(point.x), y = Dip(point.y);
        CRect client;
        if (m_hWnd) GetClientRect(client);
        if (y < 0 || y >= Dip(client.Height())) return -1;
        for (size_t i = 0; i < layout.tabs.size(); ++i)
            if (x >= layout.tabs[i].left && x < layout.tabs[i].right) return static_cast<int>(i);
        return -1;
    }

    CWnd* Tabs::GetBoundPage(int index) const { return index >= 0 && index < static_cast<int>(m_pages.size()) ? m_pages[index] : nullptr; }
    void Tabs::BindPage(int index, CWnd* page)
    {
        if (index < 0 || index >= GetTabCount()) return;
        m_pages[index] = page;
        ApplyPages();
    }
    void Tabs::ApplyPages()
    {
        const HWND focus = ::GetFocus();
        for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
        {
            CWnd* page = m_pages[i];
            if (!page || !page->GetSafeHwnd()) continue;
            if (i == m_selected) { page->ShowWindow(SW_SHOW); continue; }
            // A page that is about to be hidden must not keep the keyboard focus.
            if (focus && m_hWnd && (focus == page->m_hWnd || ::IsChild(page->m_hWnd, focus))) ::SetFocus(m_hWnd);
            page->ShowWindow(SW_HIDE);
        }
    }

    void Tabs::SetSelected(int index)
    {
        index = (std::max)(-1, (std::min)(index, GetTabCount() - 1));
        if (index == m_selected) return;
        m_selected = index;
        RevealSelected();
        ApplyPages();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }

    bool Tabs::Select(int index)
    {
        if (index < 0 || index >= GetTabCount() || index == m_selected || !m_hWnd) return false;
        CWnd* parent = GetParent();
        NMHDR header{ m_hWnd, static_cast<UINT_PTR>(GetDlgCtrlID()), TCN_SELCHANGING };
        if (parent && parent->SendMessage(WM_NOTIFY, header.idFrom, reinterpret_cast<LPARAM>(&header))) return false;   // vetoed
        m_selected = index;
        RevealSelected();
        ApplyPages();
        Publish();
        Invalidate(FALSE);
        if (parent && IsWindowEnabled())
        {
            header.code = TCN_SELCHANGE;
            parent->SendMessage(WM_NOTIFY, header.idFrom, reinterpret_cast<LPARAM>(&header));
        }
        return true;
    }

    bool Tabs::CycleSelection(bool backward)
    {
        const int count = GetTabCount();
        if (count < 2) return false;
        const int base = m_selected < 0 ? 0 : m_selected;
        return Select((base + (backward ? count - 1 : 1)) % count);
    }

    bool Tabs::HandleTabNavigation(UINT message, WPARAM key, bool ctrl, bool shift)
    {
        if (message != WM_KEYDOWN || !ctrl || !IsWindowEnabled() || GetTabCount() < 2) return false;
        if (key == VK_TAB) { CycleSelection(shift); return true; }
        if (key == VK_NEXT || key == VK_PRIOR) { CycleSelection(key == VK_PRIOR); return true; }
        return false;
    }
    bool Tabs::HandleTabNavigation(const MSG& message)
    {
        if (!m_hWnd || message.message != WM_KEYDOWN) return false;
        CWnd* parent = GetParent();
        const bool inside = message.hwnd == m_hWnd || (parent && ::IsChild(parent->m_hWnd, message.hwnd));
        if (!inside) return false;
        return HandleTabNavigation(message.message, message.wParam, (GetKeyState(VK_CONTROL) & 0x8000) != 0, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
    }

    void Tabs::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        TabsModel model;
        model.labels = m_labels; model.icons = m_icons;
        model.selected = m_selected; model.hot = m_hot; model.pressed = m_pressedTab;
        model.scroll = m_scroll; model.style = m_style;
        DrawTabs(target, palette, model, state);
    }
    CSize Tabs::MinimumLogicalSize() const
    {
        float sum = 0;
        for (float w : m_intrinsic) sum += w;
        return CSize(static_cast<int>((std::max)(TabMetrics::MinimumWidth, (std::min)(sum, 240.f))), static_cast<int>(TabsBarHeight(AnyIcon(m_icons), m_style)));
    }
    void Tabs::FillAccess(InputData& data) const
    {
        data.items.clear();
        for (const auto& label : m_labels) data.items.emplace_back(label.GetString());
        data.selected = m_selected;
        if (!m_name.IsEmpty()) data.name = m_name.GetString();
        data.text = m_selected >= 0 && m_selected < GetTabCount() ? m_labels[m_selected].GetString() : L"";
    }

    void Tabs::PointerPressed(CPoint point) { m_pressedTab = TabAtPoint(point); }
    void Tabs::PointerReleased(CPoint point, bool inside)
    {
        const int tab = TabAtPoint(point);
        const int pressed = m_pressedTab;
        m_pressedTab = -1;
        if (inside && tab >= 0 && tab == pressed) Select(tab);
    }
    void Tabs::PointerMoved(CPoint point) { m_hot = TabAtPoint(point); }
    void Tabs::PointerLeft() { m_hot = -1; }
    void Tabs::PointerCanceled() { m_pressedTab = -1; }
    bool Tabs::WantsKey(UINT key) const
    {
        return (key == VK_TAB || key == VK_NEXT || key == VK_PRIOR) && (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    }
    bool Tabs::KeyPressed(UINT key, bool alt)
    {
        const int count = GetTabCount();
        if (count == 0 || alt) return false;
        const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0, shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (ctrl) return HandleTabNavigation(WM_KEYDOWN, key, ctrl, shift);
        int target = -2;
        if (key == VK_LEFT) target = (std::max)(0, m_selected - 1);
        else if (key == VK_RIGHT) target = m_selected < 0 ? 0 : (std::min)(count - 1, m_selected + 1);
        else if (key == VK_HOME) target = 0;
        else if (key == VK_END) target = count - 1;
        if (target == -2) return false;
        Select(target);
        return true;
    }
    bool Tabs::MouseWheel(int delta)
    {
        if (!IsScrollable()) return false;
        m_scroll -= static_cast<float>(delta) / WHEEL_DELTA * TabMetrics::ScrollStep;
        ClampScroll();
        return true;
    }
    void Tabs::PerformAction(InputActionId action, int value) { if (action == InputActionId::Select) Select(value); }
    LRESULT Tabs::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        const LRESULT result = InputWindow::WindowProc(message, w, l);
        if (message == WM_SIZE && m_hWnd) { ClampScroll(); RevealSelected(); Invalidate(FALSE); }
        return result;
    }
}
