#include "../stdafx.h"
#include "InputControls.Internal.h"

namespace NativeMaterial
{
    // ------------------------------------------------------------ ClearButton
    BOOL ClearButton::Create(const RECT& bounds, CWnd* parent, UINT id)
    {
        const CString name = NativePreferences::Pair(L"Clear", L"清除");
        return CreateInput(InputKind::Button, name, WS_CHILD | WS_TABSTOP, 0, bounds, parent, id);
    }
    void ClearButton::SetAccessibleName(const CString& name)
    {
        if (!m_hWnd) return;
        SetWindowText(name);
        Publish();
    }
    void ClearButton::SetFieldFocused(bool focused)
    {
        if (m_fieldFocused == focused) return;
        m_fieldFocused = focused;
        if (m_hWnd) Invalidate(FALSE);
    }
    void ClearButton::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        DrawClearButton(target, palette, m_fieldFocused, state);
    }
    CSize ClearButton::MinimumLogicalSize() const { return CSize(static_cast<int>(InputMetrics::StepperButton), static_cast<int>(InputMetrics::StepperButton)); }
    void ClearButton::FillAccess(InputData&) const {}
    void ClearButton::PointerPressed(CPoint) {}
    void ClearButton::PointerReleased(CPoint, bool inside) { if (inside) Notify(BN_CLICKED); }
    bool ClearButton::KeyPressed(UINT key, bool) { if (key != VK_SPACE && key != VK_RETURN) return false; m_pressed = true; return true; }
    bool ClearButton::KeyReleased(UINT key)
    {
        if ((key != VK_SPACE && key != VK_RETURN) || !m_pressed) return false;
        m_pressed = false;
        Notify(BN_CLICKED);
        return true;
    }
    void ClearButton::PerformAction(InputActionId action, int) { if (action == InputActionId::Toggle) Notify(BN_CLICKED); }

    // -------------------------------------------------------- LabeledTextField
    namespace { constexpr UINT FieldId = 1, ClearId = 2; }

    BOOL LabeledTextField::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_label = label;
        if (!CreateInput(InputKind::TextField, DisplayLabel(label), WS_CHILD | WS_VISIBLE, WS_EX_CONTROLPARENT, bounds, parent, id)) return FALSE;
        if (!m_field.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, CRect(0, 0, 10, 10), this, FieldId)) return FALSE;
        if (!m_clear.Create(CRect(0, 0, 10, 10), this, ClearId)) return FALSE;
        CreateInputFont(m_font, Dpi());
        m_field.SetFont(&m_font);
        m_field.SetAppearance(GetAppearance());
        m_clear.SetAppearance(GetAppearance());
        AnnotateAccessibleName(m_field.GetSafeHwnd(), DisplayLabel(label));
        UpdateClearName();
        Layout();
        UpdateClear();
        Publish();
        return TRUE;
    }

    void LabeledTextField::Layout()
    {
        if (!m_field.GetSafeHwnd()) return;
        CRect client;
        GetClientRect(client);
        const UINT dpi = Dpi();
        const auto l = LayoutLabeledField(Dip(client.Width()), Dip(client.Height()));
        auto px = [&](D2D1_RECT_F r) { return CRect(DipToPx(r.left, dpi), DipToPx(r.top, dpi), DipToPx(r.right, dpi), DipToPx(r.bottom, dpi)); };
        const CRect edit = px(l.edit), clear = px(l.clear);
        m_field.MoveWindow(edit);
        m_clear.MoveWindow(clear);
        // The clear button overlaps the last few pixels of the field so the bottom line stays continuous.
        m_clear.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    void LabeledTextField::DpiChanged()
    {
        CreateInputFont(m_font, Dpi());
        m_field.SetFont(&m_font);
        Layout();
    }
    void LabeledTextField::AppearanceChanged()
    {
        if (m_field.GetSafeHwnd()) m_field.SetAppearance(GetAppearance());
        if (m_clear.GetSafeHwnd()) m_clear.SetAppearance(GetAppearance());
    }
    void LabeledTextField::UpdateClear()
    {
        if (!m_clear.GetSafeHwnd() || !m_field.GetSafeHwnd()) return;
        const bool show = m_field.GetWindowTextLength() > 0 && IsWindowEnabled() && !(m_field.GetStyle() & ES_READONLY);
        m_clear.ShowWindow(show ? SW_SHOWNA : SW_HIDE);
        // ShowWindow on a child of an unshown parent reports invisible; track the logical state as well.
        m_clearShown = show;
    }

    void LabeledTextField::SetLabel(const CString& label)
    {
        m_label = label;
        SetWindowText(DisplayLabel(label));
        AnnotateAccessibleName(m_field.GetSafeHwnd(), DisplayLabel(label));
        UpdateClearName();
        Publish();
        Invalidate(FALSE);
    }
    CString LabeledTextField::DisplayLabel(const CString& label)
    {
        CString shown;
        for (int i = 0; i < label.GetLength(); ++i)
        {
            if (label[i] == L'&' && i + 1 < label.GetLength()) ++i;   // "&x" keeps x, "&&" keeps one ampersand
            else if (label[i] == L'&') continue;
            shown += label[i];
        }
        return shown;
    }
    int LabeledTextField::MnemonicIndex() const
    {
        int shown = 0;
        for (int i = 0; i < m_label.GetLength(); ++i)
        {
            if (m_label[i] == L'&' && i + 1 < m_label.GetLength())
            {
                ++i;
                if (m_label[i] != L'&') return shown;
            }
            else if (m_label[i] == L'&') continue;
            ++shown;
        }
        return -1;
    }
    wchar_t LabeledTextField::Mnemonic() const
    {
        const int index = MnemonicIndex();
        if (index < 0) return 0;
        const CString shown = DisplayLabel(m_label);
        return index < shown.GetLength() ? shown[index] : 0;
    }
    bool LabeledTextField::ActivateMnemonic(wchar_t ch)
    {
        const wchar_t mnemonic = Mnemonic();
        if (!mnemonic || !m_field.GetSafeHwnd() || !IsWindowVisible() || !IsWindowEnabled()) return false;
        if (CharUpperW(reinterpret_cast<LPWSTR>(static_cast<ULONG_PTR>(ch))) != CharUpperW(reinterpret_cast<LPWSTR>(static_cast<ULONG_PTR>(mnemonic)))) return false;
        m_field.SetFocus();
        return true;
    }
    void LabeledTextField::UpdateClearName()
    {
        if (m_clear.GetSafeHwnd())
            m_clear.SetAccessibleName(NativePreferences::Format(L"Clear %s", L"清除%s", DisplayLabel(m_label).GetString()));
    }
    void LabeledTextField::SetText(const CString& text)
    {
        m_updating = true;
        m_field.SetWindowText(text);
        m_updating = false;
        UpdateClear();
        Publish();
    }
    CString LabeledTextField::GetText() const
    {
        CString text;
        m_field.GetWindowText(text);
        return text;
    }
    void LabeledTextField::SetSupportingText(const CString& text) { m_supporting = text; Publish(); Invalidate(FALSE); }
    void LabeledTextField::SetErrorText(const CString& text)
    {
        m_error = text;
        SetError(!text.IsEmpty());
        Publish();
        Invalidate(FALSE);
    }
    void LabeledTextField::Clear()
    {
        // SetWindowText raises EN_CHANGE, which this window forwards exactly like typing would.
        if (m_field.GetWindowTextLength() > 0) m_field.SetWindowText(L"");
        m_field.SetFocus();
    }
    bool LabeledTextField::IsFocused() const
    {
        const HWND focus = ::GetFocus();
        return focus && (focus == m_field.GetSafeHwnd() || focus == m_clear.GetSafeHwnd());
    }

    void LabeledTextField::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        // Keyboard cues (the mnemonic underline) follow the shared Windows UI state, hidden until Alt or Tab is used.
        const bool cues = (SendMessage(WM_QUERYUISTATE) & UISF_HIDEACCEL) == 0;
        DrawLabeledFieldChrome(target, palette, DisplayLabel(m_label), !m_error.IsEmpty() ? m_error : m_supporting, state, cues ? MnemonicIndex() : -1);
    }
    CSize LabeledTextField::MinimumLogicalSize() const { return CSize(160, 86); }
    void LabeledTextField::FillAccess(InputData& data) const
    {
        data.name = DisplayLabel(m_label).GetString();
        data.help = (!m_error.IsEmpty() ? m_error : m_supporting).GetString();
    }

    LRESULT LabeledTextField::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == WM_COMMAND && m_field.GetSafeHwnd())
        {
            const UINT id = LOWORD(w), code = HIWORD(w);
            if (id == FieldId && code == EN_CHANGE)
            {
                UpdateClear();
                if (!m_updating) Notify(EN_CHANGE);
                return 0;
            }
            if (id == FieldId && (code == EN_SETFOCUS || code == EN_KILLFOCUS))
            {
                m_clear.SetFieldFocused(code == EN_SETFOCUS);
                Invalidate(FALSE);
                // Lets a scrolling host bring the field into view when it is focused by mouse as well as by Tab.
                if (code == EN_SETFOCUS) Notify(EN_SETFOCUS);
                return 0;
            }
            if (id == ClearId && code == BN_CLICKED) { Clear(); return 0; }
        }
        const auto result = InputWindow::WindowProc(message, w, l);
        if (message == WM_SIZE) Layout();
        if (message == WM_UPDATEUISTATE) Invalidate(FALSE);
        if (message == WM_ENABLE && m_field.GetSafeHwnd()) { m_field.EnableWindow(w != 0); m_clear.EnableWindow(w != 0); UpdateClear(); }
        return result;
    }
}
