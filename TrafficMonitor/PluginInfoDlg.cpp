// PluginInfoDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "NativeMaterial/AlertDialog.h"
#include "NativeMaterial/Controls.h"
#include "NativeMenus/SearchableMenu.h"
#include "TrafficMonitor.h"
#include "PluginInfoDlg.h"
#include "LegacyMaterialTables.h"
#include "FilePathHelper.h"


// CPluginInfoDlg 对话框

IMPLEMENT_DYNAMIC(CPluginInfoDlg, CBaseDialog)

CPluginInfoDlg::CPluginInfoDlg(int plugin_index, CWnd* pParent /*=nullptr*/)
	: CBaseDialog(IDD_NETWORK_INFO_DIALOG, pParent), m_cur_index(plugin_index)
{
    EnableMaterialShell();
    if (m_cur_index < 0 || m_cur_index >= static_cast<int>(theApp.m_plugins.GetPlugins().size()))
        m_cur_index = 0;
}

CPluginInfoDlg::~CPluginInfoDlg()
{
}

void CPluginInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
}

void CPluginInfoDlg::ShowInfo()
{
    if (m_cur_index >= 0 && m_cur_index < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        auto& plugin_info = theApp.m_plugins.GetPlugins()[m_cur_index];
        m_info_list->SetCellText(RI_NAME, 1, plugin_info.Property(ITMPlugin::TMI_NAME).c_str());
        m_info_list->SetCellText(RI_DESCRIPTION, 1, plugin_info.Property(ITMPlugin::TMI_DESCRIPTION).c_str());
        m_info_list->SetCellText(RI_FILE_NAME, 1, CFilePathHelper(plugin_info.file_path).GetFileName().c_str());
        m_info_list->SetCellText(RI_FILE_PATH, 1, plugin_info.file_path.c_str());
        m_info_list->SetCellText(RI_ITEM_NUM, 1, std::to_wstring(plugin_info.plugin_items.size()).c_str());
        wstring item_names;
        wstring item_id;
        for (const auto& item : plugin_info.plugin_items)
        {
            item_names += item->GetItemName();
            item_names += L";";
            item_id += item->GetItemId();
            item_id += L";";
        }
        if (!plugin_info.plugin_items.empty())
        {
            item_names.pop_back();
            item_id.pop_back();
        }
        m_info_list->SetCellText(RI_ITEM_NAMES, 1, item_names.c_str());
        m_info_list->SetCellText(RI_ITEM_ID, 1, item_id.c_str());
        m_info_list->SetCellText(RI_AUTHOR, 1, plugin_info.Property(ITMPlugin::TMI_AUTHOR).c_str());
        m_info_list->SetCellText(RI_COPYRIGHT, 1, plugin_info.Property(ITMPlugin::TMI_COPYRIGHT).c_str());
        m_info_list->SetCellText(RI_URL, 1, plugin_info.Property(ITMPlugin::TMI_URL).c_str());
        m_info_list->SetCellText(RI_VERSION, 1, plugin_info.Property(ITMPlugin::TMI_VERSION).c_str());
        std::wstring api_version;
        if (plugin_info.plugin != nullptr)
            api_version = std::to_wstring(plugin_info.plugin->GetAPIVersion());
        m_info_list->SetCellText(RI_API_VERSION, 1, api_version.c_str());
    }

    //显示当前选择指示
    CString str;
    str.Format(_T("%d/%d"), m_cur_index + 1, theApp.m_plugins.GetPlugins().size());
    if (m_index_label != nullptr)
        m_index_label->SetWindowText(str);
    RelayoutMaterial();     // the index text can change the width the label needs
}


BEGIN_MESSAGE_MAP(CPluginInfoDlg, CBaseDialog)
    ON_COMMAND(ID_COPY_TEXT, &CPluginInfoDlg::OnCopyText)
    ON_WM_CONTEXTMENU()
    ON_BN_CLICKED(IDC_PREVIOUS_BUTTON, &CPluginInfoDlg::OnBnClickedPreviousButton)
    ON_BN_CLICKED(IDC_NEXT_BUTTON, &CPluginInfoDlg::OnBnClickedNextButton)
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


// CPluginInfoDlg 消息处理程序


BOOL CPluginInfoDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetWindowText(CCommon::LoadText(IDS_PLUGIN_INFO));
    SetIcon(theApp.GetMenuIcon(IDI_PLUGINS), FALSE);		// 设置小图标

    // Material components replace the stock controls.
    m_info_list = LegacyMaterial::ReplaceWithTable<NativeMaterial::DataTable>(*this, IDC_INFO_LIST1, MaterialAppearance(), MaterialSurfaceColor(), CCommon::LoadText(IDS_PLUGIN_INFO));
    m_index_label = ReplaceWithLabel(IDC_INDEX_STATIC);
    m_previous_button = ReplaceWithButton(IDC_PREVIOUS_BUTTON, NativeMaterial::ButtonVariant::Tonal);
    m_next_button = ReplaceWithButton(IDC_NEXT_BUTTON, NativeMaterial::ButtonVariant::Tonal);
    m_close_button = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);

    //初始化列表控件
    NativeMaterial::TableColumn item_column, value_column;
    item_column.title = CCommon::LoadText(IDS_ITEM);
    item_column.width = 180;
    value_column.title = CCommon::LoadText(IDS_VALUE);
    value_column.width = 420;
    m_info_list->SetColumns({ item_column, value_column });

    //向列表中插入行
    for (int i = 0; i < RI_MAX; i++)
    {
        NativeMaterial::TableCell name_cell;
        name_cell.text = GetRowName(i);
        m_info_list->AddRow({ name_cell, NativeMaterial::TableCell() });
    }

    CCommon::LoadMenuResource(m_menu, IDR_INFO_MENU); //装载右键菜单

    FitMaterialContent(640);
    FinishMaterialShell();

    //显示列表中的信息
    ShowInfo();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}

CString CPluginInfoDlg::GetDialogName() const
{
    return _T("PluginInfoDlg");
}

int CPluginInfoDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    // The table on top; below it the previous / next buttons with the position text, and close on the right.
    const int margin = DPI(16), gap = DPI(8), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    const int left = content.left + margin;
    const int minimum_table = DPI(336);
    int table_height = minimum_table;
    if (apply)
        table_height = (std::max)(minimum_table, static_cast<int>(content.Height()) - (margin + gap + button_height + margin));

    int y = content.top + margin;
    if (m_info_list != nullptr && apply)
        m_info_list->MoveWindow(left, y, width, table_height);
    y += table_height + gap;

    int x = left;
    const int arrow_width = DPI(64);
    for (auto* arrow : { m_previous_button, m_next_button })
    {
        if (arrow == nullptr)
            continue;
        if (apply)
            arrow->MoveWindow(x, y, arrow_width, button_height);
        x += arrow_width + gap;
    }
    int close_width = 0;
    if (m_close_button != nullptr)
    {
        CString label;
        m_close_button->GetWindowText(label);
        close_width = MeasureMaterialButtonWidth(label, NativeMaterial::ButtonVariant::Primary, 88);
        if (apply)
            m_close_button->MoveWindow(left + width - close_width, y, close_width, button_height);
    }
    if (m_index_label != nullptr)
    {
        const int label_width = (std::max)(DPI(48), left + width - close_width - gap - x);
        const int label_height = (std::min)(button_height, m_index_label->MeasureHeight(label_width));
        if (apply)
            m_index_label->MoveWindow(x, y + (button_height - label_height) / 2, label_width, label_height);
    }
    y += button_height;
    return y + margin - content.top;
}

void CPluginInfoDlg::OnMaterialAppearanceChanged()
{
    if (m_info_list != nullptr && m_info_list->GetSafeHwnd() != nullptr)
    {
        m_info_list->SetAppearance(MaterialAppearance());
        m_info_list->SetSurfaceColor(MaterialSurfaceColor());
    }
}

void CPluginInfoDlg::OnCopyText()
{
    if (!CCommon::CopyStringToClipboard(wstring(m_selected_string)))
        NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_COPY_TO_CLIPBOARD_FAILED), MB_ICONWARNING);
}

void CPluginInfoDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (m_info_list == nullptr || pWnd == nullptr || pWnd->GetSafeHwnd() != m_info_list->GetSafeHwnd())
    {
        CBaseDialog::OnContextMenu(pWnd, point);
        return;
    }

    //获取鼠标点击处的文本
    const int item = m_info_list->ContextRow();
    const int sub_item = LegacyMaterial::ColumnAtScreenPoint(*m_info_list, item, point);
    m_selected_string = item >= 0 ? m_info_list->GetCellText(item, sub_item) : CString();

    //弹出右键菜单
    CMenu* pContextMenu = m_menu.GetSubMenu(0);	//获取第一个弹出菜单
    NativeMenus::Track(pContextMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this); //在指定位置显示弹出菜单
}

void CPluginInfoDlg::OnBnClickedPreviousButton()
{
    if (m_cur_index > 0)
    {
        m_cur_index--;
        ShowInfo();
    }
}

void CPluginInfoDlg::OnBnClickedNextButton()
{
    if (m_cur_index < static_cast<int>(theApp.m_plugins.GetPlugins().size() - 1))
    {
        m_cur_index++;
        ShowInfo();
    }
}

BOOL CPluginInfoDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_LEFT)
        {
            OnBnClickedPreviousButton();
            return TRUE;
        }
        if (pMsg->wParam == VK_RIGHT)
        {
            OnBnClickedNextButton();
            return TRUE;
        }
    }

    return CBaseDialog::PreTranslateMessage(pMsg);
}


BOOL CPluginInfoDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    //通过鼠标滚轮翻页
    if (zDelta > 0)
    {
        OnBnClickedPreviousButton();
    }
    if (zDelta < 0)
    {
        OnBnClickedNextButton();
    }

    return CBaseDialog::OnMouseWheel(nFlags, zDelta, pt);
}

CString CPluginInfoDlg::GetRowName(int row_index)
{
    switch (row_index)
    {
    case CPluginInfoDlg::RI_NAME:
        return CCommon::LoadText(IDS_NAME);
    case CPluginInfoDlg::RI_DESCRIPTION:
        return CCommon::LoadText(IDS_DESCRIPTION);
    case CPluginInfoDlg::RI_FILE_NAME:
        return CCommon::LoadText(IDS_FILE_NAME);
    case CPluginInfoDlg::RI_FILE_PATH:
        return CCommon::LoadText(IDS_FILE_PATH);
    case CPluginInfoDlg::RI_ITEM_NUM:
        return CCommon::LoadText(IDS_ITEM_NUM);
    case CPluginInfoDlg::RI_ITEM_NAMES:
        return CCommon::LoadText(IDS_ITEM_NAMES);
    case CPluginInfoDlg::RI_ITEM_ID:
        return CCommon::LoadText(IDS_DISP_ITEM_ID);
    case CPluginInfoDlg::RI_AUTHOR:
        return CCommon::LoadText(IDS_AUTHOR);
    case CPluginInfoDlg::RI_COPYRIGHT:
        return CCommon::LoadText(IDS_COPYRIGHT);
    case CPluginInfoDlg::RI_URL:
        return CCommon::LoadText(IDS_URL);
    case CPluginInfoDlg::RI_VERSION:
        return CCommon::LoadText(IDS_VERSION);
    case CPluginInfoDlg::RI_API_VERSION:
        return CCommon::LoadText(IDS_PLUGIN_API_VERSION);
    default:
        break;
    }
    return CString();
}
