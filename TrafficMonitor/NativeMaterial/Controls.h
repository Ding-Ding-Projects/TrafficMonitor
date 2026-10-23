#pragma once
#include <afxwin.h>
#include <afxcmn.h>
#include <memory>
#include "Theme.h"
#include "ButtonVisual.h"

namespace NativeMaterial
{
    struct ButtonAccess;
    struct ValueAccess;
    class Button final : public CWnd
    {
    public:
        BOOL Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id);
        void SetAppearance(Appearance appearance);
        void SetLabel(const CString& label);
        void SetSurfaceColor(COLORREF color){m_surfaceColor=color;m_customSurface=true;if(m_hWnd)Invalidate();}
        void SetVariant(ButtonVariant value){m_variant=value;if(m_hWnd)Invalidate();}
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
        HRESULT DrawDC(HDC dc);
        HRESULT m_attachResult{E_PENDING}, m_drawResult{E_PENDING}, m_printResult{E_PENDING};
        Appearance m_appearance{ Appearance::Light };
        ButtonVariant m_variant{ButtonVariant::Primary};
        bool m_pressed{}, m_hover{}, m_customSurface{};
        COLORREF m_surfaceColor{};
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
    protected:
        LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam) override;
    private:
        Appearance m_appearance{Appearance::Light};
        std::shared_ptr<ValueAccess> m_access;
    };

    class RangeValue final : public CSliderCtrl
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        void SetAppearance(Appearance value) { m_appearance=value; Invalidate(); }
    protected:
        afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
        LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam) override;
        DECLARE_MESSAGE_MAP()
    private:
        Appearance m_appearance{Appearance::Light};
        std::shared_ptr<ValueAccess> m_access;
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
