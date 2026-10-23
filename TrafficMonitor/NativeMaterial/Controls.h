#pragma once
#include <afxwin.h>
#include <afxcmn.h>
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
        afx_msg void OnMouseMove(UINT, CPoint);
        afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
        afx_msg void OnCaptureChanged(CWnd*);
        afx_msg void OnKeyDown(UINT, UINT, UINT);
        afx_msg void OnKeyUp(UINT, UINT, UINT);
        afx_msg void OnSetFocus(CWnd*);
        afx_msg void OnKillFocus(CWnd*);
        afx_msg void OnEnable(BOOL);
        afx_msg void OnDestroy();
        afx_msg LRESULT OnGetObject(WPARAM, LPARAM);
        afx_msg LRESULT OnClick(WPARAM, LPARAM);
        afx_msg LRESULT OnPrintClient(WPARAM, LPARAM);
        afx_msg LRESULT OnRenderStatus(WPARAM, LPARAM);
        afx_msg UINT OnGetDlgCode();
        DECLARE_MESSAGE_MAP()
    private:
        Canvas m_canvas;
        void Draw(ID2D1RenderTarget* target);
        HRESULT m_attachResult{E_PENDING}, m_drawResult{E_PENDING}, m_printResult{E_PENDING};
        Appearance m_appearance{ Appearance::Light };
        bool m_pressed{}, m_hover{};
        std::shared_ptr<ButtonAccess> m_access;
    };

    // Native selection and range providers remain attached to the same visible
    // HWND that receives keyboard input. Drawing changes do not proxy input.
    class Choice final : public CComboBox
    {
    public:
        BOOL Create(DWORD style, const RECT& bounds, CWnd* parent, UINT id);
        void SetAppearance(Appearance value) { m_appearance=value; Invalidate(); }
        void DrawItem(LPDRAWITEMSTRUCT item) override;
        void MeasureItem(LPMEASUREITEMSTRUCT item) override;
    private:
        Appearance m_appearance{Appearance::Light};
    };

    class RangeValue final : public CSliderCtrl
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        void SetAppearance(Appearance value) { m_appearance=value; Invalidate(); }
    protected:
        afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
        DECLARE_MESSAGE_MAP()
    private:
        Appearance m_appearance{Appearance::Light};
    };

    class ReadOnlyText final : public CEdit
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        void SetAppearance(Appearance value);
    protected:
        afx_msg HBRUSH CtlColor(CDC*, UINT);
        DECLARE_MESSAGE_MAP()
    private:
        Appearance m_appearance{Appearance::Light};
        CBrush m_background;
    };
}
