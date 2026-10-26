#pragma once
#include <afxwin.h>
#include <memory>
#include "Theme.h"

namespace NativeMaterial
{
    enum class LabelRole { Primary, Secondary, Error };

    // Self-drawn, wrapping Material text. A drop-in for a static text window: it keeps its caption in the window
    // text, accepts WM_SETFONT / WM_GETFONT and paints with the palette, so it follows the theme in light, dark and
    // high contrast without any system color message. It never takes focus.
    class Label final : public CWnd
    {
    public:
        BOOL Create(LPCWSTR text, DWORD style, const RECT& bounds, CWnd* parent, UINT id = 0xFFFF);
        void SetAppearance(Appearance appearance);
        void SetRole(LabelRole role);
        void SetSurfaceColor(COLORREF color);
        // Height in physical pixels that the text needs when wrapped to a width in physical pixels.
        int MeasureHeight(int widthPx) const;
        HRESULT RenderStatus() const { return m_drawResult; }
    protected:
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
        afx_msg void OnPaint();
        afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
        afx_msg void OnDestroy();
        afx_msg LRESULT OnPrintClient(WPARAM, LPARAM);
        afx_msg LRESULT OnSetFont(WPARAM, LPARAM);
        afx_msg LRESULT OnGetFont(WPARAM, LPARAM);
        DECLARE_MESSAGE_MAP()
    private:
        HRESULT PaintTo(HDC dc, bool printing);
        Microsoft::WRL::ComPtr<IDWriteTextFormat> Format(IDWriteFactory* factory, float dpi) const;
        Appearance m_appearance{ Appearance::Light };
        LabelRole m_role{ LabelRole::Primary };
        bool m_customSurface{};
        COLORREF m_surfaceColor{};
        HFONT m_font{};
        HRESULT m_drawResult{ E_PENDING };
    };
}