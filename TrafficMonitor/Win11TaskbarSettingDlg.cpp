// Win11TaskbarSettingDlg.cpp: implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "afxdialogex.h"
#include "Win11TaskbarSettingDlg.h"
#include "TaskBarDlg.h"
#include "WindowsSettingHelper.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/InputControls.h"


// CWin11TaskbarSettingDlg dialog

IMPLEMENT_DYNAMIC(CWin11TaskbarSettingDlg, CBaseDialog)

CWin11TaskbarSettingDlg::CWin11TaskbarSettingDlg(TaskBarSettingData& data, CWnd* pParent /*=nullptr*/)
	: CBaseDialog(IDD_WIN11_TASKBAR_SETTING_DLG, pParent)
    , m_data(data)
{
    EnableMaterialShell();
}

CWin11TaskbarSettingDlg::~CWin11TaskbarSettingDlg()
{
}

void CWin11TaskbarSettingDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

CString CWin11TaskbarSettingDlg::GetDialogName() const
{
    return _T("Win11TaskbarSettingDlg");
}

int CWin11TaskbarSettingDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    const int margin = DPI(20), gap = DPI(12), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    const int left = content.left + margin;
    int y = content.top + margin;
    auto place = [&](CWnd* control, int x, int w, int h)
    {
        if (apply && control != nullptr)
            control->MoveWindow(x, y, w, h);
    };

    if (m_snap_check != nullptr)
    {
        const int height = (std::max)(static_cast<int>(m_snap_check->MinimumSize().cy), DPI(40));
        place(m_snap_check, left, width, height);
        y += height;
    }
    y += gap;

    // Vertical and horizontal offset side by side when both fit at their minimum width, otherwise stacked.
    if (m_window_offset_top_edit != nullptr && m_window_offset_left_edit != nullptr)
    {
        const int field_height = m_window_offset_top_edit->MinimumSize().cy;
        const int column = (width - gap) / 2;
        if (column >= m_window_offset_top_edit->MinimumSize().cx)
        {
            place(m_window_offset_top_edit, left, column, field_height);
            if (apply)
                m_window_offset_left_edit->MoveWindow(left + column + gap, y, width - column - gap, field_height);
            y += field_height + gap;
        }
        else
        {
            place(m_window_offset_top_edit, left, width, field_height);
            y += field_height + gap;
            place(m_window_offset_left_edit, left, width, field_height);
            y += field_height + gap;
        }
    }

    if (m_avoid_overlap_check != nullptr)
    {
        const int height = (std::max)(static_cast<int>(m_avoid_overlap_check->MinimumSize().cy), DPI(40));
        place(m_avoid_overlap_check, left, width, height);
        y += height + gap;
    }
    if (m_widgets_width_edit != nullptr)
    {
        const int height = m_widgets_width_edit->MinimumSize().cy;
        place(m_widgets_width_edit, left, (std::min)(width, (std::max)(static_cast<int>(m_widgets_width_edit->MinimumSize().cx), width / 2)), height);
        y += height + gap;
    }
    y += gap;

    // Action row: restore defaults on the left, OK and Cancel on the right.
    if (m_ok != nullptr && m_cancel != nullptr && m_restore_default != nullptr)
    {
        CString ok, cancel, restore;
        m_ok->GetWindowText(ok);
        m_cancel->GetWindowText(cancel);
        m_restore_default->GetWindowText(restore);
        const int cancel_width = MeasureMaterialButtonWidth(cancel, NativeMaterial::ButtonVariant::Text, 88);
        const int ok_width = MeasureMaterialButtonWidth(ok, NativeMaterial::ButtonVariant::Primary, 88);
        const int restore_width = MeasureMaterialButtonWidth(restore, NativeMaterial::ButtonVariant::Text, 88);
        const int right = left + width;
        place(m_restore_default, left, restore_width, button_height);
        if (apply)
        {
            m_cancel->MoveWindow(right - cancel_width, y, cancel_width, button_height);
            m_ok->MoveWindow(right - cancel_width - gap - ok_width, y, ok_width, button_height);
        }
        y += button_height;
    }
    return y + margin - content.top;
}

void CWin11TaskbarSettingDlg::EnableDlgCtrl(UINT id, bool enable)
{
    CWnd* pCtrl = GetDlgItem(id);
    if (pCtrl != nullptr)
        pCtrl->EnableWindow(enable);
}


BEGIN_MESSAGE_MAP(CWin11TaskbarSettingDlg, CBaseDialog)
    ON_BN_CLICKED(IDC_RESTORE_DEFAULT_BUTTON, &CWin11TaskbarSettingDlg::OnBnClickedRestoreDefaultButton)
END_MESSAGE_MAP()


// CWin11TaskbarSettingDlg message handlers


BOOL CWin11TaskbarSettingDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();
    SetIcon(theApp.GetMenuIcon(IDI_TASKBAR_WINDOW), FALSE);

    // Each number stepper takes the label of its caption static and the unit text of its "pixels" static.
    auto fold = [this](UINT edit_id, UINT caption_id, UINT unit_id)
    {
        CString caption, unit;
        GetDlgItemText(caption_id, caption);
        GetDlgItemText(unit_id, unit);
        RemoveStockControl(caption_id);
        RemoveStockControl(unit_id);
        return ReplaceWithNumberStepper(edit_id, caption, unit);
    };
    m_window_offset_top_edit = fold(IDC_WINDOW_OFFSET_TOP_EDIT, IDC_VERTICAL_OFFSET_STATIC, IDC_PIXEL_STATIC);
    m_window_offset_left_edit = fold(IDC_WINDOW_OFFSET_LEFT_EDIT, IDC_HORIZONTAL_OFFSET_STATIC, IDC_PIXEL_STATIC1);
    m_widgets_width_edit = fold(IDC_WIDTET_WIDTH_EDIT, IDC_WIDGET_WIDTH_STATIC, IDC_PIXEL_STATIC2);
    m_snap_check = ReplaceWithCheckbox(IDC_TASKBAR_WND_SNAP_CHECK);
    m_avoid_overlap_check = ReplaceWithCheckbox(IDC_AVOID_OVERLAP_RIGHT_WIDGETS_CHECK);
    m_restore_default = ReplaceWithButton(IDC_RESTORE_DEFAULT_BUTTON, NativeMaterial::ButtonVariant::Text);
    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);
    m_cancel = ReplaceWithButton(IDCANCEL, NativeMaterial::ButtonVariant::Text);

    EnableDlgCtrl(IDC_TASKBAR_WND_SNAP_CHECK, CTaskBarDlg::IsTaskbarCloseToIconEnable(m_data.tbar_wnd_on_left));
    SetDlgCheck(IDC_TASKBAR_WND_SNAP_CHECK, m_data.tbar_wnd_snap);
    m_window_offset_top_edit->SetRange(-20, 20);
    m_window_offset_top_edit->SetValue(m_data.window_offset_top);
    m_window_offset_left_edit->SetRange(-800, 800);
    m_window_offset_left_edit->SetValue(m_data.window_offset_left);
    SetDlgCheck(IDC_AVOID_OVERLAP_RIGHT_WIDGETS_CHECK, m_data.avoid_overlap_with_widgets);
    //EnableDlgCtrl(IDC_AVOID_OVERLAP_RIGHT_WIDGETS_CHECK, CWindowsSettingHelper::IsTaskbarWidgetsBtnShown());
    m_widgets_width_edit->SetRange(0, 300);
    m_widgets_width_edit->SetValue(m_data.taskbar_left_space_win11);
    //m_widgets_width_edit->EnableWindow(CWindowsSettingHelper::IsTaskbarWidgetsBtnShown());

    FitMaterialContent(400);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
    // exception: OCX property pages should return FALSE
}


void CWin11TaskbarSettingDlg::OnOK()
{
    // Apply text that was typed but not yet committed, exactly as pressing Enter in the field would.
    m_window_offset_top_edit->CommitText();
    m_window_offset_left_edit->CommitText();
    m_widgets_width_edit->CommitText();

    m_data.tbar_wnd_snap = GetDlgCheck(IDC_TASKBAR_WND_SNAP_CHECK);

    m_data.window_offset_top = m_window_offset_top_edit->GetValue();
    m_data.ValidWindowOffsetTop();
    m_data.window_offset_left = m_window_offset_left_edit->GetValue();
    m_data.ValidWindowOffsetLeft();

    m_data.avoid_overlap_with_widgets = GetDlgCheck(IDC_AVOID_OVERLAP_RIGHT_WIDGETS_CHECK);

    m_data.taskbar_left_space_win11 = m_widgets_width_edit->GetValue();
    if (m_data.taskbar_left_space_win11 < 0)
        m_data.taskbar_left_space_win11 = 0;
    if (m_data.taskbar_left_space_win11 > 300)
        m_data.taskbar_left_space_win11 = 300;

    CBaseDialog::OnOK();
}


void CWin11TaskbarSettingDlg::OnBnClickedRestoreDefaultButton()
{
    m_window_offset_top_edit->SetValue(0);
    m_window_offset_left_edit->SetValue(0);
    m_widgets_width_edit->SetValue(160);
}
