#pragma once
#include <afxwin.h>
#include <functional>
#include <memory>
#include <vector>
#include "Theme.h"
#include "InputVisuals.h"
#include "InputControls.Accessibility.h"
#include "TextField.h"
#include "../NativeRegex/Engine.h"

// Self-drawn Material 3 input components. None of them wraps or subclasses a
// stock BUTTON, COMBOBOX, SPIN or TRACKBAR window. Each is a real child HWND
// painted with Direct2D (like NativeMaterial::Button), DPI aware per monitor,
// themed through Palette, keyboard operable and exposed through UI Automation.
//
// Notifications reach the parent as WM_COMMAND with the control id:
//   Switch, Checkbox, RadioButton          BN_CLICKED after the user changes the state
//   RadioGroup, SegmentedButton, Dropdown  CBN_SELCHANGE after the user changes the selection
//   NumberStepper, LabeledTextField        EN_CHANGE after the committed value / text changes
// Programmatic setters never notify, so a handler can set values without reentry.
namespace NativeMaterial
{
    class InputWindow : public CWnd
    {
    public:
        InputWindow();
        ~InputWindow() override;
        void SetAppearance(Appearance appearance);
        Appearance GetAppearance() const { return m_appearance; }
        void SetSurfaceColor(COLORREF color);
        // Smallest recommended window size in physical pixels at the current DPI.
        CSize MinimumSize() const;
        UINT Dpi() const;
        // Paints the whole component into a device context (also the WM_PRINTCLIENT path).
        HRESULT PaintTo(HDC dc, bool printing = false);
        // A UI Automation provider bound to this control (the caller releases it). Used by the verification
        // to read patterns directly, without a client round trip through the message loop.
        HRESULT AutomationProvider(IRawElementProviderSimple** provider) const;
        // Numeric diagnostics: 0 attach, 1 paint, 2 print (same convention as Button, WM_APP+0x521).
        HRESULT RenderStatus(int field) const { return field == 0 ? m_attachResult : field == 1 ? m_drawResult : m_printResult; }
    protected:
        BOOL CreateInput(InputKind kind, const CString& text, DWORD style, DWORD exStyle, const RECT& bounds, CWnd* parent, UINT id);
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
        virtual void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) = 0;
        virtual CSize MinimumLogicalSize() const = 0;                 // 96-dpi units
        virtual void FillAccess(InputData&) const;
        virtual void PointerPressed(CPoint) {}
        virtual void PointerReleased(CPoint, bool /*inside*/) {}
        virtual void PointerMoved(CPoint) {}
        virtual void PointerLeft() {}
        virtual void PointerCanceled() {}
        virtual void AppearanceChanged() {}
        virtual bool WantsKey(UINT /*virtualKey*/) const { return false; }
        virtual bool KeyPressed(UINT, bool /*alt*/) { return false; }
        virtual bool KeyReleased(UINT) { return false; }
        virtual bool CharTyped(UINT) { return false; }
        virtual bool MouseWheel(int /*delta*/) { return false; }
        virtual UINT DialogCode() const { return DLGC_WANTARROWS; }
        virtual void DpiChanged() {}
        virtual bool IsFocused() const { return GetFocus() == this; }
        virtual void PerformAction(InputActionId, int) {}
        // Layout positions are in 96-dpi units; Px and Dip convert with the window DPI.
        int Px(float dip) const;
        float Dip(int px) const;
        float DipX(const CPoint& point) const { return Dip(point.x); }
        InputState State() const;
        void Notify(UINT code);
        void Publish();
        void SetError(bool error);
        bool HasError() const { return m_error; }
        bool m_hover{}, m_pressed{};
    private:
        afx_msg void OnPaint();
        afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
        afx_msg void OnLButtonDown(UINT, CPoint);
        afx_msg void OnLButtonUp(UINT, CPoint);
        afx_msg void OnMouseMove(UINT, CPoint);
        afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
        afx_msg void OnCaptureChanged(CWnd*);
        afx_msg void OnKeyDown(UINT, UINT, UINT);
        afx_msg void OnSysKeyDown(UINT, UINT, UINT);
        afx_msg void OnKeyUp(UINT, UINT, UINT);
        afx_msg void OnChar(UINT, UINT, UINT);
        afx_msg BOOL OnMouseWheel(UINT, short, CPoint);
        afx_msg void OnSetFocus(CWnd*);
        afx_msg void OnKillFocus(CWnd*);
        afx_msg void OnEnable(BOOL);
        afx_msg void OnDestroy();
        afx_msg UINT OnGetDlgCode();
        afx_msg LRESULT OnGetObject(WPARAM, LPARAM);
        afx_msg LRESULT OnInputAction(WPARAM, LPARAM);
        afx_msg LRESULT OnPrintClient(WPARAM, LPARAM);
        afx_msg LRESULT OnDpiAfterParent(WPARAM, LPARAM);
        DECLARE_MESSAGE_MAP()
        void RaiseChanges(const InputData& before, const InputData& after);
        Appearance m_appearance{ Appearance::Light };
        bool m_customSurface{}, m_error{};
        COLORREF m_surfaceColor{};
        HRESULT m_attachResult{ E_PENDING }, m_drawResult{ E_PENDING }, m_printResult{ E_PENDING };
        std::shared_ptr<InputAccess> m_access;
        InputKind m_kind{};
    };

    // Material switch: 52 x 32 track, moving handle, optional icon in the handle.
    class Switch final : public InputWindow
    {
    public:
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void SetChecked(bool checked);
        bool IsChecked() const { return m_checked; }
        void SetLabel(const CString& label);
        void SetShowIcon(bool show);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        bool KeyPressed(UINT, bool) override;
        bool KeyReleased(UINT) override;
        UINT DialogCode() const override { return DLGC_BUTTON; }
        void PerformAction(InputActionId, int) override;
    private:
        void Toggle();
        bool m_checked{}, m_icon{};
    };

    class Checkbox final : public InputWindow
    {
    public:
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void SetCheckState(CheckState state);
        CheckState GetCheckState() const { return m_state; }
        void SetChecked(bool checked) { SetCheckState(checked ? CheckState::Checked : CheckState::Unchecked); }
        bool IsChecked() const { return m_state == CheckState::Checked; }
        void SetLabel(const CString& label);
        void SetErrorState(bool error) { SetError(error); Publish(); Invalidate(FALSE); }
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        bool KeyPressed(UINT, bool) override;
        bool KeyReleased(UINT) override;
        UINT DialogCode() const override { return DLGC_BUTTON; }
        void PerformAction(InputActionId, int) override;
    private:
        void Toggle();
        CheckState m_state{ CheckState::Unchecked };
    };

    // Standalone radio. Radios between two WS_GROUP siblings form a group: choosing one clears the others,
    // arrow keys move within the group and only one radio per group is a tab stop.
    class RadioButton final : public InputWindow
    {
    public:
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id, bool startsGroup = false);
        void SetChecked(bool checked);
        bool IsChecked() const { return m_checked; }
        void SetLabel(const CString& label);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        bool KeyPressed(UINT, bool) override;
        bool KeyReleased(UINT) override;
        UINT DialogCode() const override { return DLGC_RADIOBUTTON | DLGC_WANTARROWS; }
        void PerformAction(InputActionId, int) override;
    private:
        std::vector<RadioButton*> Group() const;
        void Choose(bool notify);
        void RefreshTabStops();
        bool m_checked{};
    };

    // One window, one tab stop, N radio options stacked vertically.
    class RadioGroup final : public InputWindow
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        void SetOptions(const std::vector<CString>& options);
        void AddOption(const CString& option);
        int GetOptionCount() const { return static_cast<int>(m_options.size()); }
        void SetSelected(int index);
        int GetSelected() const { return m_selected; }
        void SetAccessibleName(const CString& name);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        bool KeyPressed(UINT, bool) override;
        void PerformAction(InputActionId, int) override;
    private:
        int RowAt(CPoint) const;
        void Choose(int index);
        std::vector<CString> m_options;
        int m_selected{ -1 }, m_hoverRow{ -1 }, m_pressedRow{ -1 };
        CString m_name;
    };

    // Two to five segments, single selection, check icon on the selected segment.
    class SegmentedButton final : public InputWindow
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        // Returns false and changes nothing unless 2 to 5 labels are given.
        bool SetSegments(const std::vector<CString>& labels);
        int GetSegmentCount() const { return static_cast<int>(m_segments.size()); }
        void SetSelected(int index);
        int GetSelected() const { return m_selected; }
        void SetAccessibleName(const CString& name);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        bool KeyPressed(UINT, bool) override;
        void PerformAction(InputActionId, int) override;
    private:
        int SegmentAt(CPoint) const;
        void Choose(int index);
        std::vector<CString> m_segments;
        int m_selected{ -1 }, m_hoverSegment{ -1 }, m_pressedSegment{ -1 };
        CString m_name;
    };

    class StepperEdit;
    // Outlined integer field with minus and plus buttons. Text entry uses a borderless EDIT inside the drawn
    // field so IME, selection and the clipboard keep working. Up/Down step, PageUp/PageDown step by ten,
    // Enter commits typed text, Escape reverts it.
    class NumberStepper final : public InputWindow
    {
    public:
        NumberStepper();
        ~NumberStepper() override;
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void SetRange(int minimum, int maximum);
        void SetStep(int step);
        // Clamps into the range; does not notify.
        void SetValue(int value);
        int GetValue() const { return m_value; }
        int GetMinimum() const { return m_minimum; }
        int GetMaximum() const { return m_maximum; }
        int GetStep() const { return m_step; }
        void SetLabel(const CString& label);
        void SetSupportingText(const CString& text);
        // Message shown for the latest typed text that is not a whole number inside the range; empty when valid.
        CString GetValidationMessage() const { return m_validation; }
        HWND EditWindow() const;
        // Applies the typed text exactly as Enter does. Returns false when the text is rejected.
        bool CommitText();
        // Moves by delta steps with clamping; returns whether the value changed. Notifies EN_CHANGE on change.
        bool StepBy(int delta);
        // Pure helper used by the verification: parses a whole number without overflow.
        static bool ParseInteger(const CString& text, long long& value);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        bool KeyPressed(UINT, bool) override;
        UINT DialogCode() const override { return 0; }
        bool IsFocused() const override;
        void DpiChanged() override;
        void AppearanceChanged() override;
        void PerformAction(InputActionId, int) override;
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        friend class StepperEdit;
        void Layout();
        void ShowValue();
        void Validate();
        void Revert();
        bool EditKey(UINT key);
        void EditChanged();
        void EditFocus(bool focused);
        StepperPart PartAt(CPoint) const;
        std::unique_ptr<StepperEdit> m_edit;
        CFont m_font;
        CBrush m_brush;
        int m_value{}, m_minimum{ 0 }, m_maximum{ 100 }, m_step{ 1 };
        bool m_updating{};
        StepperPart m_hoverPart{ StepperPart::None }, m_pressedPart{ StepperPart::None };
        CString m_label, m_supporting, m_validation;
    };

    // Case-insensitive plain substring filter, or an ECMAScript regular expression. Pattern and subject lengths
    // are bounded; an invalid pattern yields no rows and a message.
    std::vector<int> FilterDropdownItems(const std::vector<CString>& items, const CString& query, bool regex, CString* error);

    class DropdownPopup;
    class Dropdown;
    // Called instead of the built-in popup when set. Returns the chosen item index, or -1 when dismissed.
    using DropdownPresenter = std::function<int(Dropdown&, const std::vector<CString>& items, int selected, const CRect& anchorScreen)>;

    // Material exposed dropdown: outlined field, trailing arrow and a self-drawn popup list with its own search
    // field. The built-in popup filters as you type and has a Regex toggle; UseSearchableMenuPopup (see
    // DropdownSearchableMenu.h) swaps in the application search menu so the full anchored regex builder is used.
    class Dropdown final : public InputWindow
    {
    public:
        Dropdown();
        ~Dropdown() override;
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void ResetContent();
        int AddItem(const CString& item);
        void SetItems(const std::vector<CString>& items);
        int GetItemCount() const { return static_cast<int>(m_items.size()); }
        CString GetItemText(int index) const;
        void SetCurSel(int index);            // does not notify
        int GetCurSel() const { return m_selected; }
        CString GetCurText() const;
        void SetLabel(const CString& label);
        void SetPrompt(const CString& prompt);
        void SetSupportingText(const CString& text);
        void SetPresenter(DropdownPresenter presenter) { m_presenter = std::move(presenter); }
        bool Open();
        void Close(bool commit = false);
        bool IsOpen() const;
        // Inspection of the built-in popup (used by the verification).
        HWND PopupWindow() const;
        HWND PopupEditWindow() const;
        std::vector<int> PopupVisibleItems() const;
        int PopupHotItem() const;
        bool PopupRegexMode() const;
        CString PopupError() const;
        CString PopupQuery() const;
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        bool KeyPressed(UINT, bool) override;
        bool CharTyped(UINT) override;
        bool WantsKey(UINT) const override;
        UINT DialogCode() const override { return DLGC_WANTARROWS | DLGC_WANTCHARS; }
        void PerformAction(InputActionId, int) override;
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        friend class DropdownPopup;
        void Choose(int index);
        bool Typeahead(wchar_t ch);
        std::vector<CString> m_items;
        int m_selected{ -1 };
        bool m_open{}, m_closedByDeactivation{};
        DWORD m_closedAt{};
        CString m_label, m_prompt, m_supporting;
        DropdownPresenter m_presenter;
        std::unique_ptr<DropdownPopup> m_popup;
        CString m_typed;
        DWORD m_typedAt{};
        NativeRegex::Options m_retained;
    };

    class ClearButton final : public InputWindow
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        // Names the field this button clears (for example "Clear collection name"); the default is just "Clear".
        void SetAccessibleName(const CString& name);
        void SetFieldFocused(bool focused);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        bool KeyPressed(UINT, bool) override;
        bool KeyReleased(UINT) override;
        UINT DialogCode() const override { return DLGC_BUTTON; }
        void PerformAction(InputActionId, int) override;
    private:
        bool m_fieldFocused{};
    };

    // Label above, the existing NativeMaterial::TextField, supporting or error text below and a trailing
    // clear button. Clearing goes through the normal EN_CHANGE path and returns focus to the field.
    class LabeledTextField final : public InputWindow
    {
    public:
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void SetLabel(const CString& label);
        void SetText(const CString& text);          // programmatic; does not notify
        CString GetText() const;
        void SetSupportingText(const CString& text);
        void SetErrorText(const CString& text);     // empty clears the error
        CString GetErrorText() const { return m_error; }
        void Clear();                               // same path as the clear button
        TextField& Field() { return m_field; }
        ClearButton& ClearControl() { return m_clear; }
        bool ClearVisible() const { return m_clearShown; }
        // A label may mark one character with an ampersand ("Collection &name", "&&" is a literal ampersand).
        // The marker is not drawn or announced; the character is underlined while keyboard cues are shown.
        // Mnemonic returns that character (0 when none). ActivateMnemonic focuses the field when the character
        // matches (case-insensitive) and the control is visible and enabled; the host dialog calls it for WM_SYSCHAR.
        wchar_t Mnemonic() const;
        bool ActivateMnemonic(wchar_t ch);
        static CString DisplayLabel(const CString& label);
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        UINT DialogCode() const override { return 0; }
        bool IsFocused() const override;
        void DpiChanged() override;
        void AppearanceChanged() override;
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        void Layout();
        void UpdateClear();
        bool m_clearShown{};
        int MnemonicIndex() const;
        void UpdateClearName();
        TextField m_field;
        ClearButton m_clear;
        CFont m_font;
        CString m_label, m_supporting, m_error;
        bool m_updating{};
    };
}
