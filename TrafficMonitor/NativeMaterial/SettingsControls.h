#pragma once
#include <afxwin.h>
#include "Theme.h"
namespace NativeMaterial
{
    // Retains native BUTTON auto-checkbox state, keyboard behavior and accessibility.
    class SettingsCheck final : public CButton
    {
    public:
        BOOL Create(const CString& text, CWnd* parent, UINT id);
        void SetAppearance(Appearance value) { m_appearance = value; Invalidate(); }
    protected:
        afx_msg void OnPaint();
        afx_msg void OnLButtonUp(UINT flags, CPoint point);
        afx_msg void OnKeyUp(UINT key, UINT repeat, UINT flags);
        afx_msg void OnSetFocus(CWnd* old);
        afx_msg void OnKillFocus(CWnd* next);
        DECLARE_MESSAGE_MAP()
    private:
        Appearance m_appearance{ Appearance::Light };
    };
}
