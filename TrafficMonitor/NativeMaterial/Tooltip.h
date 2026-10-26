#pragma once
#include <afxwin.h>
#include <map>
#include <memory>
#include "Theme.h"

// Self-drawn Material 3 tooltips. No tooltips_class32 window is ever created and CToolTipCtrl is not used.
//
//   Plain tooltip  one line of supporting text (wraps at 240 dip) on an inverse-surface container, 12 dip text.
//   Rich tooltip   a title and supporting text on a surface-container card with a 12 dip corner (AddTool with a title).
//
// One Tooltip object serves any number of tool windows, like one CToolTipCtrl: AddTool(window, text),
// UpdateTipText(text, window), RemoveTool(window). Unlike CToolTipCtrl there is no RelayEvent: AddTool subclasses the
// tool window (SetWindowSubclass) and tracks it by itself.
//
// Behavior
//   Mouse        the tip appears after the pointer rests on the tool for the initial delay (500 ms by default) and is
//                placed below the pointer. It hides when the pointer leaves, on any mouse button, wheel or key press,
//                when the tool is disabled or hidden, and when the tool is destroyed (which also removes it).
//   Keyboard     when a tool receives keyboard focus through a navigation key (Tab, arrows, Home, End) the tip appears
//                after the same delay below the tool; losing focus or pressing a key hides it. A click focus never shows one.
//   Placement    always inside the work area of the monitor that holds the tool (flips above when there is no room below,
//                shifts horizontally to fit). See PlaceTooltip.
//   Focus        the popup is WS_EX_NOACTIVATE | TOPMOST and mouse-transparent, so it never takes focus or input.
//   Motion       the tip fades in over about 100 ms. When the operating system animation setting is off (the reduced
//                motion preference) it appears at once at full opacity. Hiding is always immediate.
//   Accessibility the popup exposes the ToolTip role and the tip text as its name; the tool receives the tip text as its
//                help text (UI Automation HelpText) while it is registered; EVENT_OBJECT_SHOW / HIDE are raised for clients.
namespace NativeMaterial
{
    namespace TooltipMetrics
    {
        constexpr float PlainMaxTextWidth = 240.f, PlainPaddingX = 8.f, PlainPaddingY = 4.f, PlainMinHeight = 24.f, PlainRadius = 4.f;
        constexpr float RichMaxTextWidth = 280.f, RichPaddingX = 16.f, RichPaddingY = 12.f, RichMinWidth = 112.f, RichRadius = 12.f;
        constexpr float PointerHeight = 20.f;          // pointer height the tip keeps clear of
        constexpr float Gap = 2.f;                     // gap between the tip and the pointer or tool
        constexpr UINT DefaultDelay = 500;
    }

    struct TooltipContent
    {
        CString title, text;                           // a non-empty title makes the tip rich
        bool Rich() const { return !title.IsEmpty(); }
    };

    // Size in device-independent pixels.
    D2D1_SIZE_F MeasureTooltip(const TooltipContent& content);
    // The container color; the whole target is cleared to it, so the visible text is everything that differs from it.
    D2D1_COLOR_F TooltipFill(const Palette& palette, bool rich);
    void DrawTooltip(ID2D1RenderTarget*, const Palette&, const TooltipContent&);
    // Pure placement in physical pixels. `avoid` is the rectangle to keep clear (pointer box or tool), anchorLeft the preferred
    // left edge. The result is below `avoid` with `gap` between, above when it does not fit below, and always inside workArea
    // (a tip larger than the work area is pinned to its top left corner).
    CRect PlaceTooltip(CSize size, CRect avoid, int anchorLeft, CRect workArea, int gap);

    class Tooltip;

    // The popup window: layered, no-activate, mouse-transparent.
    class TooltipWindow : public CWnd
    {
    public:
        TooltipWindow();
        BOOL CreatePopup(Tooltip* owner);
        // Paints into a device context (the WM_PAINT and WM_PRINTCLIENT route). Returns the Direct2D result.
        HRESULT PaintTo(HDC dc);
        const TooltipContent& Content() const { return m_content; }
        UINT ContentDpi() const { return m_dpi; }
        BYTE Alpha() const;
        void SetAppearance(Appearance appearance) { m_appearance = appearance; }
    protected:
        afx_msg void OnPaint();
        afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
        afx_msg LRESULT OnNcHitTest(CPoint) { return HTTRANSPARENT; }
        afx_msg int OnMouseActivate(CWnd*, UINT, UINT) { return MA_NOACTIVATE; }
        afx_msg LRESULT OnPrintClient(WPARAM, LPARAM);
        afx_msg void OnTimer(UINT_PTR id);
        DECLARE_MESSAGE_MAP()
    private:
        friend class Tooltip;
        void Present(const TooltipContent& content, UINT dpi, const CRect& bounds, bool animate);
        void Dismiss();
        void SetAlpha(BYTE alpha);
        Tooltip* m_owner{};
        TooltipContent m_content;
        UINT m_dpi{ 96 };
        BYTE m_alpha{ 255 };
        Appearance m_appearance{ Appearance::Light };
        bool m_shown{};
    };

    class Tooltip
    {
    public:
        Tooltip();
        ~Tooltip();
        Tooltip(const Tooltip&) = delete;
        Tooltip& operator=(const Tooltip&) = delete;
        // Creates the popup. Called implicitly by the first AddTool; calling it early surfaces a creation failure.
        BOOL Create();
        void AddTool(CWnd* window, const CString& text);
        void AddTool(CWnd* window, const CString& title, const CString& text);   // rich
        void UpdateTipText(const CString& text, CWnd* window);
        void RemoveTool(CWnd* window);
        void RemoveAllTools();
        bool HasTool(const CWnd* window) const;
        CString GetTipText(const CWnd* window) const;
        void SetAppearance(Appearance appearance);
        void SetInitialDelay(UINT milliseconds) { m_delay = milliseconds; }   // 0 shows at once
        UINT GetInitialDelay() const { return m_delay; }
        void Activate(bool active);
        bool IsActive() const { return m_enabled; }
        void Hide();

        // Shows the tip for a tool now, ignoring the delay (also the test hook that skips the delay).
        bool ShowFor(CWnd* window, bool keyboard);
        // What the tool subclass calls; public so a host can drive the same paths without synthesizing input.
        void NotifyPointer(CWnd* window, CPoint screenPoint);
        void NotifyPointerLeft(CWnd* window);
        void NotifyFocus(CWnd* window, bool keyboard);
        void NotifyFocusLost(CWnd* window);
        void NotifyDismiss(CWnd* window);

        // Inspection.
        bool IsVisible() const;
        CWnd* ActiveTool() const;
        CRect Bounds() const;                         // screen rectangle of the visible tip
        BYTE Alpha() const;
        CString ShownText() const;
        HWND PopupWindow() const;
        TooltipWindow& Popup() { return m_popup; }
        // -1 follows the operating system animation setting (default), 0 forces no motion, 1 forces the fade.
        static void SetAnimationOverride(int mode);
    private:
        friend class TooltipWindow;
        struct Tool { TooltipContent content; };
        static LRESULT CALLBACK SubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
        void OnTimer(UINT_PTR id);
        void Arm(CWnd* window, bool keyboard);
        void Disarm();
        void Annotate(HWND window, const CString& help);
        void Unannotate(HWND window);
        void Drop(HWND window, bool unsubclass);
        std::map<HWND, Tool> m_tools;
        TooltipWindow m_popup;
        HWND m_shownTool{}, m_candidate{}, m_hoverTool{};
        bool m_candidateKeyboard{}, m_shownByKeyboard{}, m_enabled{ true };
        UINT m_delay{ TooltipMetrics::DefaultDelay };
        CPoint m_pointer;
        Appearance m_appearance{ Appearance::Light };
    };
}
