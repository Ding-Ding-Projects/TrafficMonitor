#include "../stdafx.h"
#include "InputControls.Internal.h"

// Switch, Checkbox and standalone RadioButton: single-glyph components with a label.
namespace NativeMaterial
{
    namespace
    {
        CSize ToggleMinimum(const CWnd& window)
        {
            CString label;
            window.GetWindowText(label);
            // Glyph column plus an approximate label width; the label ellipsizes when the window is narrower.
            return CSize(4 + 52 + 12 + (std::min)(label.GetLength(), 40) * 8 + 4, static_cast<int>(InputMetrics::MinimumTarget));
        }
        bool ActivationKey(UINT key) { return key == VK_SPACE; }
    }

    // ---------------------------------------------------------------- Switch
    BOOL Switch::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        return CreateInput(InputKind::Switch, label, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void Switch::SetChecked(bool checked)
    {
        if (m_checked == checked) return;
        m_checked = checked;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void Switch::SetLabel(const CString& label) { SetWindowText(label); Publish(); Invalidate(FALSE); }
    void Switch::SetShowIcon(bool show) { m_icon = show; Invalidate(FALSE); }
    void Switch::Toggle()
    {
        m_checked = !m_checked;
        Publish();
        Invalidate(FALSE);
        Notify(BN_CLICKED);
    }
    void Switch::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        CString label;
        GetWindowText(label);
        DrawSwitch(target, palette, label, m_checked, m_icon, state);
    }
    CSize Switch::MinimumLogicalSize() const { return ToggleMinimum(*this); }
    void Switch::FillAccess(InputData& data) const { data.toggle = m_checked ? 1 : 0; }
    void Switch::PointerPressed(CPoint) {}
    void Switch::PointerReleased(CPoint, bool inside) { if (inside) Toggle(); }
    bool Switch::KeyPressed(UINT key, bool) { if (!ActivationKey(key)) return false; m_pressed = true; return true; }
    bool Switch::KeyReleased(UINT key)
    {
        if (!ActivationKey(key) || !m_pressed) return false;
        m_pressed = false;
        Toggle();
        return true;
    }
    void Switch::PerformAction(InputActionId action, int) { if (action == InputActionId::Toggle) Toggle(); }

    // -------------------------------------------------------------- Checkbox
    BOOL Checkbox::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        return CreateInput(InputKind::Checkbox, label, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void Checkbox::SetCheckState(CheckState state)
    {
        if (m_state == state) return;
        m_state = state;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void Checkbox::SetLabel(const CString& label) { SetWindowText(label); Publish(); Invalidate(FALSE); }
    void Checkbox::Toggle()
    {
        m_state = m_state == CheckState::Checked ? CheckState::Unchecked : CheckState::Checked;
        Publish();
        Invalidate(FALSE);
        Notify(BN_CLICKED);
    }
    void Checkbox::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        CString label;
        GetWindowText(label);
        DrawCheckbox(target, palette, label, m_state, state);
    }
    CSize Checkbox::MinimumLogicalSize() const { return ToggleMinimum(*this); }
    void Checkbox::FillAccess(InputData& data) const
    {
        data.toggle = m_state == CheckState::Checked ? 1 : m_state == CheckState::Indeterminate ? 2 : 0;
    }
    // The whole client rectangle, label included, is the target.
    void Checkbox::PointerPressed(CPoint) {}
    void Checkbox::PointerReleased(CPoint, bool inside) { if (inside) Toggle(); }
    bool Checkbox::KeyPressed(UINT key, bool) { if (!ActivationKey(key)) return false; m_pressed = true; return true; }
    bool Checkbox::KeyReleased(UINT key)
    {
        if (!ActivationKey(key) || !m_pressed) return false;
        m_pressed = false;
        Toggle();
        return true;
    }
    void Checkbox::PerformAction(InputActionId action, int) { if (action == InputActionId::Toggle) Toggle(); }

    // ----------------------------------------------------------- RadioButton
    BOOL RadioButton::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id, bool startsGroup)
    {
        const DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | (startsGroup ? WS_GROUP : 0);
        if (!CreateInput(InputKind::Radio, label, style, 0, bounds, parent, id)) return FALSE;
        RefreshTabStops();
        return TRUE;
    }
    std::vector<RadioButton*> RadioButton::Group() const
    {
        std::vector<RadioButton*> group;
        if (!m_hWnd || !::GetParent(m_hWnd)) return group;
        HWND start = m_hWnd;
        while (!(::GetWindowLongPtr(start, GWL_STYLE) & WS_GROUP))
        {
            HWND previous = ::GetWindow(start, GW_HWNDPREV);
            if (!previous) break;
            start = previous;
        }
        for (HWND current = start; current; current = ::GetWindow(current, GW_HWNDNEXT))
        {
            if (current != start && (::GetWindowLongPtr(current, GWL_STYLE) & WS_GROUP)) break;
            if (auto* radio = dynamic_cast<RadioButton*>(CWnd::FromHandlePermanent(current))) group.push_back(radio);
        }
        return group;
    }
    void RadioButton::RefreshTabStops()
    {
        const auto group = Group();
        RadioButton* stop = nullptr;
        for (auto* radio : group) if (radio->m_checked) stop = radio;
        if (!stop && !group.empty()) stop = group.front();
        for (auto* radio : group) radio->ModifyStyle(radio == stop ? 0 : WS_TABSTOP, radio == stop ? WS_TABSTOP : 0);
    }
    void RadioButton::SetChecked(bool checked)
    {
        if (checked)
        {
            for (auto* other : Group())
                if (other != this && other->m_checked) { other->m_checked = false; other->Publish(); other->Invalidate(FALSE); }
        }
        if (m_checked != checked) { m_checked = checked; Publish(); Invalidate(FALSE); }
        RefreshTabStops();
    }
    void RadioButton::SetLabel(const CString& label) { SetWindowText(label); Publish(); Invalidate(FALSE); }
    void RadioButton::Choose(bool notify)
    {
        SetChecked(true);
        if (notify) Notify(BN_CLICKED);
    }
    void RadioButton::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        CString label;
        GetWindowText(label);
        DrawRadio(target, palette, label, m_checked, state);
    }
    CSize RadioButton::MinimumLogicalSize() const { return ToggleMinimum(*this); }
    void RadioButton::FillAccess(InputData& data) const { data.toggle = m_checked ? 1 : 0; }
    void RadioButton::PointerPressed(CPoint) {}
    void RadioButton::PointerReleased(CPoint, bool inside) { if (inside) Choose(true); }
    bool RadioButton::KeyPressed(UINT key, bool)
    {
        if (ActivationKey(key)) { m_pressed = true; return true; }
        int step = 0;
        if (key == VK_LEFT || key == VK_UP) step = -1;
        else if (key == VK_RIGHT || key == VK_DOWN) step = 1;
        if (!step) return false;
        const auto group = Group();
        const int count = static_cast<int>(group.size());
        int index = -1;
        for (int i = 0; i < count; ++i) if (group[i] == this) index = i;
        if (index < 0) return true;
        // Skip disabled radios; wrap around the group.
        for (int hop = 1; hop <= count; ++hop)
        {
            auto* next = group[(index + step * hop % count + count) % count];
            if (next != this && next->IsWindowEnabled()) { next->SetFocus(); next->Choose(true); break; }
        }
        return true;
    }
    bool RadioButton::KeyReleased(UINT key)
    {
        if (!ActivationKey(key) || !m_pressed) return false;
        m_pressed = false;
        Choose(true);
        return true;
    }
    void RadioButton::PerformAction(InputActionId action, int)
    {
        if (action == InputActionId::Select || action == InputActionId::Toggle) Choose(true);
    }
}
