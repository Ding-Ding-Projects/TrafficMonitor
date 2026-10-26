// PluginManagerDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "NativeMaterial/AlertDialog.h"
#include "NativeMaterial/Controls.h"
#include "NativeMenus/SearchableMenu.h"
#include "TrafficMonitor.h"
#include "PluginManagerDlg.h"
#include "LegacyMaterialTables.h"
#include "FilePathHelper.h"
#include "PluginInfoDlg.h"
#include "WIC.h"


// CPluginManagerDlg 对话框

IMPLEMENT_DYNAMIC(CPluginManagerDlg, CBaseDialog)

CPluginManagerDlg::CPluginManagerDlg(CWnd* pParent /*=nullptr*/)
    : CBaseDialog(IDD_PLUGIN_MANAGER_DIALOG, pParent)
{
    EnableMaterialShell();
}

CPluginManagerDlg::~CPluginManagerDlg()
{
}

void CPluginManagerDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

void CPluginManagerDlg::EnableControl()
{
    bool enable{ IsSelectedPluginEnable() };
    EnableDlgCtrl(IDC_OPTINS_BUTTON, enable);
    EnableDlgCtrl(IDC_PLUGIN_INFO_BUTTON, enable);
}

bool CPluginManagerDlg::IsSelectedValid()
{
    return m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size());
}

bool CPluginManagerDlg::IsSelectedPluginEnable()
{
    CPluginManager::PluginInfo plugin_info{};
    bool plugin_loaded{ false };
    if (IsSelectedValid())
    {
        plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        plugin_loaded = (plugin_info.state == CPluginManager::PluginState::PS_SUCCEED);
    }

    return plugin_loaded;
}

CString CPluginManagerDlg::GetDialogName() const
{
    return _T("PluginManagerDlg");
}

int CPluginManagerDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    // The plugin table on top, a row of text buttons for the links, then the action row.
    const int margin = DPI(16), gap = DPI(8), link_height = DPI(36), button_height = DPI(40), link_inset = DPI(12), link_gap = DPI(4);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    const int left = content.left + margin;
    const int minimum_table = DPI(240);
    int table_height = minimum_table;
    if (apply)
        table_height = (std::max)(minimum_table, static_cast<int>(content.Height()) - (margin + gap + link_height + gap + button_height + margin));

    int y = content.top + margin;
    if (m_list_ctrl != nullptr && apply)
        m_list_ctrl->MoveWindow(left, y, width, table_height);
    y += table_height + gap;

    // Links: text buttons whose label lines up with the table edge.
    {
        int x = left - link_inset;
        for (auto* link : { m_plugin_download_lnk, m_plugin_dev_guide_lnk, m_open_plugin_dir_lnk })
        {
            if (link == nullptr)
                continue;
            CString label;
            link->GetWindowText(label);
            const int link_width = MeasureMaterialButtonWidth(label, NativeMaterial::ButtonVariant::Text, 48);
            if (apply)
                link->MoveWindow(x, y, link_width, link_height);
            x += link_width + link_gap;
        }
        y += link_height + gap;
    }

    // Actions: details and options on the left, close on the right.
    {
        int x = left;
        for (auto* action : { m_info_button, m_options_button })
        {
            if (action == nullptr)
                continue;
            CString label;
            action->GetWindowText(label);
            const int action_width = MeasureMaterialButtonWidth(label, NativeMaterial::ButtonVariant::Tonal, 88);
            if (apply)
                action->MoveWindow(x, y, action_width, button_height);
            x += action_width + gap;
        }
        if (m_close_button != nullptr)
        {
            CString label;
            m_close_button->GetWindowText(label);
            const int close_width = MeasureMaterialButtonWidth(label, NativeMaterial::ButtonVariant::Text, 88);
            if (apply)
                m_close_button->MoveWindow(left + width - close_width, y, close_width, button_height);
        }
        y += button_height;
    }
    return y + margin - content.top;
}

void CPluginManagerDlg::OnMaterialAppearanceChanged()
{
    if (m_list_ctrl != nullptr && m_list_ctrl->GetSafeHwnd() != nullptr)
    {
        m_list_ctrl->SetAppearance(MaterialAppearance());
        m_list_ctrl->SetSurfaceColor(MaterialSurfaceColor());
    }
}


BEGIN_MESSAGE_MAP(CPluginManagerDlg, CBaseDialog)
    ON_CONTROL(NativeMaterial::TableSelectionChanged, IDC_LIST1, &CPluginManagerDlg::OnTableSelectionChanged)
    ON_CONTROL(NativeMaterial::TableActivated, IDC_LIST1, &CPluginManagerDlg::OnTableActivated)
    ON_WM_CONTEXTMENU()
    ON_BN_CLICKED(IDC_OPTINS_BUTTON, &CPluginManagerDlg::OnBnClickedOptinsButton)
    ON_BN_CLICKED(IDC_PLUGIN_INFO_BUTTON, &CPluginManagerDlg::OnBnClickedPluginInfoButton)
    ON_WM_INITMENU()
    ON_COMMAND(ID_PLUGIN_DETAIL, &CPluginManagerDlg::OnPluginDetail)
    ON_COMMAND(ID_PLUGIN_OPTIONS, &CPluginManagerDlg::OnPluginOptions)
    ON_COMMAND(ID_PLUGIN_DISABLE, &CPluginManagerDlg::OnPluginDisable)
END_MESSAGE_MAP()


// CPluginManagerDlg 消息处理程序


BOOL CPluginManagerDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetIcon(theApp.GetMenuIcon(IDI_PLUGINS), FALSE);

    // Material components replace the stock controls: the list view becomes a data table (the plugin
    // option dialogs themselves are drawn by the plugin libraries and stay outside this conversion) and
    // the three link statics become text buttons that keep their control IDs.
    CString title;
    GetWindowText(title);
    m_list_ctrl = LegacyMaterial::ReplaceWithTable<NativeMaterial::DataTable>(*this, IDC_LIST1, MaterialAppearance(), MaterialSurfaceColor(), title);
    m_plugin_download_lnk = ReplaceWithButton(IDC_PLUGIN_DOWNLOAD_STATIC, NativeMaterial::ButtonVariant::Text);
    m_plugin_dev_guide_lnk = ReplaceWithButton(IDC_PLUGIN_DEV_GUID_STATIC, NativeMaterial::ButtonVariant::Text);
    m_open_plugin_dir_lnk = ReplaceWithButton(IDC_OPEN_PLUGIN_DIR_STATIC, NativeMaterial::ButtonVariant::Text);
    m_info_button = ReplaceWithButton(IDC_PLUGIN_INFO_BUTTON, NativeMaterial::ButtonVariant::Tonal);
    m_options_button = ReplaceWithButton(IDC_OPTINS_BUTTON, NativeMaterial::ButtonVariant::Tonal);
    m_close_button = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);

    //初始化列表控件
    using NativeMaterial::TableColumn;
    TableColumn file_name_column, name_column, version_column, status_column;
    file_name_column.title = CCommon::LoadText(IDS_FILE_NAME);
    file_name_column.width = 170;
    name_column.title = CCommon::LoadText(IDS_PLUGIN_NAME);
    name_column.width = 150;
    version_column.title = CCommon::LoadText(IDS_VERSION);
    version_column.width = 130;
    status_column.title = CCommon::LoadText(IDS_STATUS);
    status_column.width = 220;
    m_list_ctrl->SetColumns({ file_name_column, name_column, version_column, status_column });

    //向列表中插入行
    for (const auto& plugin : theApp.m_plugins.GetPlugins())
    {
        std::wstring file_name = CFilePathHelper(plugin.file_path).GetFileName();

        //图标
        HICON hIcon{};
        if (plugin.plugin != nullptr && plugin.plugin->GetAPIVersion() >= 5)
        {
            hIcon = (HICON)plugin.plugin->GetPluginIcon();
        }
        if (hIcon == nullptr)
        {
            if (plugin.state == CPluginManager::PluginState::PS_DISABLE)
                hIcon = theApp.GetMenuIcon(IDI_PLUGIN_DISABLED);
            else
                hIcon = theApp.GetMenuIcon(IDI_PLUGINS);
        }

        CString error_info;
        error_info.Format(_T("%u %s"), plugin.error_code, CCommon::GetErrorMessage(plugin.error_code).GetString());
        CString status;
        // The status column shows a colored indicator in front of the text: green when loaded, grey when
        // disabled, amber for an unsupported version and red for a load failure.
        COLORREF status_color{ RGB(158, 158, 158) };
        switch (plugin.state)
        {
        case CPluginManager::PluginState::PS_SUCCEED:
            status = CCommon::LoadText(IDS_PLUGIN_LOAD_SUCCEED);
            status_color = RGB(46, 160, 67);
            break;
        case CPluginManager::PluginState::PS_MUDULE_LOAD_FAILED:
            status = CCommon::LoadTextFormat(IDS_PLUGIN_MODULE_LOAD_FAILED, { error_info });
            status_color = RGB(217, 48, 37);
            break;
        case CPluginManager::PluginState::PS_FUNCTION_GET_FAILED:
            status = CCommon::LoadTextFormat(IDS_PLUGIN_FUNCTION_GET_FAILED, { error_info });
            status_color = RGB(217, 48, 37);
            break;
        case CPluginManager::PluginState::PS_DISABLE:
            status = CCommon::LoadText(IDS_DISABLED);
            break;
        case CPluginManager::PluginState::PS_VERSION_NOT_SUPPORT:
            status = CCommon::LoadText(IDS_PLUGIN_VERSION_NOT_SUPPORT);
            status_color = RGB(240, 160, 0);
            break;
        default:
            break;
        }

        NativeMaterial::TableCell file_cell, name_cell, version_cell, status_cell;
        file_cell.text = file_name.c_str();
        file_cell.icon = hIcon;
        name_cell.text = plugin.Property(ITMPlugin::TMI_NAME).c_str();
        if (plugin.state == CPluginManager::PluginState::PS_SUCCEED)
        {
            //如果插件有更新，在版本号后添加更新信息
            PluginVersion cur_version(plugin.Property(ITMPlugin::TMI_VERSION));
            PluginVersion latest_version(theApp.m_plugin_update.GetPluginLatestVersions(file_name));
            if (cur_version < latest_version)
            {
                std::wstring update_info = theApp.m_str_table.LoadTextFormat(IDS_PLUGIN_NEW_VERSION_INFO, { latest_version.GetVersionWString() });
                std::wstring version_text = plugin.Property(ITMPlugin::TMI_VERSION);
                version_text += L" (";
                version_text += update_info;
                version_text += L')';
                version_cell.text = version_text.c_str();
            }
            else
            {
                version_cell.text = plugin.Property(ITMPlugin::TMI_VERSION).c_str();
            }
        }
        status_cell.text = status;
        status_cell.hasSwatch = true;
        status_cell.swatch = status_color;
        m_list_ctrl->AddRow({ file_cell, name_cell, version_cell, status_cell }, static_cast<LPARAM>(m_list_ctrl->GetRowCount()));
    }

    //超链接：按钮点击时在OnCommand中打开
    if (theApp.m_general_data.update_source == 1)   //更新源为Gitee，跳转到Gitee的链接
        m_urls[IDC_PLUGIN_DOWNLOAD_STATIC] = L"https://gitee.com/zhongyang219/TrafficMonitorPlugins/blob/main/download/plugin_download.md";
    else //更新源为Github
        m_urls[IDC_PLUGIN_DOWNLOAD_STATIC] = L"https://github.com/zhongyang219/TrafficMonitorPlugins/blob/main/download/plugin_download.md";
    m_urls[IDC_PLUGIN_DEV_GUID_STATIC] = L"https://github.com/zhongyang219/TrafficMonitor/wiki/%E6%8F%92%E4%BB%B6%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97";

    EnableControl();

    CCommon::LoadMenuResource(m_menu, IDR_PLUGIN_MANAGER_MENU); //装载右键菜单

    //设置菜单图标
    CMenuIcon::AddIconToMenuItem(m_menu.GetSubMenu(0)->GetSafeHmenu(), ID_PLUGIN_DETAIL, FALSE, theApp.GetMenuIcon(IDI_INFO));
    CMenuIcon::AddIconToMenuItem(m_menu.GetSubMenu(0)->GetSafeHmenu(), ID_PLUGIN_OPTIONS, FALSE, theApp.GetMenuIcon(IDI_SETTINGS));

    FitMaterialContent(640);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}


void CPluginManagerDlg::OnTableSelectionChanged()
{
    const int row = m_list_ctrl != nullptr ? m_list_ctrl->GetSelectedRow() : -1;
    m_item_selected = row >= 0 ? static_cast<int>(m_list_ctrl->GetRowData(row)) : -1;
    EnableControl();
}


void CPluginManagerDlg::OnTableActivated()
{
    const int row = m_list_ctrl != nullptr ? m_list_ctrl->ActivatedRow() : -1;
    m_item_selected = row >= 0 ? static_cast<int>(m_list_ctrl->GetRowData(row)) : -1;
    OnBnClickedPluginInfoButton();
}


void CPluginManagerDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (m_list_ctrl == nullptr || pWnd == nullptr || pWnd->GetSafeHwnd() != m_list_ctrl->GetSafeHwnd())
    {
        CBaseDialog::OnContextMenu(pWnd, point);
        return;
    }

    const int row = m_list_ctrl->ContextRow();
    m_item_selected = row >= 0 ? static_cast<int>(m_list_ctrl->GetRowData(row)) : -1;
    EnableControl();

    //弹出右键菜单
    CMenu* pContextMenu = m_menu.GetSubMenu(0);	//获取第一个弹出菜单
    pContextMenu->SetDefaultItem(ID_PLUGIN_DETAIL);
    //更新插件子菜单
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        auto plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        theApp.UpdatePluginMenu(pContextMenu, plugin_info.plugin, 3);
    }

    NativeMenus::Track(pContextMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this); //在指定位置显示弹出菜单
}


void CPluginManagerDlg::OnBnClickedOptinsButton()
{
    // TODO: 在此添加控件通知处理程序代码
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        auto plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        if (plugin_info.plugin != nullptr)
        {
            // The options dialog is drawn by the plugin library itself (outside the Material conversion).
            ITMPlugin::OptionReturn rtn = plugin_info.plugin->ShowOptionsDialog(m_hWnd);
            if (rtn == ITMPlugin::OR_OPTION_NOT_PROVIDED)
                NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_PLUGIN_NO_OPTIONS_INFO), MB_ICONINFORMATION | MB_OK);
            //else if (rtn == ITMPlugin::OR_OPTION_CHANGED)
            //    theApp.m_pMainWnd->SendMessage(WM_REOPEN_TASKBAR_WND);
        }
    }
}


void CPluginManagerDlg::OnBnClickedPluginInfoButton()
{
    // TODO: 在此添加控件通知处理程序代码
    if (IsSelectedPluginEnable())
    {
        CPluginInfoDlg dlg(m_item_selected);
        dlg.DoModal();
    }
}


void CPluginManagerDlg::OnInitMenu(CMenu* pMenu)
{
    CBaseDialog::OnInitMenu(pMenu);

    // TODO: 在此处添加消息处理程序代码
    bool enable{ IsSelectedPluginEnable() };
    pMenu->EnableMenuItem(ID_PLUGIN_DETAIL, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
    pMenu->EnableMenuItem(ID_PLUGIN_OPTIONS, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
    pMenu->EnableMenuItem(ID_PLUGIN_DISABLE, MF_BYCOMMAND | (IsSelectedValid() ? MF_ENABLED : MF_GRAYED));

    bool disabled{};
    CPluginManager::PluginInfo plugin_info;
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        std::wstring file_name = CFilePathHelper(plugin_info.file_path).GetFileName();
        disabled = theApp.m_cfg_data.plugin_disabled.Contains(file_name);
    }
    pMenu->CheckMenuItem(ID_PLUGIN_DISABLE, MF_BYCOMMAND | (disabled ? MF_CHECKED : MF_UNCHECKED));
}


void CPluginManagerDlg::OnPluginDetail()
{
    OnBnClickedPluginInfoButton();
}


void CPluginManagerDlg::OnPluginOptions()
{
    OnBnClickedOptinsButton();
}


void CPluginManagerDlg::OnPluginDisable()
{
    if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
    {
        CPluginManager::PluginInfo plugin_info = theApp.m_plugins.GetPlugins()[m_item_selected];
        std::wstring file_name = CFilePathHelper(plugin_info.file_path).GetFileName();
        bool disabled = theApp.m_cfg_data.plugin_disabled.Contains(file_name);
        theApp.m_cfg_data.plugin_disabled.SetStrContained(file_name, !disabled);
        NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_RESTART_TO_APPLY_CHANGE_INFO), MB_OK | MB_ICONINFORMATION);
    }
}


BOOL CPluginManagerDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT uMsg = LOWORD(wParam);

    //点击了链接按钮
    if (HIWORD(wParam) == BN_CLICKED)
    {
        const auto url = m_urls.find(uMsg);
        if (url != m_urls.end() && !url->second.IsEmpty())
        {
            ShellExecute(NULL, _T("open"), url->second, NULL, NULL, SW_SHOW);
            return TRUE;
        }
        //点击了“打开插件目录”
        if (uMsg == IDC_OPEN_PLUGIN_DIR_STATIC)
        {
            wstring plugin_dir = CCommon::GetModuleDir() + L"plugins";
            CreateDirectory(plugin_dir.c_str(), NULL);       //如果plugins不存在，则创建它
            ShellExecute(NULL, _T("open"), _T("explorer"), plugin_dir.c_str(), NULL, SW_SHOWNORMAL);
            return TRUE;
        }
    }

    //选择了插件命令
    if (uMsg >= ID_PLUGIN_COMMAND_START && uMsg <= ID_PLUGIN_COMMAND_MAX)
    {
        int index = uMsg - ID_PLUGIN_COMMAND_START;
        if (m_item_selected >= 0 && m_item_selected < static_cast<int>(theApp.m_plugins.GetPlugins().size()))
        {
            ITMPlugin* plugin = theApp.m_plugins.GetPlugins()[m_item_selected].plugin;
            if (plugin != nullptr && plugin->GetAPIVersion() >= 5)
            {
                plugin->OnPluginCommand(index, (void*)GetSafeHwnd(), nullptr);
            }
        }
    }

    return CBaseDialog::OnCommand(wParam, lParam);
}
