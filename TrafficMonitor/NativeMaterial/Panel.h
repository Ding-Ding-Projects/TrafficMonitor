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
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        Appearance m_appearance{Appearance::Light};CBrush m_fill;HFONT m_font{};
    };
}
