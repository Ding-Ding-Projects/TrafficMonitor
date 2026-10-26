// SkinDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "SkinDlg.h"
#include "afxdialogex.h"
#include "SkinAutoAdaptSettingDlg.h"
#include "SkinManager.h"
#include "LegacyMaterialTables.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/InputControls.h"


// CSkinDlg 对话框

IMPLEMENT_DYNAMIC(CSkinDlg, CBaseDialog)

CSkinDlg::CSkinDlg(CWnd* pParent /*=NULL*/)
    : CBaseDialog(IDD_SKIN_DIALOG, pParent)
{
    EnableMaterialShell();
}

CSkinDlg::~CSkinDlg()
{
}

CString CSkinDlg::GetDialogName() const
{
    return _T("SkinDlg");
}

void CSkinDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}


void CSkinDlg::ShowPreview()
{
    //载入布局数据
    m_skin_data.Load(CSkinManager::Instance().GetSkinName(m_skin_selected));
    //获取当前皮肤中用户更改字体
    SkinSettingData cur_skin_data;
    if (CSkinManager::Instance().GetSkinSettingDataByIndex(m_skin_selected, cur_skin_data))
    {
        m_skin_data.SetSettingData(cur_skin_data);
    }
    //获取预览区大小
    m_view.SetSize(m_skin_data.GetPreviewInfo().width, m_skin_data.GetPreviewInfo().height);
    //刷新预览图
    m_view.Invalidate();

    //显示皮肤作者
    if (m_skin_info != nullptr)
    {
        m_skin_info->SetWindowText(CCommon::LoadText(IDS_SKIN_AUTHOUR, m_skin_data.GetSkinInfo().skin_author.c_str()));
        m_skin_info->Invalidate(FALSE);
    }
    ////设置提示信息
    //bool cover_font_setting{ !m_skin_data.GetSkinInfo().font_info.name.IsEmpty() || (m_skin_data.GetSkinInfo().font_info.size >= MIN_FONT_SIZE && m_skin_data.GetSkinInfo().font_info.size <= MAX_FONT_SIZE) };
    //bool cover_str_setting{ !m_skin_data.GetSkinInfo().display_text.IsInvalid() };
    //cover_font_setting = cover_font_setting && theApp.m_general_data.allow_skin_cover_font;
    //cover_str_setting = cover_str_setting && theApp.m_general_data.allow_skin_cover_text;
    //if (cover_font_setting && cover_str_setting)
    //    m_notify_static.SetWindowTextEx(CCommon::LoadText(IDS_OVERWRITE_FONT_TEXT_WARNING));
    //else if (cover_font_setting)
    //    m_notify_static.SetWindowTextEx(CCommon::LoadText(IDS_OVERWRITE_FONT_WARNING));
    //else if (cover_str_setting)
    //    m_notify_static.SetWindowTextEx(CCommon::LoadText(IDS_OVERWRITE_TEXT_WARNING));
    //else
    //m_notify_static.SetWindowTextEx(_T(""));
}

int CSkinDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    // Two columns: the skin list on the left; heading, preview and author on the right. Below them the
    // auto adapt row, the link row and the action row.
    const int margin = DPI(16), gap = DPI(8), column_gap = DPI(16), button_height = DPI(40), link_height = DPI(36);
    const int link_inset = DPI(12), link_gap = DPI(4);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    const int left = content.left + margin;
    const int list_width = (std::max)(DPI(96), (std::min)(DPI(200), width / 3));
    const int right_left = left + list_width + column_gap;
    const int right_width = (std::max)(1, width - list_width - column_gap);

    int check_height = button_height;
    if (m_auto_adapt_check != nullptr)
        check_height = (std::max)(static_cast<int>(m_auto_adapt_check->MinimumSize().cy), button_height);
    const int bottom_height = check_height + gap + link_height + gap + button_height;

    const int minimum_area = DPI(260);
    int area_height = minimum_area;
    if (apply)
        area_height = (std::max)(minimum_area, static_cast<int>(content.Height()) - (margin + gap + bottom_height + margin));
    const int area_top = content.top + margin;

    // Left column.
    {
        int y = area_top;
        if (m_select_caption != nullptr)
        {
            const int height = m_select_caption->MeasureHeight(list_width);
            if (apply)
                m_select_caption->MoveWindow(left, y, list_width, height);
            y += height + gap;
        }
        if (m_skin_list_box != nullptr && apply)
            m_skin_list_box->MoveWindow(left, y, list_width, (std::max)(DPI(96), area_top + area_height - y));
    }

    // Right column.
    {
        int y = area_top;
        if (m_preview_heading != nullptr)
        {
            const int height = m_preview_heading->MeasureHeight(right_width);
            if (apply)
                m_preview_heading->MoveWindow(right_left, y, right_width, height);
            y += height + gap;
        }
        int author_height = 0, notify_height = 0;
        if (m_skin_info != nullptr)
            author_height = m_skin_info->MeasureHeight(right_width);
        if (m_notify != nullptr)
        {
            CString notice;
            m_notify->GetWindowText(notice);
            notify_height = notice.IsEmpty() ? 0 : m_notify->MeasureHeight(right_width);
        }
        const int tail = (author_height > 0 ? author_height + gap : 0) + (notify_height > 0 ? notify_height + gap : 0);
        const int view_height = (std::max)(DPI(96), area_top + area_height - y - tail);
        if (apply && m_view.GetSafeHwnd() != nullptr)
            m_view.MoveWindow(right_left, y, right_width, view_height);
        y += view_height + gap;
        if (m_skin_info != nullptr)
        {
            if (apply)
                m_skin_info->MoveWindow(right_left, y, right_width, author_height);
            y += author_height + gap;
        }
        if (m_notify != nullptr && apply)
            m_notify->MoveWindow(right_left, y, right_width, notify_height);
    }

    int y = area_top + area_height + gap;

    // Auto adapt: the check box takes the free width, the settings button sits at its right edge.
    {
        int button_width = 0;
        if (m_auto_adapt_button != nullptr)
        {
            CString label;
            m_auto_adapt_button->GetWindowText(label);
            button_width = MeasureMaterialButtonWidth(label, NativeMaterial::ButtonVariant::Tonal, 88);
            if (apply)
                m_auto_adapt_button->MoveWindow(left + width - button_width, y + (check_height - button_height) / 2, button_width, button_height);
        }
        if (m_auto_adapt_check != nullptr && apply)
            m_auto_adapt_check->MoveWindow(left, y, (std::max)(1, width - button_width - (button_width > 0 ? gap : 0)), check_height);
        y += check_height + gap;
    }

    // Links: text buttons whose label lines up with the content edge.
    {
        int x = left - link_inset;
        for (auto* link : { m_skin_download, m_skin_course, m_open_skin_dir_lnk })
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

    // Actions on the right.
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
    }
    y += button_height;
    return y + margin - content.top;
}

void CSkinDlg::OnMaterialAppearanceChanged()
{
    if (m_skin_list_box != nullptr && m_skin_list_box->GetSafeHwnd() != nullptr)
    {
        m_skin_list_box->SetAppearance(MaterialAppearance());
        m_skin_list_box->SetSurfaceColor(MaterialSurfaceColor());
    }
    if (m_view.GetSafeHwnd() != nullptr)
        m_view.SetAppearance(MaterialAppearance(), MaterialSurfaceColor());
}


BEGIN_MESSAGE_MAP(CSkinDlg, CBaseDialog)
    ON_CONTROL(NativeMaterial::TableSelectionChanged, IDC_LIST1, &CSkinDlg::OnTableSelectionChanged)
    ON_BN_CLICKED(IDC_SKIN_AUTO_ADAPT_BUTTON, &CSkinDlg::OnBnClickedSkinAutoAdaptButton)
    ON_BN_CLICKED(IDC_SKIN_AUTO_ADAPT_CHECK, &CSkinDlg::OnBnClickedSkinAutoAdaptCheck)
END_MESSAGE_MAP()


// CSkinDlg 消息处理程序

BOOL CSkinDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetIcon(theApp.GetMenuIcon(IDI_SKIN), FALSE);		// 设置小图标

    // Material components replace the stock controls: the list box becomes a single-select list, the
    // group box a section heading, the link statics text buttons, and the preview view scrolls with rails.
    m_select_caption = ReplaceWithLabel(IDC_STATIC, NativeMaterial::LabelRole::Secondary);
    CString caption;
    if (m_select_caption != nullptr)
        m_select_caption->GetWindowText(caption);
    m_skin_list_box = LegacyMaterial::ReplaceWithTable<NativeMaterial::MaterialList>(*this, IDC_LIST1, MaterialAppearance(), MaterialSurfaceColor(), caption);
    m_preview_heading = ReplaceGroupBoxWithHeading(GetDlgItem(IDC_PREVIEW_GROUP_STATIC));
    m_skin_info = ReplaceWithLabel(IDC_SKIN_INFO, NativeMaterial::LabelRole::Secondary);
    m_notify = ReplaceWithLabel(IDC_NOTIFY_STATIC, NativeMaterial::LabelRole::Error);
    m_auto_adapt_check = ReplaceWithCheckbox(IDC_SKIN_AUTO_ADAPT_CHECK);
    m_auto_adapt_button = ReplaceWithButton(IDC_SKIN_AUTO_ADAPT_BUTTON, NativeMaterial::ButtonVariant::Tonal);
    m_skin_course = ReplaceWithButton(IDC_SKIN_COURSE_STATIC, NativeMaterial::ButtonVariant::Text);
    m_skin_download = ReplaceWithButton(IDC_SKIN_DOWNLOAD_STATIC, NativeMaterial::ButtonVariant::Text);
    m_open_skin_dir_lnk = ReplaceWithButton(IDC_OPEN_SKIN_DIR_STATIC, NativeMaterial::ButtonVariant::Text);
    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);
    m_cancel = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);

    //初始化选择框
    for (const auto& skin_name : CSkinManager::Instance().GetSkinNames())
    {
        m_skin_list_box->AddItem(skin_name.c_str());
    }
    m_skin_list_box->SetCurSel(m_skin_selected);
    //初始化预览视图
    m_view.Create(this, CRect(0, 0, 100, 100), 3000);
    m_view.SetSkinData(&m_skin_data);
    m_view.SetAppearance(MaterialAppearance(), MaterialSurfaceColor());

    //设置提示信息
    if (m_notify != nullptr)
        m_notify->SetWindowText(_T(""));

    //显示预览图片
    ShowPreview();

    //初始化控件
    SetDlgCheck(IDC_SKIN_AUTO_ADAPT_CHECK, theApp.m_cfg_data.skin_auto_adapt);
    EnableDlgCtrl(IDC_SKIN_AUTO_ADAPT_BUTTON, theApp.m_cfg_data.skin_auto_adapt);

    //设置超链接：按钮点击时在OnCommand中打开
    m_urls[IDC_SKIN_COURSE_STATIC] = _T("https://github.com/zhongyang219/TrafficMonitor/wiki/%E7%9A%AE%E8%82%A4%E5%88%B6%E4%BD%9C%E6%95%99%E7%A8%8B");
    if (theApp.m_general_data.update_source == 1)
        m_urls[IDC_SKIN_DOWNLOAD_STATIC] = _T("https://gitee.com/zhongyang219/TrafficMonitorSkin/blob/master/%E7%9A%AE%E8%82%A4%E4%B8%8B%E8%BD%BD.md");
    else
        m_urls[IDC_SKIN_DOWNLOAD_STATIC] = _T("https://github.com/zhongyang219/TrafficMonitorSkin/blob/master/皮肤下载.md");

    FitMaterialContent(640);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}


void CSkinDlg::OnTableSelectionChanged()
{
    const int selected = m_skin_list_box != nullptr ? m_skin_list_box->GetCurSel() : -1;
    if (selected < 0)
    {
        // A list always keeps one skin selected: put the selection mark back on the current one.
        if (m_skin_list_box != nullptr)
            m_skin_list_box->SetCurSel(m_skin_selected);
        return;
    }
    m_skin_selected = selected;
    ShowPreview();
    RelayoutMaterial();     // the author text can change the height the preview column needs
}


BOOL CSkinDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (HIWORD(wParam) == BN_CLICKED)
    {
        const UINT id = LOWORD(wParam);
        const auto url = m_urls.find(id);
        if (url != m_urls.end() && !url->second.IsEmpty())
        {
            ShellExecute(NULL, _T("open"), url->second, NULL, NULL, SW_SHOW);
            return TRUE;
        }
        //点击了“打开皮肤目录”
        if (id == IDC_OPEN_SKIN_DIR_STATIC)
        {
            CreateDirectory(theApp.m_skin_path.c_str(), NULL);       //如果皮肤目录不存在，则创建它
            ShellExecute(NULL, _T("open"), _T("explorer"), theApp.m_skin_path.c_str(), NULL, SW_SHOWNORMAL);
            return TRUE;
        }
    }
    return CBaseDialog::OnCommand(wParam, lParam);
}


void CSkinDlg::OnBnClickedSkinAutoAdaptButton()
{
    CSkinAutoAdaptSettingDlg dlg;
    const auto& skins{ CSkinManager::Instance().GetSkinNames() };
    if (dlg.DoModal() == IDOK)
    {
        int dark_mode_skin = dlg.GetDarkModeSkin();
        int light_mode_skin = dlg.GetLightModeSkin();
        if (dark_mode_skin >= 0 && dark_mode_skin < static_cast<int>(skins.size()))
            theApp.m_cfg_data.skin_name_dark_mode = skins[dark_mode_skin];
        if (light_mode_skin >= 0 && light_mode_skin < static_cast<int>(skins.size()))
            theApp.m_cfg_data.skin_name_light_mode = skins[light_mode_skin];
    }
}


void CSkinDlg::OnOK()
{
    theApp.m_cfg_data.skin_auto_adapt = GetDlgCheck(IDC_SKIN_AUTO_ADAPT_CHECK);

    CBaseDialog::OnOK();
}


void CSkinDlg::OnBnClickedSkinAutoAdaptCheck()
{
    bool skin_auto_adapt_checked = GetDlgCheck(IDC_SKIN_AUTO_ADAPT_CHECK);
    EnableDlgCtrl(IDC_SKIN_AUTO_ADAPT_BUTTON, skin_auto_adapt_checked);
}
