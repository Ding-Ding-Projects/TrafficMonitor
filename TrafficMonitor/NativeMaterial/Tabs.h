#pragma once
#include <commctrl.h>
#include "InputControls.h"

// Self-drawn Material 3 tab bar. It is a real child HWND painted with Direct2D; it never creates or
// subclasses a SysTabControl32. It replaces CTabCtrl for the dialogs that host several pages.
//
// Styles
//   Primary    48 dip tall (64 dip when a tab has an icon: icon above the label), a 3 dip rounded indicator
//              under the label of the active tab, active label in the primary color.
//   Secondary  48 dip tall, icon beside the label, a 2 dip full-width indicator, active label in the on-surface color.
// Both draw a 1 dip divider along the bottom edge.
//
// Layout
//   Tabs that fit share the width equally (fixed tabs). When they do not, every tab keeps its own width (90 to 360 dip)
//   and the bar scrolls: mouse wheel, keyboard selection and Select() bring the active tab into view.
//   The bar is laid out left to right; right-to-left mirroring is not implemented.
//
// Keyboard (the bar is one tab stop, selection follows focus as the Material guidance asks)
//   Left / Right select the previous / next tab, Home / End the first / last. Ctrl+Tab and Ctrl+Shift+Tab cycle with
//   wrap-around while the bar has focus; HandleTabNavigation lets a dialog offer the same cycle while focus is on a page.
//
// Notifications (documented choice: WM_NOTIFY, so a dialog that used CTabCtrl keeps its ON_NOTIFY lines)
//   TCN_SELCHANGING  sent before the selection moves; the parent returns TRUE to veto the change.
//   TCN_SELCHANGE    sent after the selection moved, with the control id in idFrom.
//   Both are sent only after a user change (click, keys, UI Automation, Select()). SetSelected() never notifies.
//
// Pages
//   BindPage(index, window) associates a child window with a tab. The selected tab's page is shown (SW_SHOW) and every
//   other bound page is hidden whenever the selection changes, including SetSelected(). Placing the pages is the
//   parent's job; the bar only owns visibility.
//
// UI Automation: the bar is a Tab control with Selection; each tab is a TabItem with SelectionItem.
namespace NativeMaterial
{
    enum class TabStyle { Primary, Secondary };

    namespace TabMetrics
    {
        constexpr float Height = 48.f, HeightWithIcon = 64.f;
        constexpr float MinimumWidth = 90.f, MaximumWidth = 360.f;
        constexpr float Padding = 16.f;                // horizontal padding on each side of the label
        constexpr float PrimaryIndicator = 3.f, SecondaryIndicator = 2.f;
        constexpr float ScrollStep = 96.f;             // wheel notch, in dip
    }

    struct TabsModel
    {
        std::vector<CString> labels;
        std::vector<wchar_t> icons;                    // 0 for none; a Segoe MDL2 Assets glyph otherwise
        int selected{ -1 }, hot{ -1 }, pressed{ -1 };
        float scroll{};
        TabStyle style{ TabStyle::Primary };
    };

    struct TabsLayout
    {
        std::vector<D2D1_RECT_F> tabs;                 // rectangles in bar coordinates, already scrolled
        D2D1_RECT_F indicator{};                       // active indicator (empty when nothing is selected)
        bool scrollable{};
        float contentWidth{}, maxScroll{};
    };

    // Intrinsic width of each tab (label plus padding, clamped to 90..360 dip); icon-only width is at least 56.
    // Secondary tabs put the icon beside the label (24 dip plus an 8 dip gap), primary tabs above it.
    std::vector<float> MeasureTabWidths(const std::vector<CString>& labels, const std::vector<wchar_t>& icons, TabStyle style = TabStyle::Primary);
    // Label widths alone (no padding), used to size the primary indicator.
    std::vector<float> MeasureLabelWidths(const std::vector<CString>& labels);
    float TabsBarHeight(bool anyIcon, TabStyle style);
    // Pure geometry. labelWidths are the label widths without padding (used to size the primary indicator).
    TabsLayout LayoutTabs(float width, float height, const std::vector<float>& intrinsic, const std::vector<float>& labelWidths,
        float scroll, TabStyle style, int selected);
    // The scroll offset that brings tab `index` fully into view with the least movement.
    float TabScrollToReveal(const TabsLayout& unscrolled, int index, float viewWidth, float current);
    void DrawTabs(ID2D1RenderTarget*, const Palette&, const TabsModel&, const InputState&);

    class Tabs final : public InputWindow
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id, TabStyle style = TabStyle::Primary);
        // Tabs are added at the end. The icon is an optional Segoe MDL2 Assets code point.
        int AddTab(const CString& text, wchar_t icon = 0);
        void RemoveAllTabs();
        int GetTabCount() const { return static_cast<int>(m_labels.size()); }
        CString GetTabText(int index) const;
        void SetTabText(int index, const CString& text);
        void SetStyle(TabStyle style);
        TabStyle GetStyle() const { return m_style; }
        void SetAccessibleName(const CString& name);

        // Silent selection (no notification); applies page visibility and scrolls the tab into view.
        void SetSelected(int index);
        int GetSelected() const { return m_selected; }
        // A user-level change: asks TCN_SELCHANGING, moves, notifies TCN_SELCHANGE. Returns whether it moved.
        bool Select(int index);

        void BindPage(int index, CWnd* page);
        CWnd* GetBoundPage(int index) const;

        // Ctrl+Tab / Ctrl+Shift+Tab. Returns true when the key was a tab cycle and was handled.
        bool HandleTabNavigation(UINT message, WPARAM key, bool ctrl, bool shift);
        // Convenience for a dialog PreTranslateMessage: reads the live modifier state. Consumes the message only when
        // its window is this bar or one of the pages' descendants in the bar's parent.
        bool HandleTabNavigation(const MSG& message);

        // Inspection used by the verification.
        bool IsScrollable() const;
        float ScrollOffset() const { return m_scroll; }
        float MaximumScroll() const;
        CRect TabRect(int index) const;               // client pixels, as drawn
        CRect IndicatorRect() const;                  // client pixels
        int TabAtPoint(CPoint point) const;
        int PreferredHeight() const;                  // physical pixels
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        void PointerCanceled() override;
        bool KeyPressed(UINT, bool) override;
        bool WantsKey(UINT) const override;
        bool MouseWheel(int delta) override;
        UINT DialogCode() const override { return DLGC_WANTARROWS; }
        void PerformAction(InputActionId, int) override;
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        TabsLayout CurrentLayout(float scroll) const;
        void Remeasure();
        void ApplyPages();
        void RevealSelected();
        void ClampScroll();
        bool CycleSelection(bool backward);
        std::vector<CString> m_labels;
        std::vector<wchar_t> m_icons;
        std::vector<float> m_intrinsic, m_labelWidths;
        std::vector<CWnd*> m_pages;
        int m_selected{ -1 }, m_hot{ -1 }, m_pressedTab{ -1 };
        float m_scroll{};
        TabStyle m_style{ TabStyle::Primary };
        CString m_name;
    };
}
