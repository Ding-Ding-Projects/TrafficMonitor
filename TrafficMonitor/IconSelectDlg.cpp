// IconSelectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "IconSelectDlg.h"
#include "afxdialogex.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/InputControls.h"


// CIconPreview

BEGIN_MESSAGE_MAP(CIconPreview, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CIconPreview::Create(const CRect& rect, CWnd* parent, UINT id)
{
    const CString class_name = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW));
    return CWnd::Create(class_name, L"", WS_CHILD | WS_VISIBLE, rect, parent, id);
}

void CIconPreview::SetPicture(HBITMAP bitmap)
{
    if (m_bitmap.GetSafeHandle() != NULL)
        m_bitmap.DeleteObject();
    if (bitmap != NULL && m_bitmap.Attach(bitmap))
        ::GetObject(bitmap, sizeof(BITMAP), &m_info);
    if (m_hWnd != nullptr)
        Invalidate(FALSE);
}

void CIconPreview::SetSurfaceColor(COLORREF color)
{
    m_surface = color;
    if (m_hWnd != nullptr)
        Invalidate(FALSE);
}

void CIconPreview::OnPaint()
{
    CPaintDC dc(this);
    CRect client;
    GetClientRect(client);
    dc.FillSolidRect(client, m_surface);
    if (m_bitmap.GetSafeHandle() != NULL && m_info.bmWidth > 0 && m_info.bmHeight > 0)
    {
        CDC memory;
        memory.CreateCompatibleDC(&dc);
        CBitmap* previous = memory.SelectObject(&m_bitmap);
        dc.SetStretchBltMode(HALFTONE);     // avoid distortion of the stretched picture
        dc.SetBrushOrg(0, 0);
        dc.StretchBlt(0, 0, client.Width(), client.Height(), &memory, 0, 0, m_info.bmWidth, m_info.bmHeight, SRCCOPY);
        memory.SelectObject(previous);
        if (draw_overlay)
            draw_overlay(dc);
    }
}


// CIconSelectDlg dialog

IMPLEMENT_DYNAMIC(CIconSelectDlg, CBaseDialog)

CIconSelectDlg::CIconSelectDlg(int icon_selected, CWnd* pParent /*=NULL*/)
    : CBaseDialog(IDD_ICON_SELECT_DIALOG, pParent), m_icon_selected{ icon_selected }
{
    EnableMaterialShell();
}

CIconSelectDlg::~CIconSelectDlg()
{
}

int CIconSelectDlg::GetIconSelected() const
{
    if (m_icon_selected < 0 || m_icon_selected >= MAX_NOTIFY_ICON)
        return 0;
    return m_icon_selected;
}

void CIconSelectDlg::SetAutoAdaptNotifyIcon(bool val)
{
    m_atuo_adapt_notify_icon = val;
}

bool CIconSelectDlg::AutoAdaptNotifyIcon() const
{
    return m_atuo_adapt_notify_icon;
}

void CIconSelectDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

void CIconSelectDlg::DrawPreviewIcon(CDC* pDC)
{
    ::DrawIconEx(pDC->m_hDC, ICON_X, ICON_Y, theApp.m_notify_icons[GetIconSelected()], theApp.DPI(16), theApp.DPI(16), 0, NULL, DI_NORMAL);
}

void CIconSelectDlg::LoadPreviewPicture()
{
    const UINT resource = (m_icon_selected == 4 || m_icon_selected == 5) ? IDB_NOTIFY_ICON_PREVIEW_LIGHT : IDB_NOTIFY_ICON_PREVIEW;
    m_preview_pic.SetPicture((HBITMAP)LoadImage(AfxGetInstanceHandle(),
        MAKEINTRESOURCE(resource), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION));
}

int CIconSelectDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    // Icon dropdown, preview heading and picture, the auto adapt check box, then the action row.
    const int margin = DPI(20), gap = DPI(12), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    const int left = content.left + margin;
    int y = content.top + margin;
    if (m_icon_select_combo != nullptr)
    {
        const int height = m_icon_select_combo->MinimumSize().cy;
        if (apply)
            m_icon_select_combo->MoveWindow(left, y, width, height);
        y += height + gap;
    }
    if (m_preview_heading != nullptr)
    {
        const int height = m_preview_heading->MeasureHeight(width);
        if (apply)
            m_preview_heading->MoveWindow(left, y, width, height);
        y += height + DPI(8);
    }
    if (m_preview_pic.GetSafeHwnd() != nullptr)
    {
        if (apply)
            m_preview_pic.MoveWindow(left, y, PREVIEW_WIDTH, PREVIEW_HEIGHT);
        y += PREVIEW_HEIGHT + gap;
    }
    if (m_auto_adapt_chk != nullptr)
    {
        const int height = (std::max)(static_cast<int>(m_auto_adapt_chk->MinimumSize().cy), DPI(40));
        if (apply)
            m_auto_adapt_chk->MoveWindow(left, y, width, height);
        y += height + gap;
    }
    y += gap;
    if (m_ok != nullptr && m_cancel != nullptr)
    {
        CString ok, cancel;
        m_ok->GetWindowText(ok);
        m_cancel->GetWindowText(cancel);
        const int cancel_width = MeasureMaterialButtonWidth(cancel, NativeMaterial::ButtonVariant::Text, 88);
        const int ok_width = MeasureMaterialButtonWidth(ok, NativeMaterial::ButtonVariant::Primary, 88);
        if (apply)
        {
            const int right = left + width;
            m_cancel->MoveWindow(right - cancel_width, y, cancel_width, button_height);
            m_ok->MoveWindow(right - cancel_width - gap - ok_width, y, ok_width, button_height);
        }
        y += button_height;
    }
    return y + margin - content.top;
}

void CIconSelectDlg::OnMaterialAppearanceChanged()
{
    if (m_preview_pic.GetSafeHwnd() != nullptr)
        m_preview_pic.SetSurfaceColor(MaterialSurfaceColor());
}


BEGIN_MESSAGE_MAP(CIconSelectDlg, CBaseDialog)
    ON_CBN_SELCHANGE(IDC_COMBO1, &CIconSelectDlg::OnCbnSelchangeCombo1)
    ON_BN_CLICKED(IDC_AUTO_ADAPT_CHECK, &CIconSelectDlg::OnBnClickedAutoAdaptCheck)
END_MESSAGE_MAP()


// CIconSelectDlg message handlers


BOOL CIconSelectDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    SetIcon(theApp.GetMenuIcon(IDI_NOTIFY), FALSE);		// small icon

    // Material components replace the stock controls. The caption static becomes the dropdown label,
    // the group box becomes a section heading and the bitmap static becomes the self-painted preview.
    CString select_caption;
    GetDlgItemText(IDC_SELECT_ICON_STATIC, select_caption);
    RemoveStockControl(IDC_SELECT_ICON_STATIC);
    m_icon_select_combo = ReplaceWithDropdown(IDC_COMBO1, select_caption);
    m_preview_heading = ReplaceGroupBoxWithHeading(GetDlgItem(IDC_STATIC));
    {
        CWnd* stock_preview = GetDlgItem(IDC_ICON_PREVIEW);
        CRect preview_rect = GetControlRect(stock_preview);
        const HWND previous = ::GetWindow(stock_preview->GetSafeHwnd(), GW_HWNDPREV);
        stock_preview->DestroyWindow();
        preview_rect.right = preview_rect.left + PREVIEW_WIDTH;
        preview_rect.bottom = preview_rect.top + PREVIEW_HEIGHT;
        m_preview_pic.Create(preview_rect, this, IDC_ICON_PREVIEW);
        m_preview_pic.SetWindowPos(previous != nullptr ? CWnd::FromHandle(previous) : &CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        m_preview_pic.SetSurfaceColor(MaterialSurfaceColor());
        m_preview_pic.draw_overlay = [this](CDC& dc) { DrawPreviewIcon(&dc); };
    }
    m_auto_adapt_chk = ReplaceWithCheckbox(IDC_AUTO_ADAPT_CHECK);
    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);
    m_cancel = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);

    LoadPreviewPicture();

    // Initialize the icon list
    m_icon_select_combo->AddItem(CCommon::LoadText(IDS_DEFAULT_ICON));
    m_icon_select_combo->AddItem(CCommon::LoadText(IDS_ICON, _T(" 1")));
    m_icon_select_combo->AddItem(CCommon::LoadText(IDS_ICON, _T(" 2")));
    m_icon_select_combo->AddItem(CCommon::LoadText(IDS_ICON, _T(" 3")));
    m_icon_select_combo->AddItem(CCommon::LoadText(IDS_ICON, _T(" 4")));
    m_icon_select_combo->AddItem(CCommon::LoadText(IDS_ICON, _T(" 5")));
    m_icon_select_combo->SetCurSel(m_icon_selected);

    SetDlgCheck(IDC_AUTO_ADAPT_CHECK, m_atuo_adapt_notify_icon);
    m_auto_adapt_chk->EnableWindow(theApp.m_win_version.GetMajorVersion() >= 10);

    FitMaterialContent(380);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // exception: OCX property pages should return FALSE
}


void CIconSelectDlg::OnCbnSelchangeCombo1()
{
    m_icon_selected = m_icon_select_combo->GetCurSel();
    LoadPreviewPicture();       // repaints the preview, which draws the icon through draw_overlay
}


void CIconSelectDlg::OnBnClickedAutoAdaptCheck()
{
    m_atuo_adapt_notify_icon = GetDlgCheck(IDC_AUTO_ADAPT_CHECK);
}

CString CIconSelectDlg::GetDialogName() const
{
    return _T("IconSelectDlg");
}
