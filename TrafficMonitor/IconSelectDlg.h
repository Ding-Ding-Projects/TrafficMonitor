#pragma once
#include "afxwin.h"
#include <functional>
#include "BaseDialog.h"

// Self-painted preview surface: a stretched bitmap with an optional overlay (the notification icon).
// It replaces the stock bitmap static of the original template.
class CIconPreview : public CWnd
{
public:
    BOOL Create(const CRect& rect, CWnd* parent, UINT id);
    void SetPicture(HBITMAP bitmap);                    // takes ownership of the bitmap
    void SetSurfaceColor(COLORREF color);
    std::function<void(CDC&)> draw_overlay;
protected:
    CBitmap m_bitmap;
    BITMAP m_info{};
    COLORREF m_surface{ RGB(255, 255, 255) };
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
};

// CIconSelectDlg dialog (Material shell)

class CIconSelectDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CIconSelectDlg)

public:
    CIconSelectDlg(int icon_selected, CWnd* pParent = NULL);   // standard constructor
    virtual ~CIconSelectDlg();

    int GetIconSelected() const;

    void SetAutoAdaptNotifyIcon(bool val);
    bool AutoAdaptNotifyIcon() const;

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ICON_SELECT_DIALOG };
#endif

#define PREVIEW_WIDTH theApp.DPI(200)		// preview width and height
#define PREVIEW_HEIGHT theApp.DPI(40)

#define ICON_X theApp.DPI(46)		// position of the icon inside the preview
#define ICON_Y theApp.DPI(12)

protected:
    CIconPreview m_preview_pic;
    NativeMaterial::Dropdown* m_icon_select_combo{};
    NativeMaterial::Checkbox* m_auto_adapt_chk{};
    NativeMaterial::Label* m_preview_heading{};
    NativeMaterial::Button* m_ok{};
    NativeMaterial::Button* m_cancel{};

    int m_icon_selected{};

    bool m_atuo_adapt_notify_icon;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual CString GetDialogName() const override;
    virtual int LayoutMaterialControls(const CRect& content, bool apply) override;
    virtual void OnMaterialAppearanceChanged() override;

    void DrawPreviewIcon(CDC* pDC);
    void LoadPreviewPicture();

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnCbnSelchangeCombo1();
    afx_msg void OnBnClickedAutoAdaptCheck();
};
