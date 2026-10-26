#pragma once
#include "InputControls.h"
#include "FeedbackVisuals.h"
#include <functional>

// Self-drawn Material 3 progress indicator: a linear bar (4 high track) or a circular ring, determinate or
// indeterminate. It is a real child HWND painted with Direct2D and exposed through UI Automation as a progress bar.
//
//   Progress bar;  bar.Create(L"Copying files", ProgressShape::Linear, rect, parent, id);
//   bar.SetRange(0, 200);  bar.SetValue(50);              // clamped into the range; shows 25 %
//   bar.SetIndeterminate(true);  bar.SetStatusText(L"Scanning");
//
// * Determinate indicators expose RangeValue (read only: the value comes from the operation, not the user).
//   An indeterminate indicator offers no RangeValue, and its status text is the accessible help text.
// * Indeterminate motion runs on one shared 30 fps clock (a hidden message window that exists only while at
//   least one indeterminate indicator does); it invalidates only visible indicators.
// * Reduced motion (Windows "Show animations" off, or the test probe) replaces the motion with a static
//   indeterminate presentation (a fixed segment or ring arc) plus the status text, so the state is never conveyed
//   by motion alone. Give an indeterminate indicator a status text and, for the linear shape, a height of at least
//   32 DIP so the text has room; with no status text it shows a localized "Working..." in that mode.
// Programmatic setters never notify; the indicator has no user input.
namespace NativeMaterial
{
    class Progress final : public InputWindow
    {
    public:
        Progress();
        ~Progress() override;
        BOOL Create(const CString& name, ProgressShape shape, const RECT& bounds, CWnd* parent, UINT id);
        void SetName(const CString& name);
        void SetRange(int minimum, int maximum);     // maximum is kept above minimum; the value is clamped again
        void SetValue(int value);                    // clamped into the range
        int GetValue() const { return m_value; }
        int GetMinimum() const { return m_minimum; }
        int GetMaximum() const { return m_maximum; }
        float Fraction() const;                      // 0..1
        void SetIndeterminate(bool indeterminate);
        bool IsIndeterminate() const { return m_indeterminate; }
        void SetStatusText(const CString& text);
        CString StatusText() const { return m_status; }
        // The text actually drawn: the status text, or the localized default in the reduced-motion presentation.
        CString EffectiveStatusText() const;
        ProgressShape Shape() const { return m_shape; }
        // Motion state.
        bool ReducedMotion() const;
        bool IsAnimating() const { return m_indeterminate && !ReducedMotion(); }
        float Phase() const { return m_phase; }
        void Advance(ULONGLONG nowMilliseconds);     // moves the phase; the shared clock calls this
        // Verification hooks.
        static void SetReducedMotionProbe(std::function<bool()> probe);    // an empty function restores the system setting
        static int RegisteredIndicators();           // indicators on the shared clock
        static bool ClockRunning();
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        UINT DialogCode() const override { return 0; }
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        void JoinClock();
        void LeaveClock();
        ProgressShape m_shape{ ProgressShape::Linear };
        int m_value{}, m_minimum{ 0 }, m_maximum{ 100 };
        bool m_indeterminate{}, m_joined{};
        float m_phase{};
        CString m_status;
    };
}
