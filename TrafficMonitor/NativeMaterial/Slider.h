#pragma once
#include "InputControls.h"

// Self-drawn Material 3 slider. It is a real child HWND painted with Direct2D (not a trackbar), keyboard operable,
// mouse draggable and exposed through UI Automation RangeValue.
//
// Notifications reach the parent two ways after a user change (programmatic setters never notify):
//   WM_COMMAND with EN_CHANGE and the control id, for handlers that treat the slider like the other inputs.
//   WM_HSCROLL with the usual trackbar codes (SB_LINELEFT/RIGHT, SB_PAGELEFT/RIGHT, SB_LEFT/RIGHT, SB_THUMBTRACK,
//   SB_THUMBPOSITION, SB_ENDSCROLL) and the slider HWND in lParam, for handlers written against a trackbar.
// Keys: Left/Down subtract a step, Right/Up add a step, PageDown/PageUp move by the page step, Home/End jump to the ends.
namespace NativeMaterial
{
    // Geometry in 96-dpi units. The handle travels between left and right on the line centerY.
    struct SliderLayout { float left, right, centerY; D2D1_RECT_F value; };
    SliderLayout LayoutSlider(float width, float height, bool showValue);
    float SliderHandleX(const SliderLayout& layout, int value, int minimum, int maximum);
    // Nearest value on the step grid for a horizontal position (clamped into the range).
    int SliderValueAt(const SliderLayout& layout, float x, int minimum, int maximum, int step);
    // Snaps a value to minimum + k * step, clamped into the range.
    int SliderSnap(int value, int minimum, int maximum, int step);
    void DrawSlider(ID2D1RenderTarget*, const Palette&, int value, int minimum, int maximum, int step, bool discrete,
        bool showValue, const CString& valueText, const InputState&);

    class Slider final : public InputWindow
    {
    public:
        // The label is the accessible name; draw the visible caption with a separate label.
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void SetRange(int minimum, int maximum);
        void SetStep(int step);
        void SetPageStep(int page);
        // Discrete sliders draw a stop at every step.
        void SetDiscrete(bool discrete);
        void SetShowValue(bool show);
        void SetLabel(const CString& label);
        // Snaps into the range and onto the step grid; does not notify.
        void SetValue(int value);
        int GetValue() const { return m_value; }
        int GetMinimum() const { return m_minimum; }
        int GetMaximum() const { return m_maximum; }
        int GetStep() const { return m_step; }
        int GetPageStep() const { return m_page; }
        bool IsDiscrete() const { return m_discrete; }
        CString GetValueText() const;
        // Moves to a value as a user change would (clamped, notifies when it changed). Returns whether it changed.
        bool MoveTo(int value, UINT scrollCode);
        int ValueAtClientX(int x) const;
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerCanceled() override;
        bool KeyPressed(UINT, bool) override;
        UINT DialogCode() const override { return DLGC_WANTARROWS; }
        void PerformAction(InputActionId, int) override;
    private:
        void NotifyScroll(UINT code);
        SliderLayout CurrentLayout() const;
        int m_value{}, m_minimum{ 0 }, m_maximum{ 100 }, m_step{ 1 }, m_page{};
        bool m_discrete{}, m_showValue{}, m_dragging{};
    };
}