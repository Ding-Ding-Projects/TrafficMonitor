#pragma once
#include "Theme.h"
#include <afxwin.h>
namespace NativeMaterial
{
    class Panel final : public CWnd
    {
    public:
        BOOL Create(CWnd*,UINT);
        void SetAppearance(Appearance);
        COLORREF SurfaceColor() const{return m_surfaceColor;}
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        COLORREF m_surfaceColor{RGB(255,255,255)};Appearance m_appearance{Appearance::Light};CBrush m_fill;HFONT m_font{};
    };
}
