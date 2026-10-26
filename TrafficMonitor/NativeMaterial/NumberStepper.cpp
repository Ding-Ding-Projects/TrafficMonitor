#include "../stdafx.h"
#include "InputControls.Internal.h"

namespace NativeMaterial
{
    // The only child of the stepper: a borderless single-line EDIT that keeps native IME, selection,
    // clipboard and undo. Step keys and Enter/Escape are routed to the owning stepper.
    class StepperEdit final : public CEdit
    {
    public:
        NumberStepper* owner{};
    protected:
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
        {
            if (owner)
            {
                if (message == WM_KEYDOWN && owner->EditKey(static_cast<UINT>(w))) return 0;
                if (message == WM_CHAR && (w == VK_RETURN || w == VK_ESCAPE)) return 0;
                if (message == WM_GETDLGCODE)
                {
                    auto result = CEdit::WindowProc(message, w, l);
                    const auto* key = reinterpret_cast<const MSG*>(l);
                    const UINT code = key ? static_cast<UINT>(key->wParam) : static_cast<UINT>(w);
                    if (code == VK_RETURN || code == VK_ESCAPE) result |= DLGC_WANTMESSAGE;
                    return result;
                }
                if (message == WM_SETFOCUS || message == WM_KILLFOCUS)
                {
                    const auto result = CEdit::WindowProc(message, w, l);
                    owner->EditFocus(message == WM_SETFOCUS);
                    return result;
                }
            }
            return CEdit::WindowProc(message, w, l);
        }
    };

    NumberStepper::NumberStepper() = default;
    NumberStepper::~NumberStepper() = default;

    BOOL NumberStepper::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_label = label;
        if (!CreateInput(InputKind::Stepper, label, WS_CHILD | WS_VISIBLE, WS_EX_CONTROLPARENT, bounds, parent, id)) return FALSE;
        m_edit = std::make_unique<StepperEdit>();
        m_edit->owner = this;
        if (!m_edit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_CENTER | ES_AUTOHSCROLL, CRect(0, 0, 10, 10), this, 1)) return FALSE;
        AppearanceChanged();
        CreateInputFont(m_font, Dpi());
        Layout();
        ShowValue();
        AnnotateAccessibleName(m_edit->GetSafeHwnd(), label);
        Publish();
        return TRUE;
    }

    HWND NumberStepper::EditWindow() const { return m_edit ? m_edit->GetSafeHwnd() : nullptr; }

    void NumberStepper::AppearanceChanged()
    {
        m_brush.DeleteObject();
        m_brush.CreateSolidBrush(ToColorRef(Palette::Resolve(GetAppearance()).surface));
        if (m_edit && m_edit->GetSafeHwnd()) m_edit->Invalidate();
    }

    void NumberStepper::Layout()
    {
        if (!m_edit || !m_edit->GetSafeHwnd()) return;
        CRect client;
        GetClientRect(client);
        const auto layout = LayoutStepper(Dip(client.Width()), Dip(client.Height()));
        PlaceCenteredEdit(*m_edit, m_font, layout.edit, Dpi());
    }
    void NumberStepper::DpiChanged()
    {
        CreateInputFont(m_font, Dpi());
        Layout();
    }
    void NumberStepper::ShowValue()
    {
        if (!m_edit || !m_edit->GetSafeHwnd()) return;
        CString text;
        text.Format(L"%d", m_value);
        m_updating = true;
        m_edit->SetWindowText(text);
        m_updating = false;
    }

    void NumberStepper::SetRange(int minimum, int maximum)
    {
        if (minimum > maximum) std::swap(minimum, maximum);
        m_minimum = minimum; m_maximum = maximum;
        m_value = (std::max)(m_minimum, (std::min)(m_maximum, m_value));
        ShowValue();
        Validate();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void NumberStepper::SetStep(int step) { m_step = (std::max)(1, step); Publish(); }
    void NumberStepper::SetValue(int value)
    {
        value = (std::max)(m_minimum, (std::min)(m_maximum, value));
        m_value = value;
        ShowValue();
        m_validation.Empty();
        SetError(false);
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void NumberStepper::SetLabel(const CString& label)
    {
        m_label = label;
        SetWindowText(label);
        if (m_edit) AnnotateAccessibleName(m_edit->GetSafeHwnd(), label);
        Publish();
        Invalidate(FALSE);
    }
    void NumberStepper::SetSupportingText(const CString& text) { m_supporting = text; Publish(); if (m_hWnd) Invalidate(FALSE); }

    bool NumberStepper::ParseInteger(const CString& text, long long& value)
    {
        CString trimmed(text);
        trimmed.Trim();
        if (trimmed.IsEmpty() || trimmed.GetLength() > 12) return false;
        int index = 0;
        bool negative = false;
        if (trimmed[0] == L'-' || trimmed[0] == L'+') { negative = trimmed[0] == L'-'; index = 1; }
        if (index >= trimmed.GetLength()) return false;
        long long result = 0;
        for (; index < trimmed.GetLength(); ++index)
        {
            const wchar_t c = trimmed[index];
            if (c < L'0' || c > L'9') return false;
            result = result * 10 + (c - L'0');
        }
        value = negative ? -result : result;
        return true;
    }

    void NumberStepper::Validate()
    {
        if (!m_edit || !m_edit->GetSafeHwnd()) return;
        CString text;
        m_edit->GetWindowText(text);
        long long parsed = 0;
        CString message;
        if (!ParseInteger(text, parsed))
            message = NativePreferences::Pair(L"Enter a whole number.", L"請輸入整數。");
        else if (parsed < m_minimum || parsed > m_maximum)
            message = NativePreferences::Format(L"Enter a value from %d to %d.", L"請輸入 %d 至 %d 之間嘅數值。", m_minimum, m_maximum);
        m_validation = message;
        SetError(!message.IsEmpty());
    }

    void NumberStepper::EditChanged()
    {
        if (m_updating) return;
        Validate();
        Publish();
        Invalidate(FALSE);
    }

    bool NumberStepper::CommitText()
    {
        Validate();
        if (!m_validation.IsEmpty()) { Publish(); Invalidate(FALSE); return false; }
        CString text;
        m_edit->GetWindowText(text);
        long long parsed = 0;
        ParseInteger(text, parsed);
        const bool changed = static_cast<int>(parsed) != m_value;
        m_value = static_cast<int>(parsed);
        ShowValue();
        Publish();
        Invalidate(FALSE);
        if (changed) Notify(EN_CHANGE);
        return true;
    }

    void NumberStepper::Revert()
    {
        ShowValue();
        m_validation.Empty();
        SetError(false);
        Publish();
        Invalidate(FALSE);
    }

    bool NumberStepper::StepBy(int delta)
    {
        const long long wanted = static_cast<long long>(m_value) + static_cast<long long>(delta) * m_step;
        const int next = static_cast<int>((std::max)(static_cast<long long>(m_minimum), (std::min)(static_cast<long long>(m_maximum), wanted)));
        const bool changed = next != m_value;
        m_value = next;
        ShowValue();
        m_validation.Empty();
        SetError(false);
        Publish();
        Invalidate(FALSE);
        if (changed) Notify(EN_CHANGE);
        return changed;
    }

    bool NumberStepper::EditKey(UINT key)
    {
        switch (key)
        {
        case VK_UP: CommitText(); StepBy(1); return true;
        case VK_DOWN: CommitText(); StepBy(-1); return true;
        case VK_PRIOR: CommitText(); StepBy(10); return true;
        case VK_NEXT: CommitText(); StepBy(-10); return true;
        case VK_RETURN: CommitText(); return true;
        case VK_ESCAPE: Revert(); return true;
        default: return false;
        }
    }
    bool NumberStepper::KeyPressed(UINT key, bool) { return EditKey(key); }

    void NumberStepper::EditFocus(bool focused)
    {
        if (!focused)
        {
            // Leaving the field keeps a valid typed value and discards an invalid one.
            if (m_validation.IsEmpty()) CommitText(); else Revert();
        }
        Invalidate(FALSE);
    }
    bool NumberStepper::IsFocused() const { return m_edit && m_edit->GetSafeHwnd() && ::GetFocus() == m_edit->GetSafeHwnd(); }

    StepperPart NumberStepper::PartAt(CPoint point) const
    {
        CRect client;
        GetClientRect(client);
        const auto layout = LayoutStepper(Dip(client.Width()), Dip(client.Height()));
        const float x = Dip(point.x), y = Dip(point.y);
        auto inside = [&](D2D1_RECT_F r) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; };
        if (inside(layout.minus)) return StepperPart::Minus;
        if (inside(layout.plus)) return StepperPart::Plus;
        return StepperPart::None;
    }
    void NumberStepper::PointerPressed(CPoint point)
    {
        m_pressedPart = PartAt(point);
        if (m_pressedPart == StepperPart::None && m_edit) m_edit->SetFocus();
    }
    void NumberStepper::PointerReleased(CPoint point, bool inside)
    {
        const auto part = PartAt(point);
        if (inside && part != StepperPart::None && part == m_pressedPart)
        {
            if (m_edit) m_edit->SetFocus();
            CommitText();
            StepBy(part == StepperPart::Plus ? 1 : -1);
        }
        m_pressedPart = StepperPart::None;
    }
    void NumberStepper::PointerMoved(CPoint point) { m_hoverPart = PartAt(point); }
    void NumberStepper::PointerLeft() { m_hoverPart = StepperPart::None; }

    void NumberStepper::PerformAction(InputActionId action, int value)
    {
        if (action != InputActionId::SetValue || value < m_minimum || value > m_maximum) return;
        const bool changed = value != m_value;
        SetValue(value);
        if (changed) Notify(EN_CHANGE);
    }

    void NumberStepper::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        const CString& message = !m_validation.IsEmpty() ? m_validation : m_supporting;
        InputState drawn = state;
        drawn.error = !m_validation.IsEmpty();
        DrawStepper(target, palette, m_label, message, m_value > m_minimum, m_value < m_maximum, m_hoverPart, m_pressedPart, drawn);
    }
    CSize NumberStepper::MinimumLogicalSize() const
    {
        return CSize(168, static_cast<int>(InputMetrics::FieldTop + InputMetrics::FieldHeight + InputMetrics::MessageHeight));
    }
    void NumberStepper::FillAccess(InputData& data) const
    {
        data.value = m_value; data.minimum = m_minimum; data.maximum = m_maximum; data.step = m_step;
        data.name = m_label.GetString();
        data.help = (!m_validation.IsEmpty() ? m_validation : m_supporting).GetString();
        data.text = std::to_wstring(m_value);
    }

    LRESULT NumberStepper::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (m_edit && m_edit->GetSafeHwnd())
        {
            if (message == WM_COMMAND && HIWORD(w) == EN_CHANGE && reinterpret_cast<HWND>(l) == m_edit->GetSafeHwnd()) { EditChanged(); return 0; }
            if ((message == WM_CTLCOLOREDIT || message == WM_CTLCOLORSTATIC) && reinterpret_cast<HWND>(l) == m_edit->GetSafeHwnd())
            {
                const auto palette = Palette::Resolve(GetAppearance());
                auto* dc = CDC::FromHandle(reinterpret_cast<HDC>(w));
                dc->SetTextColor(ToColorRef(IsWindowEnabled() ? palette.onSurface : palette.secondaryText));
                dc->SetBkColor(ToColorRef(palette.surface));
                return reinterpret_cast<LRESULT>(m_brush.GetSafeHandle());
            }
            if (message == WM_ENABLE) m_edit->EnableWindow(w != 0);
        }
        const auto result = InputWindow::WindowProc(message, w, l);
        if (message == WM_SIZE) Layout();
        return result;
    }
}
