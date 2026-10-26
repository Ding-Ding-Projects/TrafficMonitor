// CNetworkInfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "NativeMaterial/AlertDialog.h"
#include "NativeMaterial/Controls.h"
#include "NativeMenus/SearchableMenu.h"
#include "TrafficMonitor.h"
#include "NetworkInfoDlg.h"
#include "LegacyMaterialTables.h"
#include "afxdialogex.h"


// CNetworkInfoDlg 对话框

IMPLEMENT_DYNAMIC(CNetworkInfoDlg, CBaseDialog)

// Posted by the IP address thread; the table may only be changed on the dialog thread.
static constexpr UINT WM_NETWORK_INFO_SET_TEXT = WM_APP + 0x4E31;

CNetworkInfoDlg::CNetworkInfoDlg(vector<NetWorkConection>& adapters, MIB_IFROW* pIfRow, int connection_selected, CWnd* pParent /*=NULL*/)
    : CBaseDialog(IDD_NETWORK_INFO_DIALOG, pParent), m_connections(adapters), m_pIfRow(pIfRow), m_connection_selected(connection_selected)
{
    EnableMaterialShell();
    m_current_connection = connection_selected;
}

CNetworkInfoDlg::~CNetworkInfoDlg()
{
}


void CNetworkInfoDlg::ShowInfo()
{
    CString temp;
    MIB_IFROW& network_info = GetConnectIfTable(m_connection_selected);
    //接口名
    SetInfoText(0, network_info.wszName);
    //接口描述
    SetInfoText(1, CCommon::StrToUnicode((const char*)network_info.bDescr).c_str());
    //连接类型
    switch (network_info.dwType)
    {
    case IF_TYPE_OTHER: temp = CCommon::LoadText(IDS_IF_TYPE_OTHER); break;
    case IF_TYPE_ETHERNET_CSMACD: temp = CCommon::LoadText(IDS_IF_TYPE_ETHERNET_CSMACD); break;
    case IF_TYPE_ISO88025_TOKENRING: temp = CCommon::LoadText(IDS_IF_TYPE_ISO88025_TOKENRING); break;
    case IF_TYPE_FDDI: temp = CCommon::LoadText(IDS_IF_TYPE_FDDI); break;
    case IF_TYPE_PPP: temp = CCommon::LoadText(IDS_IF_TYPE_PPP); break;
    case IF_TYPE_SOFTWARE_LOOPBACK: temp = CCommon::LoadText(IDS_IF_TYPE_SOFTWARE_LOOPBACK); break;
    case IF_TYPE_ATM: temp = CCommon::LoadText(IDS_IF_TYPE_ATM); break;
    case IF_TYPE_IEEE80211: temp = CCommon::LoadText(IDS_IF_TYPE_IEEE80211); break;
    case IF_TYPE_TUNNEL: temp = CCommon::LoadText(IDS_IF_TYPE_TUNNEL); break;
    case IF_TYPE_IEEE1394: temp = CCommon::LoadText(IDS_IF_TYPE_IEEE1394); break;
    case IF_TYPE_IEEE80216_WMAN: temp = CCommon::LoadText(IDS_IF_TYPE_IEEE80216_WMAN); break;
    case IF_TYPE_WWANPP: temp = CCommon::LoadText(IDS_IF_TYPE_WWANPP); break;
    case IF_TYPE_WWANPP2: temp = CCommon::LoadText(IDS_IF_TYPE_WWANPP2); break;
    default: temp = CCommon::LoadText(IDS_UNKNOW_CONNECTION); break;
    }
    SetInfoText(2, temp);
    //速度
    temp.Format(_T("%dMbps"), network_info.dwSpeed / 1000000);
    SetInfoText(3, temp);
    //适配器物理地址
    temp = _T("");
    char buff[3];
    for (size_t i{}; i < network_info.dwPhysAddrLen; i++)
    {
        sprintf_s(buff, "%.2x", network_info.bPhysAddr[i]);
        temp += buff;
        if (i != network_info.dwPhysAddrLen - 1)
            temp += _T('-');
    }
    SetInfoText(4, temp);
    //IP地址
    SetInfoText(5, GetConnection(m_connection_selected).ip_address.c_str());
    //子网掩码
    SetInfoText(6, GetConnection(m_connection_selected).subnet_mask.c_str());
    //默认网关
    SetInfoText(7, GetConnection(m_connection_selected).default_gateway.c_str());
    //连接状态
    switch (network_info.dwOperStatus)
    {
    case IF_OPER_STATUS_NON_OPERATIONAL: temp = CCommon::LoadText(IDS_IF_OPER_STATUS_NON_OPERATIONAL); break;
    case IF_OPER_STATUS_UNREACHABLE: temp = CCommon::LoadText(IDS_IF_OPER_STATUS_UNREACHABLE); break;
    case IF_OPER_STATUS_DISCONNECTED: temp = CCommon::LoadText(IDS_IF_OPER_STATUS_DISCONNECTED); break;
    case IF_OPER_STATUS_CONNECTING: temp = CCommon::LoadText(IDS_IF_OPER_STATUS_CONNECTING); break;
    case IF_OPER_STATUS_CONNECTED: temp = CCommon::LoadText(IDS_IF_OPER_STATUS_CONNECTED); break;
    case IF_OPER_STATUS_OPERATIONAL: temp = CCommon::LoadText(IDS_IF_OPER_STATUS_OPERATIONAL); break;
        //case IfOperStatusUp:
        //    temp = CCommon::LoadText(IDS_IF_OPER_STATUS_UP);
        //    break;
        //case IfOperStatusDown:
        //case IfOperStatusNotPresent:
        //case IfOperStatusLowerLayerDown:
        //    temp = CCommon::LoadText(IDS_IF_OPER_STATUS_DOWN);
        //    break;
        //case IfOperStatusTesting:
        //case IfOperStatusUnknown:
        //    temp = CCommon::LoadText(IDS_UNKNOW_STATUS);
        //    break;
        //case IfOperStatusDormant:
        //    temp = CCommon::LoadText(IDS_IF_OPER_STATUS_DORMANT);
            //break;
    default: temp = CCommon::LoadText(IDS_UNKNOW_STATUS); break;
    }
    SetInfoText(8, temp);
    //已接收字节数
    temp.Format(_T("%s (%s)"), CCommon::IntToString(network_info.dwInOctets, true, true), CCommon::DataSizeToString(network_info.dwInOctets));
    SetInfoText(9, temp);
    //已发送字节数
    temp.Format(_T("%s (%s)"), CCommon::IntToString(network_info.dwOutOctets, true, true), CCommon::DataSizeToString(network_info.dwOutOctets));
    SetInfoText(10, temp);
    //自程序启动以来已接收字节数
    unsigned __int64 in_bytes_since_start;
    in_bytes_since_start = network_info.dwInOctets - GetConnection(m_connection_selected).in_bytes;
    temp.Format(_T("%s (%s)"), CCommon::IntToString(in_bytes_since_start, true, true), CCommon::DataSizeToString(in_bytes_since_start));
    SetInfoText(11, temp);
    //自程序启动以来已发送字节数
    unsigned __int64 out_bytes_since_start;
    out_bytes_since_start = network_info.dwOutOctets - GetConnection(m_connection_selected).out_bytes;
    temp.Format(_T("%s (%s)"), CCommon::IntToString(out_bytes_since_start, true, true), CCommon::DataSizeToString(out_bytes_since_start));
    SetInfoText(12, temp);

    //显示当前选择指示
    CString str;
    str.Format(_T("%d/%d"), m_connection_selected + 1, m_connections.size());
    CFont* font = GetFont();
    if (m_index_label != nullptr)
    {
        m_index_label->SetWindowText(str);
        if (m_current_connection == m_connection_selected && !theApp.m_cfg_data.m_select_all)
            m_index_label->SetFont(&m_font_bold);
        else
            m_index_label->SetFont(font);
        m_index_label->Invalidate(FALSE);
    }
}

void CNetworkInfoDlg::GetProgramElapsedTime()
{
    //程序已运行时间
    SYSTEMTIME current_time, time;
    GetLocalTime(&current_time);
    time = CCommon::CompareSystemTime(current_time, m_start_time);
    CString temp;
    temp.Format(CCommon::LoadText(IDS_HOUR_MINUTE_SECOND), time.wHour, time.wMinute, time.wSecond);
    SetInfoText(13, temp);
}

MIB_IFROW& CNetworkInfoDlg::GetConnectIfTable(int connection_index)
{
    static MIB_IFROW nouse{};
    if (connection_index >= 0 && connection_index < static_cast<int>(m_connections.size()))
    {
        int index = m_connections[connection_index].index;
        if (m_pIfRow != nullptr)
            return m_pIfRow[index];
    }
    return nouse;
}

NetWorkConection CNetworkInfoDlg::GetConnection(int connection_index)
{
    if (connection_index >= 0 && connection_index < static_cast<int>(m_connections.size()))
        return m_connections[connection_index];
    else
        return NetWorkConection();
}

UINT CNetworkInfoDlg::GetInternetIPThreadFunc(LPVOID lpParam)
{
    CCommon::SetThreadLanguage(theApp.m_general_data.language.language_id);		//设置线程语言
    CNetworkInfoDlg* p_instance = (CNetworkInfoDlg*)lpParam;
    wstring ip_address, ip_location;

    //IPV4
    CCommon::GetInternetIp2(ip_address, ip_location, false);			//获取外网IP地址，
    if (!IsWindow(p_instance->GetSafeHwnd()))		//如果当前对话框已经销毁，则退出线程
        return 0;
    if (!ip_address.empty())
    {
        CString info;
        if (ip_location.empty())
            info = ip_address.c_str();
        else
            info.Format(_T("%s (%s)"), ip_address.c_str(), ip_location.c_str());
        p_instance->PostInfoText(14, info);
    }
    else
    {
        p_instance->PostInfoText(14, CCommon::LoadText(IDS_GET_FAILED));
    }

    //IPV6
    wstring ipv6_address, ipv6_location;
    CCommon::GetInternetIp2(ip_address, ip_location, true);			//获取外网IP地址，
    if (!IsWindow(p_instance->GetSafeHwnd()))		//如果当前对话框已经销毁，则退出线程
        return 0;
    if (!ip_address.empty())
    {
        CString info;
        if (ip_location.empty())
            info = ip_address.c_str();
        else
            info.Format(_T("%s (%s)"), ip_address.c_str(), ip_location.c_str());
        p_instance->PostInfoText(15, info);
    }
    else
    {
        p_instance->PostInfoText(15, CCommon::LoadText(IDS_GET_FAILED));
    }

    p_instance->m_ip_acquired = true;
    return 0;
}

CString CNetworkInfoDlg::GetDialogName() const
{
    return _T("NetworkInfoDlg");
}

void CNetworkInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

void CNetworkInfoDlg::SetInfoText(int row, const CString& text)
{
    if (m_info_list != nullptr)
        m_info_list->SetCellText(row, 1, text);
}

void CNetworkInfoDlg::PostInfoText(int row, const CString& text)
{
    // The table is not thread safe: hand the text to the dialog thread.
    HWND hwnd = GetSafeHwnd();
    if (!::IsWindow(hwnd))
        return;
    auto* copy = new CString(text);
    if (!::PostMessage(hwnd, WM_NETWORK_INFO_SET_TEXT, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(copy)))
        delete copy;
}

LRESULT CNetworkInfoDlg::OnSetInfoText(WPARAM row, LPARAM text)
{
    std::unique_ptr<CString> owned(reinterpret_cast<CString*>(text));
    if (owned != nullptr)
        SetInfoText(static_cast<int>(row), *owned);
    return 0;
}

int CNetworkInfoDlg::LayoutMaterialControls(const CRect& content, bool apply)
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

void CNetworkInfoDlg::OnMaterialAppearanceChanged()
{
    if (m_info_list != nullptr && m_info_list->GetSafeHwnd() != nullptr)
    {
        m_info_list->SetAppearance(MaterialAppearance());
        m_info_list->SetSurfaceColor(MaterialSurfaceColor());
    }
}


BEGIN_MESSAGE_MAP(CNetworkInfoDlg, CBaseDialog)
    ON_COMMAND(ID_COPY_TEXT, &CNetworkInfoDlg::OnCopyText)
    ON_WM_CONTEXTMENU()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_PREVIOUS_BUTTON, &CNetworkInfoDlg::OnBnClickedPreviousButton)
    ON_BN_CLICKED(IDC_NEXT_BUTTON, &CNetworkInfoDlg::OnBnClickedNextButton)
    ON_WM_TIMER()
    ON_WM_MOUSEWHEEL()
    ON_CONTROL(NativeMaterial::TableActivated, IDC_INFO_LIST1, &CNetworkInfoDlg::OnTableActivated)
    ON_MESSAGE(WM_NETWORK_INFO_SET_TEXT, &CNetworkInfoDlg::OnSetInfoText)
END_MESSAGE_MAP()


// CNetworkInfoDlg 消息处理程序


BOOL CNetworkInfoDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetIcon(theApp.GetMenuIcon(IDI_INFO), FALSE);		// 设置小图标

    //重新获取IP地址
    CAdapterCommon::RefreshIpAddress(m_connections);

    // Material components replace the stock controls.
    CString title;
    GetWindowText(title);
    m_info_list = LegacyMaterial::ReplaceWithTable<NativeMaterial::DataTable>(*this, IDC_INFO_LIST1, MaterialAppearance(), MaterialSurfaceColor(), title);
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
    const CString row_names[] = {
        CCommon::LoadText(IDS_INTERFACE_NAME),
        CCommon::LoadText(IDS_INTERFACE_DESCRIPTION),
        CCommon::LoadText(IDS_CONNECTION_TYPE),
        CCommon::LoadText(IDS_SPEED),
        CCommon::LoadText(IDS_ADAPTER_PHYSICAL_ADDRESS),
        CCommon::LoadText(IDS_IP_ADDRESS),
        CCommon::LoadText(IDS_SUBNET_MASK),
        CCommon::LoadText(IDS_DEFAULT_GATEWAY),
        CCommon::LoadText(IDS_OPERATIONAL_STATUS),
        CCommon::LoadText(IDS_BYTES_RECEIVED),
        CCommon::LoadText(IDS_BYTES_SENT),
        CCommon::LoadText(IDS_BYTES_RECEIVED_SINCE_START),
        CCommon::LoadText(IDS_BYTES_SENT_SINCE_START),
        CCommon::LoadText(IDS_PROGRAM_ELAPSED_TIME),
        CCommon::LoadText(IDS_INTERNET_IP_ADDRESS, _T(" (ipv4)")),
        CCommon::LoadText(IDS_INTERNET_IP_ADDRESS, _T(" (ipv6)")),
    };
    for (const auto& row_name : row_names)
    {
        NativeMaterial::TableCell name_cell;
        name_cell.text = row_name;
        m_info_list->AddRow({ name_cell, NativeMaterial::TableCell() });
    }
    //if (theApp.m_cfg_data.m_show_internet_ip)
    //{
    //	SetInfoText(14, CCommon::LoadText(IDS_ACQUIRING, _T("...")));
    //	SetInfoText(15, CCommon::LoadText(IDS_ACQUIRING, _T("...")));
    //}
    //else
    //{
    SetInfoText(14, CCommon::LoadText(IDS_DOUBLE_CLICK_TO_ACQUIRE));
    SetInfoText(15, CCommon::LoadText(IDS_DOUBLE_CLICK_TO_ACQUIRE));
    //}

    //显示列表中的信息
    LOGFONT lf{};
    GetFont()->GetLogFont(&lf);
    lf.lfWeight = FW_BOLD;
    m_font_bold.CreateFontIndirect(&lf);
    ShowInfo();
    GetProgramElapsedTime();

    //CCommon::GetInternetIp();
    //if (theApp.m_cfg_data.m_show_internet_ip)
 //       m_pGetIPThread = AfxBeginThread(GetInternetIPThreadFunc, this);		//启动获取外网IP的线程

    CCommon::LoadMenuResource(m_menu, IDR_INFO_MENU); //装载右键菜单

    FitMaterialContent(640);
    FinishMaterialShell();

    SetTimer(CONNECTION_DETAIL_TIMER, 1000, NULL);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}


void CNetworkInfoDlg::OnCopyText()
{
    // TODO: 在此添加命令处理程序代码
    if (!CCommon::CopyStringToClipboard(wstring(m_selected_string)))
        NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_COPY_TO_CLIPBOARD_FAILED), MB_ICONWARNING);
}


void CNetworkInfoDlg::OnContextMenu(CWnd* pWnd, CPoint point)
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


void CNetworkInfoDlg::OnClose()
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    //对话框关闭时强制结束获取IP地址的线程
    //if(theApp.m_cfg_data.m_show_internet_ip)
 //       TerminateThread(m_pGetIPThread->m_hThread, 0);
    CBaseDialog::OnClose();
}


void CNetworkInfoDlg::OnBnClickedPreviousButton()
{
    // TODO: 在此添加控件通知处理程序代码
    if (m_connections.size() > 1 && m_connection_selected > 0)
    {
        m_connection_selected--;
        ShowInfo();
    }
}


void CNetworkInfoDlg::OnBnClickedNextButton()
{
    // TODO: 在此添加控件通知处理程序代码
    if (m_connections.size() > 1 && m_connection_selected < m_connections.size() - 1)
    {
        m_connection_selected++;
        ShowInfo();
    }
}


BOOL CNetworkInfoDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: 在此添加专用代码和/或调用基类
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



void CNetworkInfoDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (nIDEvent == CONNECTION_DETAIL_TIMER)
    {
        GetProgramElapsedTime();
    }

    CBaseDialog::OnTimer(nIDEvent);
}

BOOL CNetworkInfoDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
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


void CNetworkInfoDlg::OnTableActivated()
{
    // TODO: 在此添加控件通知处理程序代码
    const int row = m_info_list != nullptr ? m_info_list->ActivatedRow() : -1;
    if (/*!theApp.m_cfg_data.m_show_internet_ip && */!m_ip_acquired && (row == 14 || row == 15))		//双击了IP地址一行时
    {
        SetInfoText(14, CCommon::LoadText(IDS_ACQUIRING, _T("...")));
        SetInfoText(15, CCommon::LoadText(IDS_ACQUIRING, _T("...")));
        m_pGetIPThread = AfxBeginThread(GetInternetIPThreadFunc, this);
    }
}
