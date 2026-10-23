#pragma once
#include <afxwin.h>
#include <memory>
#include "Theme.h"

namespace NativeMaterial
{
    struct ButtonAccess;
    class Button final : public CWnd
    {
    public:
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void SetAppearance(Appearance appearance);
        void SetLabel(const CString& label);
    protected:
        afx_msg void OnPaint();
        afx_msg BOOL OnEraseBkgnd(CDC*);
        afx_msg void OnLButtonDown(UINT, CPoint);
        afx_msg void OnLButtonUp(UINT, CPoint);
        afx_msg void OnKeyDown(UINT, UINT, UINT);
        afx_msg void OnKeyUp(UINT, UINT, UINT);
        afx_msg void OnSetFocus(CWnd*);
        afx_msg void OnKillFocus(CWnd*);
        afx_msg void OnEnable(BOOL);
        afx_msg void OnDestroy();
        afx_msg LRESULT OnGetObject(WPARAM, LPARAM);
        afx_msg LRESULT OnClick(WPARAM, LPARAM);
        afx_msg UINT OnGetDlgCode();
        DECLARE_MESSAGE_MAP()
    private:
        Canvas m_canvas;
        Appearance m_appearance{ Appearance::Light };
        bool m_pressed{};
        std::shared_ptr<ButtonAccess> m_access;
    };
}
