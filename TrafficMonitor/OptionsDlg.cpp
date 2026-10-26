// OptionsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"
#include "NativeMaterial/Controls.h"


// COptionsDlg 对话框

IMPLEMENT_DYNAMIC(COptionsDlg, CBaseDialog)

COptionsDlg::COptionsDlg(int tab, CWnd* pParent /*=NULL*/)
    : CBaseDialog(IDD_OPTIONS_DIALOG, pParent), m_tab_selected{ tab }
{
    EnableMaterialShell();
}

COptionsDlg::~COptionsDlg()
{
}

CString COptionsDlg::GetDialogName() const
{
    return OPTION_DLG_NAME;
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COptionsDlg, CBaseDialog)
    ON_BN_CLICKED(IDC_APPLY_BUTTON, &COptionsDlg::OnBnClickedApplyButton)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &COptionsDlg::OnTabSelectionChanged)
END_MESSAGE_MAP()


// COptionsDlg 消息处理程序

int COptionsDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    const int margin = DPI(16), gap = DPI(12), button_height = DPI(40);
    const int tab_height = m_tab.GetSafeHwnd() != nullptr ? m_tab.PreferredHeight() : DPI(64);
    // The pages scroll, so the window only needs a usable page height; the user may make it taller.
    const int minimum_page_height = DPI(300);
    const int footer = margin + button_height + margin;

    if (!apply)
        return tab_height + minimum_page_height + footer;

    const int page_height = (std::max)(1, static_cast<int>(content.Height()) - tab_height - footer);
    if (m_tab.GetSafeHwnd() != nullptr)
        m_tab.MoveWindow(content.left, content.top, content.Width(), tab_height);

    // 每个子窗口的大小为标签栏下方的整个区域，并设置滚动信息
    const CRect page(content.left, content.top + tab_height, content.right, content.top + tab_height + page_height);
    for (size_t i = 0; i < m_tab_vect.size(); i++)
    {
        m_tab_vect[i]->MoveWindow(page);
        m_tab_vect[i]->SetScrollbarInfo(page.Height(), m_tab_height[i]);
    }

    // Action row: Apply and Cancel as text buttons, OK as the primary action on the right.
    if (m_ok != nullptr && m_cancel != nullptr && m_apply != nullptr)
    {
        CString ok, cancel, apply_text;
        m_ok->GetWindowText(ok);
        m_cancel->GetWindowText(cancel);
        m_apply->GetWindowText(apply_text);
        const int ok_width = MeasureMaterialButtonWidth(ok, NativeMaterial::ButtonVariant::Primary, 88);
        const int cancel_width = MeasureMaterialButtonWidth(cancel, NativeMaterial::ButtonVariant::Text, 88);
        const int apply_width = MeasureMaterialButtonWidth(apply_text, NativeMaterial::ButtonVariant::Text, 88);
        const int y = page.bottom + margin;
        int right = content.right - margin;
        m_ok->MoveWindow(right - ok_width, y, ok_width, button_height);
        right -= ok_width + gap;
        m_cancel->MoveWindow(right - cancel_width, y, cancel_width, button_height);
        right -= cancel_width + gap;
        m_apply->MoveWindow(right - apply_width, y, apply_width, button_height);
    }
    return content.Height();
}

BOOL COptionsDlg::PreTranslateMessage(MSG* pMsg)
{
    // Ctrl+Tab and Ctrl+Shift+Tab cycle the tabs while the focus is on the tab bar or on a page.
    if (m_tab.GetSafeHwnd() != nullptr && m_tab.HandleTabNavigation(*pMsg))
        return TRUE;
    return CBaseDialog::PreTranslateMessage(pMsg);
}

BOOL COptionsDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetIcon(theApp.GetMenuIcon(IDI_SETTINGS), FALSE);       // 设置小图标

    // The system tab control of the template is replaced by the Material tab bar.
    RemoveStockControl(IDC_TAB1);
    m_tab.Create(CRect(0, MaterialContentTop(), DPI(100), MaterialContentTop() + DPI(64)), this, IDC_TAB1, NativeMaterial::TabStyle::Primary);
    CString caption;
    GetWindowText(caption);
    m_tab.SetAccessibleName(caption);
    m_tab.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);   // first tab stop

    //创建子对话框，其父窗口为选项对话框本身
    m_tab1_dlg.Create(IDD_MAIN_WND_SETTINGS_DIALOG, this);
    m_tab2_dlg.Create(IDD_TASKBAR_SETTINGS_DIALOG, this);
    m_tab3_dlg.Create(IDD_GENERAL_SETTINGS_DIALOG, this);

    //保存子对话框
    m_tab_vect.push_back(&m_tab1_dlg);
    m_tab_vect.push_back(&m_tab2_dlg);
    m_tab_vect.push_back(&m_tab3_dlg);

    //获取子对话框的初始高度
    for (const auto* pDlg : m_tab_vect)
    {
        CRect rect;
        pDlg->GetWindowRect(rect);
        m_tab_height.push_back(rect.Height());
    }

    //添加对话框，每个标签带一个图标（Segoe MDL2 Assets 字形）
    m_tab.AddTab(CCommon::LoadText(IDS_MAIN_WINDOW_SETTINGS), 0xE7F4);
    m_tab.AddTab(CCommon::LoadText(IDS_TASKBAR_WINDOW_SETTINGS), 0xE7C4);
    m_tab.AddTab(CCommon::LoadText(IDS_GENERAL_SETTINGS), 0xE713);
    for (size_t i = 0; i < m_tab_vect.size(); i++)
        m_tab.BindPage(static_cast<int>(i), m_tab_vect[i]);

    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);
    m_cancel = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);
    m_apply = ReplaceWithButton(IDC_APPLY_BUTTON, NativeMaterial::ButtonVariant::Text);

    FitMaterialContent(560);
    FinishMaterialShell();

    //设置默认选中的标签
    if (m_tab_selected < 0 || m_tab_selected >= m_tab.GetTabCount())
        m_tab_selected = 0;
    m_tab.SetSelected(m_tab_selected);
    m_tab_vect[m_tab_selected]->OnTabEntered();
    m_tab.SetFocus();

    return FALSE;  // the focus was set to the tab bar
}


void COptionsDlg::OnTabSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    UNREFERENCED_PARAMETER(pNMHDR);
    const int selected = m_tab.GetSelected();
    if (selected >= 0 && selected < static_cast<int>(m_tab_vect.size()))
        m_tab_vect[selected]->OnTabEntered();
    *pResult = 0;
}


void COptionsDlg::OnOK()
{
    // TODO: 在此添加专用代码和/或调用基类
    m_tab1_dlg.OnOK();
    m_tab2_dlg.OnOK();
    m_tab3_dlg.OnOK();

    CBaseDialog::OnOK();
}


void COptionsDlg::OnCancel()
{
    // TODO: 在此添加专用代码和/或调用基类
    m_tab3_dlg.OnCancel();

    CBaseDialog::OnCancel();
}


void COptionsDlg::OnBnClickedApplyButton()
{
    m_tab2_dlg.SaveColorSettingToDefaultStyle();
    ::SendMessage(theApp.m_pMainWnd->GetSafeHwnd(), WM_SETTINGS_APPLIED, (WPARAM)this, 0);
    for (size_t i = 0; i < m_tab_vect.size(); i++)
    {
        m_tab_vect[i]->OnSettingsApplied();
    }
}
