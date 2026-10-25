#pragma once
#include <afxwin.h>
#include "Theme.h"
#include "TextField.h"
namespace NativeMaterial
{
    COLORREF SettingsDisabledColor(Appearance appearance,bool background=false);
    // Notification-only styling; the real EDIT retains text and disabled state.
    class SettingsNumber final : public TextField
    {
    public:
        void SetGrouped(bool grouped){m_grouped=grouped;}
        void SetAppearance(Appearance appearance){m_appearance=appearance;TextField::SetAppearance(appearance);}
        HBRUSH Color(CDC* dc);
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        void PaintDisabled(HDC);
        bool m_grouped{};Appearance m_appearance{Appearance::Light};CBrush m_disabledBackground;
    };
    // Retains native BUTTON auto-checkbox state, keyboard behavior and accessibility.
    class SettingsCheck final : public CButton
    {
    public:
        BOOL Create(const CString& text, CWnd* parent, UINT id);
        void SetAppearance(Appearance value) { m_appearance = value; Invalidate(); }
        void SetGrouped(bool value){m_grouped=value;Invalidate();}
    protected:
        afx_msg void OnPaint();
        afx_msg void OnLButtonUp(UINT flags, CPoint point);
        afx_msg void OnKeyUp(UINT key, UINT repeat, UINT flags);
        afx_msg void OnSetFocus(CWnd* old);
        afx_msg void OnKillFocus(CWnd* next);
        DECLARE_MESSAGE_MAP()
    private:
        Appearance m_appearance{ Appearance::Light };
        bool m_grouped{};
    };
}
