#pragma once

#include "Theme.h"
#include <afxwin.h>
#include <memory>

namespace NativeMaterial
{
    // Turns an existing top-level window into a frameless window with a Material 3 title bar.
    //
    // The operating-system caption is removed through WM_NCCALCSIZE while the sizing border,
    // DWM shadow, system menu and snap behavior stay with the window. The title bar is a child
    // window that fills the top HeightPx() pixels of the client area; owners must place their
    // own content below it. Caption drag, resize edges and the minimize, maximize and close
    // buttons are reported through WM_NCHITTEST so Windows keeps its native semantics, including
    // Snap Layouts on the maximize button.
    class TitleBar final
    {
    public:
        static constexpr int HeightDp = 40;
        static constexpr int ButtonWidthDp = 48;

        enum class Part { None, Caption, Minimize, Maximize, Close };

        struct Labels
        {
            CString minimize{ L"Minimize" }, maximize{ L"Maximize" }, restore{ L"Restore" }, close{ L"Close" };
        };

        // Client-coordinate rectangles of the title bar parts at the window's current DPI.
        struct Geometry
        {
            CRect bar, icon, text, minimize, maximize, close;
            bool hasIcon{}, hasMinimize{}, hasMaximize{}, hasClose{};
        };

        // Pure layout used by painting, hit testing, accessibility and the focused test.
        static Geometry Compute(int clientWidthPx, UINT dpi, bool icon, bool minimize, bool maximize, bool close);

        TitleBar();
        ~TitleBar();
        TitleBar(const TitleBar&) = delete;
        TitleBar& operator=(const TitleBar&) = delete;

        // Subclasses the window, removes its caption and creates the title bar child.
        // Safe to call once the window handle exists. The window keeps its WS_CAPTION,
        // WS_THICKFRAME and WS_SYSMENU style bits so Windows retains snap, resize and
        // Alt+Space behavior; only the drawn non-client caption is removed.
        HRESULT Attach(CWnd* window, Appearance appearance, const Labels& labels = Labels{});
        void Detach();
        bool IsAttached() const noexcept;

        void SetAppearance(Appearance appearance);
        void SetLabels(const Labels& labels);

        // Height of the title bar in pixels at the window's current DPI.
        int HeightPx() const;
        // Outer window size that leaves a content area (below the title bar) of the given size.
        SIZE OuterSizeForContent(int contentWidthPx, int contentHeightPx) const;
        Geometry Measure() const;
        // Hit test in screen coordinates using the same logic as WM_NCHITTEST.
        Part PartAt(POINT screen) const;
        HWND BarWindow() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
