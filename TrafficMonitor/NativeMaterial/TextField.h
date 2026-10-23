#pragma once
#include "Theme.h"
#include <afxwin.h>
namespace NativeMaterial
{
    // The EDIT HWND retains native text, IME, caret, undo and accessibility.
    class TextField : public CEdit
    {
        DECLARE_DYNAMIC(TextField)
    public:
        void SetAppearance(Appearance);
        HBRUSH Color(CDC*);
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        void Frame();
        Appearance m_appearance{Appearance::Light};CBrush m_background;
    };
}
