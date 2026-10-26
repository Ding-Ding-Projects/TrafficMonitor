// CAutoAdaptSettingsDlg.cpp: implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "CAutoAdaptSettingsDlg.h"
#include "afxdialogex.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/InputControls.h"


// CAutoAdaptSettingsDlg dialog

IMPLEMENT_DYNAMIC(CAutoAdaptSettingsDlg, CBaseDialog)

CAutoAdaptSettingsDlg::CAutoAdaptSettingsDlg(TaskBarSettingData& data, CWnd* pParent /*=nullptr*/)
	: CBaseDialog(IDD_ATUO_ADAPT_SETTING_DIALOG, pParent), m_data(data)
{
	EnableMaterialShell();
}

CAutoAdaptSettingsDlg::~CAutoAdaptSettingsDlg()
{
}

void CAutoAdaptSettingsDlg::InitComboBox(NativeMaterial::Dropdown& combo_box, int style_sel)
{
	for (int i = 0; i < TASKBAR_DEFAULT_STYLE_NUM; i++)
		combo_box.AddItem(CCommon::LoadText(IDS_PRESET, std::to_wstring(i + 1).c_str()));
	if (style_sel >= 0 && style_sel < TASKBAR_DEFAULT_STYLE_NUM)
		combo_box.SetCurSel(style_sel);
	else
		combo_box.SetCurSel(0);
}

int CAutoAdaptSettingsDlg::GetComboBoxSel(const NativeMaterial::Dropdown& combo_box)
{
	int sel = combo_box.GetCurSel();
	if (sel >= 0 && sel < TASKBAR_DEFAULT_STYLE_NUM)
		return sel;
	else
		return 0;
}

void CAutoAdaptSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
}

CString CAutoAdaptSettingsDlg::GetDialogName() const
{
    return _T("AutoAdaptSettingsDlg");
}

int CAutoAdaptSettingsDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
	// Two labelled dropdowns, the check box with its explanation, then the action row.
	const int margin = DPI(20), gap = DPI(12), button_height = DPI(40);
	const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
	int y = content.top + margin;
	for (auto* dropdown : { m_dark_mode_default_style_combo, m_light_mode_default_style_combo })
	{
		if (dropdown == nullptr)
			continue;
		const int height = dropdown->MinimumSize().cy;
		if (apply)
			dropdown->MoveWindow(content.left + margin, y, width, height);
		y += height + gap;
	}
	if (m_auto_save_check != nullptr)
	{
		const int height = (std::max)(static_cast<int>(m_auto_save_check->MinimumSize().cy), DPI(40));
		if (apply)
			m_auto_save_check->MoveWindow(content.left + margin, y, width, height);
		y += height;
	}
	if (m_auto_save_tip != nullptr)
	{
		const int indent = DPI(8);
		const int height = m_auto_save_tip->MeasureHeight(width - indent);
		if (apply)
			m_auto_save_tip->MoveWindow(content.left + margin + indent, y, width - indent, height);
		y += height;
	}
	y += gap * 2;
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


BEGIN_MESSAGE_MAP(CAutoAdaptSettingsDlg, CBaseDialog)
END_MESSAGE_MAP()


// CAutoAdaptSettingsDlg message handlers


BOOL CAutoAdaptSettingsDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	// Material components replace the stock controls; the static captions become the dropdown labels.
	CString dark_caption, light_caption;
	GetDlgItemText(IDC_DARK_MODE_STATIC, dark_caption);
	GetDlgItemText(IDC_LIGHT_MODE_STATIC, light_caption);
	RemoveStockControl(IDC_DARK_MODE_STATIC);
	RemoveStockControl(IDC_LIGHT_MODE_STATIC);
	m_dark_mode_default_style_combo = ReplaceWithDropdown(IDC_DARK_MODE_DEFAULT_STYLE_COMBO, dark_caption);
	m_light_mode_default_style_combo = ReplaceWithDropdown(IDC_LIGHT_MODE_DEFAULT_STYLE_COMBO, light_caption);
	m_auto_save_check = ReplaceWithCheckbox(IDC_AUTO_SAVE_TO_PRESET_CHECK);
	m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);
	m_cancel = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);

	// The explanation used to be a stock tool tip on the check box; it is now always visible text.
	m_auto_save_tip = AddMaterialLabel(CCommon::LoadText(IDS_AUTO_SAVE_TO_PRESET_TIP), NativeMaterial::LabelRole::Secondary);

	InitComboBox(*m_dark_mode_default_style_combo, m_data.dark_default_style);
	InitComboBox(*m_light_mode_default_style_combo, m_data.light_default_style);
	SetDlgCheck(IDC_AUTO_SAVE_TO_PRESET_CHECK, m_data.auto_save_taskbar_color_settings_to_preset);

	FitMaterialContent(380);
	FinishMaterialShell();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // exception: OCX property pages should return FALSE
}


void CAutoAdaptSettingsDlg::OnOK()
{
	// Read the settings back from the controls
	m_data.dark_default_style = GetComboBoxSel(*m_dark_mode_default_style_combo);
	m_data.light_default_style = GetComboBoxSel(*m_light_mode_default_style_combo);
	m_data.auto_save_taskbar_color_settings_to_preset = GetDlgCheck(IDC_AUTO_SAVE_TO_PRESET_CHECK);

	CBaseDialog::OnOK();
}
