// SelectConnectionsDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "NativeMaterial/AlertDialog.h"
#include "NativeMaterial/Controls.h"
#include "TrafficMonitor.h"
#include "SelectConnectionsDlg.h"
#include "LegacyMaterialTables.h"
#include "afxdialogex.h"
#include "AdapterCommon.h"

// CSelectConnectionsDlg 对话框

IMPLEMENT_DYNAMIC(CSelectConnectionsDlg, CBaseDialog)

CSelectConnectionsDlg::CSelectConnectionsDlg(const StringSet& connections_hide, CWnd* pParent /*=nullptr*/)
	: CBaseDialog(IDD_SELECT_CONNECTIONS_DIALOG, pParent), m_connections_hide(connections_hide)
{
    EnableMaterialShell();
    std::vector<NetWorkConection> connections;
    CAdapterCommon::GetAdapterInfo(connections);
    for (const auto& item : connections)
        m_all_connection_names.push_back(CCommon::StrToUnicode(item.description.c_str()));
}

CSelectConnectionsDlg::~CSelectConnectionsDlg()
{
}

void CSelectConnectionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}


CString CSelectConnectionsDlg::GetDialogName() const
{
    //throw std::logic_error("The method or operation is not implemented.");
    return _T("SelectConnectionsDlg");
}

int CSelectConnectionsDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    // The checkable list fills the dialog; OK and Cancel sit on the right below it.
    const int margin = DPI(16), gap = DPI(8), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    const int left = content.left + margin;
    const int minimum_list = DPI(192);
    int list_height = minimum_list;
    if (apply)
        list_height = (std::max)(minimum_list, static_cast<int>(content.Height()) - (margin + gap + button_height + margin));

    int y = content.top + margin;
    if (m_list_ctrl != nullptr && apply)
        m_list_ctrl->MoveWindow(left, y, width, list_height);
    y += list_height + gap;

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

void CSelectConnectionsDlg::OnMaterialAppearanceChanged()
{
    if (m_list_ctrl != nullptr && m_list_ctrl->GetSafeHwnd() != nullptr)
    {
        m_list_ctrl->SetAppearance(MaterialAppearance());
        m_list_ctrl->SetSurfaceColor(MaterialSurfaceColor());
    }
}

BEGIN_MESSAGE_MAP(CSelectConnectionsDlg, CBaseDialog)
END_MESSAGE_MAP()


// CSelectConnectionsDlg 消息处理程序


BOOL CSelectConnectionsDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetIcon(theApp.GetMenuIcon(IDI_CONNECTION), FALSE);

    // Material components replace the stock controls: the check list box becomes a checkable list.
    CString title;
    GetWindowText(title);
    m_list_ctrl = LegacyMaterial::ReplaceWithTable<NativeMaterial::MaterialList>(*this, IDC_LIST1, MaterialAppearance(), MaterialSurfaceColor(), title);
    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);
    m_cancel = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);

    //向列表中添加项目
    m_list_ctrl->SetCheckable(true);
    for (const auto& connection_name : m_all_connection_names)
    {
        const bool checked = !m_connections_hide.Contains(connection_name);
        m_list_ctrl->AddItem(connection_name.c_str(), nullptr, 0, checked);
    }

    FitMaterialContent(420);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}


void CSelectConnectionsDlg::OnOK()
{
    // TODO: 在此添加专用代码和/或调用基类
    //保存每个项目的勾选状态
    bool is_empty{ true };
    for (int i = 0; i < static_cast<int>(m_all_connection_names.size()); i++)
    {
        bool is_checked = m_list_ctrl != nullptr && m_list_ctrl->GetItemChecked(i);
        if (is_checked)
            is_empty = false;
        m_connections_hide.SetStrContained(m_all_connection_names[i], !is_checked);
    }

    if (is_empty)
    {
        NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_SELECT_AT_LEASE_ONE_WARNING), MB_ICONWARNING | MB_OK);
        return;
    }

    CBaseDialog::OnOK();
}
