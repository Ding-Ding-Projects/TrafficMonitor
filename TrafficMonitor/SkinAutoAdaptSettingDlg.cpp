// SkinAutoAdaptSettingDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "afxdialogex.h"
#include "SkinAutoAdaptSettingDlg.h"
#include "SkinManager.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/InputControls.h"


// CSkinAutoAdaptSettingDlg 对话框

IMPLEMENT_DYNAMIC(CSkinAutoAdaptSettingDlg, CBaseDialog)

CSkinAutoAdaptSettingDlg::CSkinAutoAdaptSettingDlg(CWnd* pParent /*=nullptr*/)
	: CBaseDialog(IDD_SKIN_AUTO_ADAPT_DLG, pParent)
{
    EnableMaterialShell();
}

CSkinAutoAdaptSettingDlg::~CSkinAutoAdaptSettingDlg()
{
}

void CSkinAutoAdaptSettingDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

CString CSkinAutoAdaptSettingDlg::GetDialogName() const
{
    return _T("SkinAutoAdaptSettingDlg");
}

int CSkinAutoAdaptSettingDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    // Two labelled dropdowns, then the action row.
    const int margin = DPI(20), gap = DPI(12), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    int y = content.top + margin;
    for (auto* dropdown : { m_dark_mode_skin_combo, m_light_mode_skin_combo })
    {
        if (dropdown == nullptr)
            continue;
        const int height = dropdown->MinimumSize().cy;
        if (apply)
            dropdown->MoveWindow(content.left + margin, y, width, height);
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
            const int right = content.left + margin + width;
            m_cancel->MoveWindow(right - cancel_width, y, cancel_width, button_height);
            m_ok->MoveWindow(right - cancel_width - gap - ok_width, y, ok_width, button_height);
        }
        y += button_height;
    }
    return y + margin - content.top;
}


BEGIN_MESSAGE_MAP(CSkinAutoAdaptSettingDlg, CBaseDialog)
END_MESSAGE_MAP()


// CSkinAutoAdaptSettingDlg 消息处理程序


BOOL CSkinAutoAdaptSettingDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    // Material components replace the stock combo boxes; the static captions become the dropdown labels.
    CString dark_caption, light_caption;
    GetDlgItemText(IDC_DARK_MODE_STATIC, dark_caption);
    GetDlgItemText(IDC_LIGHT_MODE_STATIC, light_caption);
    RemoveStockControl(IDC_DARK_MODE_STATIC);
    RemoveStockControl(IDC_LIGHT_MODE_STATIC);
    m_dark_mode_skin_combo = ReplaceWithDropdown(IDC_DARK_MODE_SKIN_COMBO, dark_caption);
    m_light_mode_skin_combo = ReplaceWithDropdown(IDC_LIGHT_MODE_SKIN_COMBO, light_caption);
    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);
    m_cancel = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);

    //初始化下拉列表
    for (const auto& skin_name : CSkinManager::Instance().GetSkinNames())
    {
        m_dark_mode_skin_combo->AddItem(skin_name.c_str());
        m_light_mode_skin_combo->AddItem(skin_name.c_str());
    }

    int dark_mode_skin = CSkinManager::Instance().FindSkinIndex(theApp.m_cfg_data.skin_name_dark_mode);
    int light_mode_skin = CSkinManager::Instance().FindSkinIndex(theApp.m_cfg_data.skin_name_light_mode);
    m_dark_mode_skin_combo->SetCurSel(dark_mode_skin);
    m_light_mode_skin_combo->SetCurSel(light_mode_skin);

    FitMaterialContent(360);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常: OCX 属性页应返回 FALSE
}


void CSkinAutoAdaptSettingDlg::OnOK()
{
    m_dark_mode_skin = m_dark_mode_skin_combo->GetCurSel();
    m_light_mode_skin = m_light_mode_skin_combo->GetCurSel();

    CBaseDialog::OnOK();
}
