#pragma once
#include <afxwin.h>
#include <functional>
#include "../NativeMaterial/Controls.h"
#include "../NativeMaterial/Panel.h"

namespace NativeUpdate
{
    // Persistent, non-blocking update notice. It is a separate top-level window that never
    // takes focus when it appears, never runs a modal loop and leaves the monitor usable.
    // UI thread only. Texts arrive fully formatted; decisions live in BannerLogic.h.
    class BannerWindow final : public CWnd
    {
    public:
        enum : UINT { Primary = 61001, Secondary, ReleaseNotes, Dismiss };
        struct Content
        {
            CString headline, detail, primary, secondary, notes, url;
        };
        BOOL Create();
        // Shows or refreshes the banner without activating it. An empty primary/notes label hides that button.
        void Present(const Content& content);
        void Hide();
        bool IsShown() const { return m_hWnd && ::IsWindow(m_hWnd) && ::IsWindowVisible(m_hWnd); }
        std::function<void(UINT)> onCommand;
    protected:
        BOOL PreTranslateMessage(MSG* message) override;
        BOOL OnCommand(WPARAM, LPARAM) override;
        LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam) override;
        afx_msg void OnClose();
        DECLARE_MESSAGE_MAP()
    private:
        int Px(int value) const { return MulDiv(value, m_dpi, 96); }
        void Fonts();
        int TextHeight(const CString& text, int width, CFont& font);
        UINT m_dpi{ 96 };
        CFont m_font, m_bold;
        NativeMaterial::Panel m_panel;
        CStatic m_headline, m_detail;
        NativeMaterial::Button m_primary, m_secondary, m_notes, m_dismiss;
        COLORREF m_outline{ RGB(121, 116, 126) };
        CString m_url;
    };
}
