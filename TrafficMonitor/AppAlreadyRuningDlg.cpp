// AppAlreadyRuningDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "AppAlreadyRuningDlg.h"
#include "afxdialogex.h"
#include "NativeMaterial/Controls.h"


// CAppAlreadyRuningDlg 对话框

IMPLEMENT_DYNAMIC(CAppAlreadyRuningDlg, CBaseDialog)

CAppAlreadyRuningDlg::CAppAlreadyRuningDlg(HWND handel, CWnd* pParent /*=nullptr*/)
    : CBaseDialog(IDD_APP_ALREAD_RUNING_DIALOG, pParent), m_handle(handel)
{
    EnableMaterialShell();
}

CAppAlreadyRuningDlg::~CAppAlreadyRuningDlg()
{
}

void CAppAlreadyRuningDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

CString CAppAlreadyRuningDlg::GetDialogName() const
{
    return _T("AppAlreadyRuningDlg");
}


BEGIN_MESSAGE_MAP(CAppAlreadyRuningDlg, CBaseDialog)
    ON_BN_CLICKED(IDC_EXIT_INST_BUTTON, &CAppAlreadyRuningDlg::OnBnClickedExitInstButton)
    ON_BN_CLICKED(IDC_OPEN_SETTINGS_BUTTON, &CAppAlreadyRuningDlg::OnBnClickedOpenSettingsButton)
    ON_BN_CLICKED(IDC_SHOW_HIDE_MAIN_WINDOW_BUTTON, &CAppAlreadyRuningDlg::OnBnClickedShowHideMainWindowButton)
    ON_BN_CLICKED(IDC_SHOW_HIDE_TASKBAR_WINDOW_BUTTON, &CAppAlreadyRuningDlg::OnBnClickedShowHideTaskbarWindowButton)
END_MESSAGE_MAP()


// CAppAlreadyRuningDlg 消息处理程序


BOOL CAppAlreadyRuningDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), FALSE);

    // Material shell: the message becomes a Label and the four actions become Material buttons (control IDs unchanged)
    m_message = ReplaceWithLabel(IDC_STATIC);
    const UINT button_ids[4] = { IDC_EXIT_INST_BUTTON, IDC_OPEN_SETTINGS_BUTTON, IDC_SHOW_HIDE_MAIN_WINDOW_BUTTON, IDC_SHOW_HIDE_TASKBAR_WINDOW_BUTTON };
    for (int i = 0; i < 4; i++)
        m_actions[i] = ReplaceWithButton(button_ids[i], NativeMaterial::ButtonVariant::Tonal);
    FitMaterialContent(320);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}


int CAppAlreadyRuningDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    const int margin = DPI(20), gap = DPI(8), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    int y = content.top + margin;
    if (m_message != nullptr)
    {
        const int height = m_message->MeasureHeight(width);
        if (apply)
            m_message->MoveWindow(content.left + margin, y, width, height);
        y += height + DPI(16);
    }
    for (auto* button : m_actions)
    {
        if (button == nullptr)
            continue;
        if (apply)
            button->MoveWindow(content.left + margin, y, width, button_height);
        y += button_height + gap;
    }
    return y - gap + margin - content.top;
}


void CAppAlreadyRuningDlg::OnBnClickedExitInstButton()
{
    ::PostMessage(m_handle, WM_COMMAND, ID_APP_EXIT, 0);
}


void CAppAlreadyRuningDlg::OnBnClickedOpenSettingsButton()
{
    ::PostMessage(m_handle, WM_COMMAND, ID_OPTIONS, 0);
}


void CAppAlreadyRuningDlg::OnBnClickedShowHideMainWindowButton()
{
    ::PostMessage(m_handle, WM_COMMAND, ID_SHOW_MAIN_WND, 0);
}


void CAppAlreadyRuningDlg::OnBnClickedShowHideTaskbarWindowButton()
{
    ::PostMessage(m_handle, WM_COMMAND, ID_SHOW_TASK_BAR_WND, 0);
}
