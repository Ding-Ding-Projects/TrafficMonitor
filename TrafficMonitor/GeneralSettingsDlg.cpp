// GeneralSettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NativeMaterial/AlertDialog.h"
#include "TrafficMonitor.h"
#include "TrafficMonitorDlg.h"
#include "GeneralSettingsDlg.h"
#include "PluginManagerDlg.h"
#include "SelectConnectionsDlg.h"
#include "NativePreferences/Text.h"


// CGeneralSettingsDlg dialog

static const int MONITOR_SPAN_STEP = 100;

namespace
{
    // A caption that is folded into the Material component it describes needs no trailing colon.
    CString TrimCaption(CString text)
    {
        text.Trim();
        while (!text.IsEmpty() && (text.Right(1) == L":" || text.Right(1) == L"："))
        {
            text = text.Left(text.GetLength() - 1);
            text.TrimRight();
        }
        return text;
    }

    // Index of the first item that starts with name (case-insensitive), the way the stock combo box searched; -1 when none.
    int FindDropdownItem(const NativeMaterial::Dropdown& dropdown, const CString& name)
    {
        for (int i = 0; i < dropdown.GetItemCount(); ++i)
        {
            const CString item = dropdown.GetItemText(i);
            if (item.GetLength() >= name.GetLength() && item.Left(name.GetLength()).CompareNoCase(name) == 0)
                return i;
        }
        return -1;
    }
}

IMPLEMENT_DYNAMIC(CGeneralSettingsDlg, CTabDlg)

CGeneralSettingsDlg::CGeneralSettingsDlg(CWnd* pParent /*=NULL*/)
    : CTabDlg(IDD_GENERAL_SETTINGS_DIALOG, pParent)
{
    EnableMaterialPage();
}

CGeneralSettingsDlg::~CGeneralSettingsDlg()
{
}

void CGeneralSettingsDlg::CheckTaskbarDisplayItem()
{
    //如果选项设置中关闭了某个硬件监控，则不显示对应的温度监控相关项目
    if (!theApp.m_general_data.IsHardwareEnable(HI_CPU))
    {
        theApp.m_taskbar_data.display_item.Remove(TDI_CPU_TEMP);
    }
    if (!theApp.m_general_data.IsHardwareEnable(HI_GPU))
    {
        theApp.m_taskbar_data.display_item.Remove(TDI_GPU_TEMP);
    }
    if (!theApp.m_general_data.IsHardwareEnable(HI_HDD))
    {
        theApp.m_taskbar_data.display_item.Remove(TDI_HDD_TEMP);
    }
    if (!theApp.m_general_data.IsHardwareEnable(HI_MBD))
        theApp.m_taskbar_data.display_item.Remove(TDI_MAIN_BOARD_TEMP);
}

void CGeneralSettingsDlg::OnSettingsApplied()
{
    //当设置被应用时，重置xxxx_ori的值
    m_monitor_time_span_ori = m_data.monitor_time_span;
    m_update_source_ori = m_data.update_source;
}

void CGeneralSettingsDlg::OnMaterialAppearanceChanged()
{
    CTabDlg::OnMaterialAppearanceChanged();
    m_tooltip.SetAppearance(MaterialAppearance());
}

int CGeneralSettingsDlg::LayoutPage(const CRect& area, bool apply)
{
    const int margin = DPI(16), gap = DPI(8), section_gap = DPI(20), row_height = DPI(40);
    const int left = area.left + margin;
    const int width = (std::max)(1, static_cast<int>(area.Width()) - margin * 2);
    int y = area.top + margin;

    auto place = [&](CWnd* control, int x, int top, int w, int h)
    {
        if (apply && control != nullptr)
            control->MoveWindow(x, top, w, h);
    };
    // Section heading.
    auto heading = [&](NativeMaterial::Label* label)
    {
        if (label == nullptr)
            return;
        const int height = (std::max)(DPI(32), label->MeasureHeight(width));
        place(label, left, y, width, height);
        y += height;
    };
    // Wrapping text above the controls it introduces.
    auto paragraph = [&](NativeMaterial::Label* label)
    {
        if (label == nullptr)
            return;
        const int height = label->MeasureHeight(width);
        place(label, left, y, width, height);
        y += height + gap;
    };
    // A check box takes a full row.
    auto toggle = [&](NativeMaterial::InputWindow* control)
    {
        if (control == nullptr)
            return;
        const int height = (std::max)(static_cast<int>(control->MinimumSize().cy), row_height);
        place(control, left, y, width, height);
        y += height;
    };
    // Radio buttons and check boxes side by side, wrapping to the next line when the row is full.
    auto flow = [&](std::initializer_list<NativeMaterial::InputWindow*> items)
    {
        int x = left, line_top = y, line_height = 0;
        for (auto* item : items)
        {
            if (item == nullptr)
                continue;
            const CSize minimum = item->MinimumSize();
            const int w = (std::min)(static_cast<int>(minimum.cx), width);
            const int h = (std::max)(static_cast<int>(minimum.cy), row_height);
            if (x > left && x + w > left + width)
            {
                x = left;
                line_top += line_height;
                line_height = 0;
            }
            place(item, x, line_top, w, h);
            x += w + gap;
            line_height = (std::max)(line_height, h);
        }
        y = line_top + line_height;
    };
    // Push buttons in a row that wraps.
    auto buttons = [&](std::initializer_list<std::pair<NativeMaterial::Button*, NativeMaterial::ButtonVariant>> items)
    {
        int x = left, line_top = y;
        bool any = false;
        for (const auto& item : items)
        {
            if (item.first == nullptr)
                continue;
            CString label;
            item.first->GetWindowText(label);
            const int w = (std::min)(MeasureMaterialButtonWidth(label, item.second, 64), width);
            if (any && x + w > left + width)
            {
                x = left;
                line_top += row_height + gap;
            }
            place(item.first, x, line_top, w, row_height);
            x += w + gap;
            any = true;
        }
        if (any)
            y = line_top + row_height + gap;
    };
    // A dropdown or number field below its caption row.
    auto field = [&](NativeMaterial::InputWindow* control)
    {
        if (control == nullptr)
            return;
        const int w = (std::min)(width, (std::max)(static_cast<int>(control->MinimumSize().cx), DPI(320)));
        const int height = control->MinimumSize().cy;
        place(control, left, y, w, height);
        y += height + gap;
    };
    // "Notify when ... has been reached": the check box and its threshold (and unit) on one line when the width allows,
    // otherwise the check box on its own line and the threshold indented below it.
    auto tip_row = [&](NativeMaterial::Checkbox* check, NativeMaterial::NumberStepper* stepper, NativeMaterial::Dropdown* unit)
    {
        if (check == nullptr || stepper == nullptr)
            return;
        const int check_width = check->MinimumSize().cx;
        const int check_height = (std::max)(static_cast<int>(check->MinimumSize().cy), row_height);
        const int stepper_width = (std::max)(static_cast<int>(stepper->MinimumSize().cx), DPI(168));
        const int stepper_height = stepper->MinimumSize().cy;
        const int unit_width = unit != nullptr ? (std::max)(static_cast<int>(unit->MinimumSize().cx), DPI(120)) : 0;
        const int unit_height = unit != nullptr ? static_cast<int>(unit->MinimumSize().cy) : 0;
        const int field_width = stepper_width + (unit != nullptr ? gap + unit_width : 0);
        const int height = (std::max)({ check_height, stepper_height, unit_height });
        if (check_width + gap * 2 + field_width <= width)
        {
            place(check, left, y + (height - check_height) / 2, width - field_width - gap, check_height);
            const int field_left = left + width - field_width;
            place(stepper, field_left, y + (height - stepper_height) / 2, stepper_width, stepper_height);
            if (unit != nullptr)
                place(unit, field_left + stepper_width + gap, y + (height - unit_height) / 2, unit_width, unit_height);
            y += height;
        }
        else
        {
            place(check, left, y, width, check_height);
            y += check_height;
            const int indent = (std::min)(DPI(32), (std::max)(0, width - field_width));
            place(stepper, left + indent, y, stepper_width, stepper_height);
            if (unit != nullptr)
                place(unit, left + indent + stepper_width + gap, y, unit_width, unit_height);
            y += (std::max)(stepper_height, unit_height);
        }
        y += gap;
    };

    // Application settings
    heading(m_heading_application);
    toggle(m_check_update_check);
    buttons({ { m_check_now_btn, NativeMaterial::ButtonVariant::Tonal } });
    paragraph(m_update_source_label);
    flow({ m_github_radio, m_gitee_radio });
    y += gap;
    field(m_language_combo);
    y += section_gap;

    // Configuration and data files
    heading(m_heading_config_files);
    flow({ m_save_to_appdata_radio, m_save_to_program_dir_radio });
    y += gap;
    buttons({ { m_open_config_path_btn, NativeMaterial::ButtonVariant::Tonal } });
    y += section_gap;

    // Display
    heading(m_heading_display);
    toggle(m_show_notify_icon_check);
    y += section_gap;

    // Auto run
    heading(m_heading_auto_run);
    toggle(m_auto_run_check);
    buttons({ { m_reset_auto_run_btn, NativeMaterial::ButtonVariant::Tonal } });
    paragraph(m_auto_run_method_label);
    flow({ m_auto_run_registry_radio, m_auto_run_task_schedule_radio });
    y += section_gap;

    // Notification message
    heading(m_heading_notification);
    tip_row(m_traffic_tip_check, m_traffic_tip_edit, m_traffic_tip_combo);
    tip_row(m_memory_tip_check, m_memory_tip_edit, nullptr);
    tip_row(m_cpu_temp_tip_check, m_cpu_temp_tip_edit, nullptr);
    tip_row(m_gpu_temp_tip_check, m_gpu_temp_tip_edit, nullptr);
    tip_row(m_hdd_temp_tip_check, m_hdd_temp_tip_edit, nullptr);
    tip_row(m_mbd_temp_tip_check, m_mbd_temp_tip_edit, nullptr);
    y += section_gap - gap;

    // Hardware monitoring
    heading(m_heading_hardware);
    flow({ m_cpu_check, m_gpu_check, m_hdd_check, m_mbd_check });
    y += gap;
    field(m_hard_disk_combo);
    field(m_select_cpu_combo);
    y += section_gap - gap;

    // Advanced
    heading(m_heading_advanced);
    toggle(m_show_all_connection_check);
    buttons({ { m_select_connection_btn, NativeMaterial::ButtonVariant::Tonal } });
    paragraph(m_cpu_method_label);
    flow({ m_use_cpu_time_radio, m_use_pdh_radio });
    y += gap;
    field(m_monitor_span_edit);
    buttons({ { m_restore_default_time_span_btn, NativeMaterial::ButtonVariant::Text },
              { m_plugin_manager_btn, NativeMaterial::ButtonVariant::Tonal } });

    return y + margin - area.top;
}

bool CGeneralSettingsDlg::ShowHardwareMonitorWarning()
{
    //如果已经有硬件监控项目被勾选了，则不再弹出提示
    if (m_data.hardware_monitor_item != 0)
        return true;

    // The operating system remembers an earlier "do not show this again" answer of this question under this key and
    // value; honor it, then ask with the Material alert dialog.
    if (::RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DontShowMeThisDialogAgain",
        L"{B8A281A7-76DF-4F0F-BF6A-1A394EF8BAD5}", RRF_RT_ANY, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
        return true;

    return NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_HARDWARE_MONITOR_WARNING), APP_NAME, MB_OKCANCEL | MB_ICONWARNING) == IDOK;
}

void CGeneralSettingsDlg::AddOrUpdateAutoRunTooltip(bool add)
{
    if (m_auto_run_check == nullptr)
        return;
    if (m_data.auto_run)
    {
        CString str_tool_tip;
        if (m_data.auto_run_by_task_scheduler)
            str_tool_tip = CCommon::LoadText(IDS_AUTO_RUN_METHOD_TASK_SCHEDULE);
        else
            str_tool_tip = CCommon::LoadText(IDS_AUTO_RUN_METHOD_REGESTRY);
        if (!m_auto_run_path.empty())
        {
            str_tool_tip += _T("\r\n");
            str_tool_tip += CCommon::LoadText(IDS_PATH, _T(": "));
            str_tool_tip += m_auto_run_path.c_str();
        }
        if (add)
            m_tooltip.AddTool(m_auto_run_check, str_tool_tip);
        else
            m_tooltip.UpdateTipText(str_tool_tip, m_auto_run_check);
    }
    else
    {
        if (add)
            m_tooltip.AddTool(m_auto_run_check, CString());
        else
            m_tooltip.UpdateTipText(CString(), m_auto_run_check);
    }
}

bool CGeneralSettingsDlg::IsMonitorTimeSpanModified() const
{
    return m_data.monitor_time_span != m_monitor_time_span_ori;
}

void CGeneralSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlg::DoDataExchange(pDX);
}

void CGeneralSettingsDlg::SetControlEnable()
{
    m_traffic_tip_edit->EnableWindow(m_data.traffic_tip_enable);
    m_traffic_tip_combo->EnableWindow(m_data.traffic_tip_enable);
    m_memory_tip_edit->EnableWindow(m_data.memory_usage_tip.enable);
    m_cpu_temp_tip_edit->EnableWindow(m_data.cpu_temp_tip.enable);
    m_gpu_temp_tip_edit->EnableWindow(m_data.gpu_temp_tip.enable);
    m_hdd_temp_tip_edit->EnableWindow(m_data.hdd_temp_tip.enable);
    m_mbd_temp_tip_edit->EnableWindow(m_data.mainboard_temp_tip.enable);

    //m_hard_disk_combo->EnableWindow(m_data.IsHardwareEnable(HI_HDD));
    m_select_cpu_combo->EnableWindow(m_data.IsHardwareEnable(HI_CPU));

    EnableDlgCtrl(IDC_SELECT_CONNECTIONS_BUTTON, !m_data.show_all_interface);

    EnableDlgCtrl(IDC_RESET_AUTO_RUN_BUTTON, m_data.auto_run);
    EnableDlgCtrl(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, m_data.auto_run);
    EnableDlgCtrl(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, m_data.auto_run);
}


BEGIN_MESSAGE_MAP(CGeneralSettingsDlg, CTabDlg)
    ON_BN_CLICKED(IDC_CHECK_NOW_BUTTON, &CGeneralSettingsDlg::OnBnClickedCheckNowButton)
    ON_BN_CLICKED(IDC_CHECK_UPDATE_CHECK, &CGeneralSettingsDlg::OnBnClickedCheckUpdateCheck)
    ON_BN_CLICKED(IDC_AUTO_RUN_CHECK, &CGeneralSettingsDlg::OnBnClickedAutoRunCheck)
    ON_BN_CLICKED(IDC_TODAY_TRAFFIC_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedTodayTrafficTipCheck)
    ON_BN_CLICKED(IDC_MEMORY_USAGE_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedMemoryUsageTipCheck)
    ON_BN_CLICKED(IDC_OPEN_CONFIG_PATH_BUTTON, &CGeneralSettingsDlg::OnBnClickedOpenConfigPathButton)
    ON_BN_CLICKED(IDC_SHOW_ALL_CONNECTION_CHECK, &CGeneralSettingsDlg::OnBnClickedShowAllConnectionCheck)
    ON_BN_CLICKED(IDC_USE_CPU_TIME_RADIO, &CGeneralSettingsDlg::OnBnClickedUseCpuTimeRadio)
    ON_BN_CLICKED(IDC_USE_PDH_RADIO, &CGeneralSettingsDlg::OnBnClickedUsePdhRadio)
    ON_BN_CLICKED(IDC_CPU_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedCpuTempTipCheck)
    ON_BN_CLICKED(IDC_GPU_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedGpuTempTipCheck)
    ON_BN_CLICKED(IDC_HDD_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedHddTempTipCheck)
    ON_BN_CLICKED(IDC_MBD_TEMP_TIP_CHECK, &CGeneralSettingsDlg::OnBnClickedMbdTempTipCheck)
    ON_BN_CLICKED(IDC_GITHUB_RADIO, &CGeneralSettingsDlg::OnBnClickedGithubRadio)
    ON_BN_CLICKED(IDC_GITEE_RADIO, &CGeneralSettingsDlg::OnBnClickedGiteeRadio)
    ON_BN_CLICKED(IDC_RESTORE_DEFAULT_TIME_SPAN_BUTTON, &CGeneralSettingsDlg::OnBnClickedRestoreDefaultTimeSpanButton)
    ON_CBN_SELCHANGE(IDC_SELECT_HARD_DISK_COMBO, &CGeneralSettingsDlg::OnCbnSelchangeSelectHardDiskCombo)
    ON_BN_CLICKED(IDC_CPU_CHECK, &CGeneralSettingsDlg::OnBnClickedCpuCheck)
    ON_BN_CLICKED(IDC_GPU_CHECK, &CGeneralSettingsDlg::OnBnClickedGpuCheck)
    ON_BN_CLICKED(IDC_HDD_CHECK, &CGeneralSettingsDlg::OnBnClickedHddCheck)
    ON_BN_CLICKED(IDC_MBD_CHECK, &CGeneralSettingsDlg::OnBnClickedMbdCheck)
    ON_CBN_SELCHANGE(IDC_SELECT_CPU_COMBO, &CGeneralSettingsDlg::OnCbnSelchangeSelectCpuCombo)
    ON_BN_CLICKED(IDC_PLUGIN_MANAGE_BUTTON, &CGeneralSettingsDlg::OnBnClickedPluginManageButton)
    ON_BN_CLICKED(IDC_SHOW_NOTIFY_ICON_CHECK, &CGeneralSettingsDlg::OnBnClickedShowNotifyIconCheck)
    ON_BN_CLICKED(IDC_SELECT_CONNECTIONS_BUTTON, &CGeneralSettingsDlg::OnBnClickedSelectConnectionsButton)
    ON_BN_CLICKED(IDC_RESET_AUTO_RUN_BUTTON, &CGeneralSettingsDlg::OnBnClickedResetAutoRunButton)
    ON_EN_CHANGE(IDC_MONITOR_SPAN_EDIT, &CGeneralSettingsDlg::OnEnChangeMonitorSpanEdit)
    ON_BN_CLICKED(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, &CGeneralSettingsDlg::OnBnClickedAutoRunMethodRegestryRadio)
    ON_BN_CLICKED(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, &CGeneralSettingsDlg::OnBnClickedAutoRunMethodTaskScheduleRadio)
END_MESSAGE_MAP()


// CGeneralSettingsDlg 消息处理程序


BOOL CGeneralSettingsDlg::OnInitDialog()
{
    CTabDlg::OnInitDialog();

    // TODO:  在此添加额外的初始化

    using Variant = NativeMaterial::ButtonVariant;
    const CString threshold_label = NativePreferences::InlinePair(L"Threshold", L"臨界值");
    const CString unit_label = NativePreferences::InlinePair(L"Unit", L"單位");
    // The caption of a template static that is folded into the component it describes; the static is removed.
    auto caption_of = [this](UINT id)
    {
        CString text;
        GetDlgItemText(id, text);
        RemoveStockControl(id);
        return TrimCaption(text);
    };
    // The unit text that followed a field in the template becomes the supporting text of its number field.
    auto unit_of = [this](UINT id)
    {
        CString text;
        GetDlgItemText(id, text);
        RemoveStockControl(id);
        text.Trim();
        return text;
    };

    // Section headings replace the group boxes; the group boxes are found by their translated captions.
    m_heading_application = ReplaceGroupBoxWithHeading(FindTemplateText(true, L"TXT_APPLICATION_SETTINGS"));
    m_heading_config_files = ReplaceGroupBoxWithHeading(FindTemplateText(true, L"TXT_CONFIGURATION_AND_DATA_FILES"));
    m_heading_display = ReplaceGroupBoxWithHeading(FindTemplateText(true, L"TXT_DISPLAY"));
    m_heading_auto_run = ReplaceGroupBoxWithHeading(FindTemplateText(true, L"TXT_AUTO_RUN"));
    m_heading_notification = ReplaceGroupBoxWithHeading(FindTemplateText(true, L"TXT_NOTIFICATION_MESSAGE"));
    m_heading_hardware = ReplaceGroupBoxWithHeading(GetDlgItem(IDC_HARDWARE_MONITOR_STATIC));
    m_heading_advanced = ReplaceGroupBoxWithHeading(FindTemplateText(true, L"TXT_ADVANCED"));
    m_update_source_label = ReplaceWithLabel(IDC_UPDATE_SORUCE_STATIC);
    m_auto_run_method_label = ReplaceWithLabel(FindTemplateText(false, L"TXT_AUTO_RUN_METHOD"));
    m_cpu_method_label = ReplaceWithLabel(FindTemplateText(false, L"TXT_CPU_ACQUISITION_METHOD"));

    // Check boxes
    m_check_update_check = ReplaceWithCheckbox(IDC_CHECK_UPDATE_CHECK);
    m_auto_run_check = ReplaceWithCheckbox(IDC_AUTO_RUN_CHECK);
    m_show_notify_icon_check = ReplaceWithCheckbox(IDC_SHOW_NOTIFY_ICON_CHECK);
    m_traffic_tip_check = ReplaceWithCheckbox(IDC_TODAY_TRAFFIC_TIP_CHECK);
    m_memory_tip_check = ReplaceWithCheckbox(IDC_MEMORY_USAGE_TIP_CHECK);
    m_cpu_temp_tip_check = ReplaceWithCheckbox(IDC_CPU_TEMP_TIP_CHECK);
    m_gpu_temp_tip_check = ReplaceWithCheckbox(IDC_GPU_TEMP_TIP_CHECK);
    m_hdd_temp_tip_check = ReplaceWithCheckbox(IDC_HDD_TEMP_TIP_CHECK);
    m_mbd_temp_tip_check = ReplaceWithCheckbox(IDC_MBD_TEMP_TIP_CHECK);
    m_cpu_check = ReplaceWithCheckbox(IDC_CPU_CHECK);
    m_gpu_check = ReplaceWithCheckbox(IDC_GPU_CHECK);
    m_hdd_check = ReplaceWithCheckbox(IDC_HDD_CHECK);
    m_mbd_check = ReplaceWithCheckbox(IDC_MBD_CHECK);
    m_show_all_connection_check = ReplaceWithCheckbox(IDC_SHOW_ALL_CONNECTION_CHECK);

    // Radio buttons: update source, data location, auto run mode, CPU usage method
    m_github_radio = ReplaceWithRadioButton(IDC_GITHUB_RADIO, true);
    m_gitee_radio = ReplaceWithRadioButton(IDC_GITEE_RADIO, false);
    m_save_to_appdata_radio = ReplaceWithRadioButton(IDC_SAVE_TO_APPDATA_RADIO, true);
    m_save_to_program_dir_radio = ReplaceWithRadioButton(IDC_SAVE_TO_PROGRAM_DIR_RADIO, false);
    m_auto_run_registry_radio = ReplaceWithRadioButton(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, true);
    m_auto_run_task_schedule_radio = ReplaceWithRadioButton(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, false);
    m_use_cpu_time_radio = ReplaceWithRadioButton(IDC_USE_CPU_TIME_RADIO, true);
    m_use_pdh_radio = ReplaceWithRadioButton(IDC_USE_PDH_RADIO, false);

    // Number fields
    m_traffic_tip_edit = ReplaceWithNumberStepper(IDC_TODAY_TRAFFIC_TIP_EDIT, threshold_label, unit_of(IDC_TODAY_TRAFFIC_BACK_STATIC));
    m_memory_tip_edit = ReplaceWithNumberStepper(IDC_MEMORY_USAGE_TIP_EDIT, threshold_label, unit_of(IDC_MEMORY_USAGE_BACK_STATIC));
    m_cpu_temp_tip_edit = ReplaceWithNumberStepper(IDC_CPU_TEMP_TIP_EDIT, threshold_label, unit_of(IDC_CPU_TEMP_STATIC));
    m_gpu_temp_tip_edit = ReplaceWithNumberStepper(IDC_GPU_TEMP_TIP_EDIT, threshold_label, unit_of(IDC_GPU_TEMP_STATIC));
    m_hdd_temp_tip_edit = ReplaceWithNumberStepper(IDC_HDD_TIP_EDIT, threshold_label, unit_of(IDC_HDD_STATIC));
    m_mbd_temp_tip_edit = ReplaceWithNumberStepper(IDC_MBD_TEMP_TIP_EDIT, threshold_label, unit_of(IDC_MBD_TEMP_STATIC));
    {
        const CString interval_label = caption_of(IDC_MONITOR_INTERVAL_STATIC);
        m_monitor_span_edit = ReplaceWithNumberStepper(IDC_MONITOR_SPAN_EDIT, interval_label, unit_of(IDC_MILLISECONDS_STATIC));
    }

    // Dropdowns; the caption static of a dropdown becomes its label
    {
        const CString language_caption = caption_of(IDC_LANGUAGE_STATIC);
        m_language_combo = ReplaceWithDropdown(IDC_LANGUAGE_COMBO, language_caption);
        m_traffic_tip_combo = ReplaceWithDropdown(IDC_TODAY_TRAFFIC_TIP_COMBO, unit_label);
        const CString hdd_caption = caption_of(IDC_SELECT_HDD_STATIC);
        m_hard_disk_combo = ReplaceWithDropdown(IDC_SELECT_HARD_DISK_COMBO, hdd_caption);
        const CString cpu_caption = caption_of(IDC_SELECT_CPU_STATIC);
        m_select_cpu_combo = ReplaceWithDropdown(IDC_SELECT_CPU_COMBO, cpu_caption);
    }

    // Push buttons
    m_check_now_btn = ReplaceWithButton(IDC_CHECK_NOW_BUTTON, Variant::Tonal);
    m_reset_auto_run_btn = ReplaceWithButton(IDC_RESET_AUTO_RUN_BUTTON, Variant::Tonal);
    m_open_config_path_btn = ReplaceWithButton(IDC_OPEN_CONFIG_PATH_BUTTON, Variant::Tonal);
    m_select_connection_btn = ReplaceWithButton(IDC_SELECT_CONNECTIONS_BUTTON, Variant::Tonal);
    m_restore_default_time_span_btn = ReplaceWithButton(IDC_RESTORE_DEFAULT_TIME_SPAN_BUTTON, Variant::Text);
    m_plugin_manager_btn = ReplaceWithButton(IDC_PLUGIN_MANAGE_BUTTON, Variant::Tonal);

    // Tab order follows the reading order of the page.
    {
        const std::vector<CWnd*> order{
            m_heading_application, m_check_update_check, m_check_now_btn, m_update_source_label, m_github_radio, m_gitee_radio, m_language_combo,
            m_heading_config_files, m_save_to_appdata_radio, m_save_to_program_dir_radio, m_open_config_path_btn,
            m_heading_display, m_show_notify_icon_check,
            m_heading_auto_run, m_auto_run_check, m_reset_auto_run_btn, m_auto_run_method_label, m_auto_run_registry_radio, m_auto_run_task_schedule_radio,
            m_heading_notification, m_traffic_tip_check, m_traffic_tip_edit, m_traffic_tip_combo, m_memory_tip_check, m_memory_tip_edit,
            m_cpu_temp_tip_check, m_cpu_temp_tip_edit, m_gpu_temp_tip_check, m_gpu_temp_tip_edit,
            m_hdd_temp_tip_check, m_hdd_temp_tip_edit, m_mbd_temp_tip_check, m_mbd_temp_tip_edit,
            m_heading_hardware, m_cpu_check, m_gpu_check, m_hdd_check, m_mbd_check, m_hard_disk_combo, m_select_cpu_combo,
            m_heading_advanced, m_show_all_connection_check, m_select_connection_btn, m_cpu_method_label, m_use_cpu_time_radio, m_use_pdh_radio,
            m_monitor_span_edit, m_restore_default_time_span_btn, m_plugin_manager_btn };
        for (auto it = order.rbegin(); it != order.rend(); ++it)
        {
            if (*it != nullptr)
                (*it)->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    SetDlgCheck(IDC_CHECK_UPDATE_CHECK, m_data.check_update_when_start);
    if (theApp.IsForceShowNotifyIcon())
    {
        m_data.show_notify_icon = true;
        EnableDlgCtrl(IDC_SHOW_NOTIFY_ICON_CHECK, FALSE);
    }
    SetDlgCheck(IDC_SHOW_NOTIFY_ICON_CHECK, m_data.show_notify_icon);

    SetDlgCheck(m_data.update_source == 0 ? IDC_GITHUB_RADIO : IDC_GITEE_RADIO, true);

    //检查开始菜单的“启动”目录下有没有程序的快捷方式，如果有则设置开机自启动，然后删除快捷方式
    wstring start_up_path = CCommon::GetStartUpPath();
    // Managed installations use their stable launcher and never adopt portable shortcuts.
    bool shortcut_exist = !theApp.m_installed_mode
        && CCommon::FileExist((start_up_path + L"\\TrafficMonitor.lnk").c_str());
    m_data.auto_run = false;
#ifdef WITHOUT_TEMPERATURE
    m_data.auto_run_by_task_scheduler = false;
#else
    m_data.auto_run_by_task_scheduler = true;
#endif

    if (shortcut_exist)
    {
        theApp.SetAutoRun(true, false);
        m_data.auto_run = true;
        DeleteFile((start_up_path + L"\\TrafficMonitor.lnk").c_str());
        SetDlgCheck(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, true);
    }
    //检查开机自动运行的设置是通过注册表还是任务计划程序实现的，并设置相应的选项
    else
    {
        bool auto_run_by_registry = theApp.GetAutoRun(&m_auto_run_path, false);
        if (auto_run_by_registry)
        {
            SetDlgCheck(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, true);
            m_data.auto_run = true;
            m_data.auto_run_by_task_scheduler = false;
        }
        else
        {
            bool auto_run_by_task_scheduler = theApp.GetAutoRun(&m_auto_run_path, true);
            if (auto_run_by_task_scheduler)
            {
                SetDlgCheck(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, true);
                m_data.auto_run = true;
                m_data.auto_run_by_task_scheduler = true;
            }
        }
    }
    if (m_data.auto_run_by_task_scheduler)
        SetDlgCheck(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO, true);
    else
        SetDlgCheck(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, true);

    if (theApp.m_installed_mode)
    {
        m_data.auto_run_by_task_scheduler = false;
        SetDlgCheck(IDC_AUTO_RUN_METHOD_REGESTRY_RADIO, true);
        GetDlgItem(IDC_AUTO_RUN_METHOD_TASK_SCHEDULE_RADIO)->EnableWindow(FALSE);
    }


    SetDlgCheck(IDC_SAVE_TO_APPDATA_RADIO, !m_data.portable_mode);
    SetDlgCheck(IDC_SAVE_TO_PROGRAM_DIR_RADIO, m_data.portable_mode);
    GetDlgItem(IDC_SAVE_TO_PROGRAM_DIR_RADIO)->EnableWindow(theApp.m_module_dir_writable);

    SetDlgCheck(IDC_AUTO_RUN_CHECK, m_data.auto_run);

    SetDlgCheck(IDC_TODAY_TRAFFIC_TIP_CHECK, m_data.traffic_tip_enable);
    m_traffic_tip_edit->SetRange(1, 32767);
    m_traffic_tip_edit->SetValue(m_data.traffic_tip_value);
    m_traffic_tip_combo->AddItem(_T("MB"));
    m_traffic_tip_combo->AddItem(_T("GB"));
    m_traffic_tip_combo->SetCurSel(m_data.traffic_tip_unit);
    SetDlgCheck(IDC_MEMORY_USAGE_TIP_CHECK, m_data.memory_usage_tip.enable);
    m_memory_tip_edit->SetRange(1, 100);
    m_memory_tip_edit->SetValue(m_data.memory_usage_tip.tip_value);

    SetDlgCheck(IDC_CPU_TEMP_TIP_CHECK, m_data.cpu_temp_tip.enable);
    m_cpu_temp_tip_edit->SetRange(1, 120);
    m_cpu_temp_tip_edit->SetValue(m_data.cpu_temp_tip.tip_value);

    SetDlgCheck(IDC_GPU_TEMP_TIP_CHECK, m_data.gpu_temp_tip.enable);
    m_gpu_temp_tip_edit->SetRange(1, 120);
    m_gpu_temp_tip_edit->SetValue(m_data.gpu_temp_tip.tip_value);

    SetDlgCheck(IDC_HDD_TEMP_TIP_CHECK, m_data.hdd_temp_tip.enable);
    m_hdd_temp_tip_edit->SetRange(1, 120);
    m_hdd_temp_tip_edit->SetValue(m_data.hdd_temp_tip.tip_value);

    SetDlgCheck(IDC_MBD_TEMP_TIP_CHECK, m_data.mainboard_temp_tip.enable);
    m_mbd_temp_tip_edit->SetRange(1, 120);
    m_mbd_temp_tip_edit->SetValue(m_data.mainboard_temp_tip.tip_value);

    SetControlEnable();

    m_language_combo->AddItem(CCommon::LoadText(IDS_FOLLOWING_SYSTEM));
    int current_language_index{ -1 };       //当前语言在所有语言列表中的序号
    for (size_t i = 0; i < theApp.m_str_table.GetLanguageList().size(); i++)
    {
        const LanguageInfo& language_info = theApp.m_str_table.GetLanguageList()[i];
        m_language_combo->AddItem(language_info.display_name.c_str());
        if (language_info == m_data.language)
            current_language_index = static_cast<int>(i);
    }
    m_language_combo->SetCurSel(current_language_index + 1);     //由于下拉列表第一项是“跟随系统”，因此序号需要加1

    SetDlgCheck(IDC_SHOW_ALL_CONNECTION_CHECK, m_data.show_all_interface);

    m_tooltip.AddTool(m_show_all_connection_check, CCommon::LoadText(IDS_SHOW_ALL_INFO_TIP));
    m_tooltip.AddTool(m_save_to_appdata_radio, CString(theApp.m_appdata_dir.c_str()));
    m_tooltip.AddTool(m_save_to_program_dir_radio, CString(theApp.m_module_dir.c_str()));
    AddOrUpdateAutoRunTooltip(true);
    m_tooltip.SetAppearance(MaterialAppearance());

    if (m_data.cpu_usage_acquire_method == GeneralSettingData::CA_CPU_TIME)
    {
        SetDlgCheck(IDC_USE_CPU_TIME_RADIO, true);
    }
    else if (m_data.cpu_usage_acquire_method == GeneralSettingData::CA_PDH)
    {
        SetDlgCheck(IDC_USE_PDH_RADIO, true);
    }

    m_monitor_span_edit->SetRange(MONITOR_TIME_SPAN_MIN, MONITOR_TIME_SPAN_MAX);
    m_monitor_span_edit->SetStep(MONITOR_SPAN_STEP);
    m_monitor_span_edit->SetValue(m_data.monitor_time_span);

    m_monitor_time_span_ori = m_data.monitor_time_span;
    m_update_source_ori = m_data.update_source;

    if (CTrafficMonitorDlg::Instance()->IsGetDiskUsageByPdh())
    {
        const auto& disk_names = CTrafficMonitorDlg::Instance()->GetPdhDiskUsageHelper().GetDiskNames();
        for (const auto& hdd_name : disk_names)
            m_hard_disk_combo->AddItem(hdd_name);
        int cur_index = FindDropdownItem(*m_hard_disk_combo, m_data.hard_disk_name.c_str());
        m_hard_disk_combo->SetCurSel(cur_index);
    }
#ifndef WITHOUT_TEMPERATURE
    //初始化硬件监控Check box
    SetDlgCheck(IDC_CPU_CHECK, m_data.IsHardwareEnable(HI_CPU));
    SetDlgCheck(IDC_GPU_CHECK, m_data.IsHardwareEnable(HI_GPU));
    SetDlgCheck(IDC_HDD_CHECK, m_data.IsHardwareEnable(HI_HDD));
    SetDlgCheck(IDC_MBD_CHECK, m_data.IsHardwareEnable(HI_MBD));

    if (theApp.m_pMonitor != nullptr)
    {
        CSingleLock sync(&theApp.m_minitor_lib_critical, TRUE);
        //初始化选择硬盘下拉列表
        if (!CTrafficMonitorDlg::Instance()->IsGetDiskUsageByPdh())
        {
            for (const auto& hdd_item : theApp.m_pMonitor->AllHDDTemperature())
                m_hard_disk_combo->AddItem(hdd_item.first.c_str());
            int cur_index = FindDropdownItem(*m_hard_disk_combo, m_data.hard_disk_name.c_str());
            m_hard_disk_combo->SetCurSel(cur_index);
        }
        //初始化选择CPU下拉列表
        m_select_cpu_combo->AddItem(CCommon::LoadText(IDS_AVREAGE_TEMPERATURE));
        for (const auto& cpu_item : theApp.m_pMonitor->AllCpuTemperature())
            m_select_cpu_combo->AddItem(cpu_item.first.c_str());
        int cur_index = FindDropdownItem(*m_select_cpu_combo, m_data.cpu_core_name.c_str());
        if (cur_index < 0)
            cur_index = 0;
        m_select_cpu_combo->SetCurSel(cur_index);
    }
#endif

    //不含温度监控的版本，禁用温度相关的控件
#ifdef WITHOUT_TEMPERATURE
    EnableDlgCtrl(IDC_CPU_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_CPU_TEMP_TIP_EDIT, false);
    EnableDlgCtrl(IDC_GPU_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_GPU_TEMP_TIP_EDIT, false);
    EnableDlgCtrl(IDC_HDD_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_HDD_TIP_EDIT, false);
    EnableDlgCtrl(IDC_MBD_TEMP_TIP_CHECK, false);
    EnableDlgCtrl(IDC_MBD_TEMP_TIP_EDIT, false);
    EnableDlgCtrl(IDC_CPU_CHECK, false);
    EnableDlgCtrl(IDC_GPU_CHECK, false);
    EnableDlgCtrl(IDC_HDD_CHECK, false);
    EnableDlgCtrl(IDC_MBD_CHECK, false);
    //EnableDlgCtrl(IDC_SELECT_HARD_DISK_COMBO, false);
    EnableDlgCtrl(IDC_SELECT_CPU_COMBO, false);
    EnableDlgCtrl(IDC_HARDWARE_MONITOR_STATIC, false);
#endif

    FinishMaterialPage();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}

CWnd* CGeneralSettingsDlg::FindTemplateText(bool group_box, const wchar_t* key)
{
    // The translated caption replaces the TXT_ key in the control text; a missing translation leaves the key itself.
    const std::wstring& translated = theApp.m_str_table.LoadText(key);
    const CString wanted = translated.empty() ? CString(key) : CString(translated.c_str());
    for (CWnd* child = GetWindow(GW_CHILD); child != nullptr; child = child->GetWindow(GW_HWNDNEXT))
    {
        wchar_t class_name[32]{};
        ::GetClassNameW(child->GetSafeHwnd(), class_name, 31);
        const bool is_group_box = _wcsicmp(class_name, L"Button") == 0 && (child->GetStyle() & BS_TYPEMASK) == BS_GROUPBOX;
        const bool is_static = _wcsicmp(class_name, L"Static") == 0;
        if (group_box ? !is_group_box : !is_static)
            continue;
        CString text;
        child->GetWindowText(text);
        if (text == wanted)
            return child;
    }
    return nullptr;
}


void CGeneralSettingsDlg::OnBnClickedCheckNowButton()
{
    // TODO: 在此添加控件通知处理程序代码
    theApp.CheckUpdateInThread(true);
}


void CGeneralSettingsDlg::OnBnClickedCheckUpdateCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.check_update_when_start = GetDlgCheck(IDC_CHECK_UPDATE_CHECK);
}


void CGeneralSettingsDlg::OnBnClickedAutoRunCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.auto_run = GetDlgCheck(IDC_AUTO_RUN_CHECK);
    m_auto_run_modified = true;
    SetControlEnable();
}


void CGeneralSettingsDlg::OnOK()
{
    // TODO: 在此添加专用代码和/或调用基类
    // Apply text that was typed but not yet committed, exactly as pressing Enter in the field would.
    m_traffic_tip_edit->CommitText();
    m_memory_tip_edit->CommitText();
    m_cpu_temp_tip_edit->CommitText();
    m_gpu_temp_tip_edit->CommitText();
    m_hdd_temp_tip_edit->CommitText();
    m_mbd_temp_tip_edit->CommitText();
    m_monitor_span_edit->CommitText();

    //获取消息提示的设置
    m_data.traffic_tip_value = m_traffic_tip_edit->GetValue();
    if (m_data.traffic_tip_value < 1) m_data.traffic_tip_value = 1;
    if (m_data.traffic_tip_value > 32767) m_data.traffic_tip_value = 32767;
    m_data.traffic_tip_unit = m_traffic_tip_combo->GetCurSel();

    auto checkTipValue = [](int& value)
    {
        if (value < 1) value = 1;
        if (value > 100) value = 100;
    };
    auto checkTempTipValue = [](int& value)
    {
        if (value < 1) value = 1;
        if (value > 120) value = 120;
    };
    m_data.memory_usage_tip.tip_value = m_memory_tip_edit->GetValue();
    checkTipValue(m_data.memory_usage_tip.tip_value);

    m_data.cpu_temp_tip.tip_value = m_cpu_temp_tip_edit->GetValue();
    checkTempTipValue(m_data.cpu_temp_tip.tip_value);

    m_data.gpu_temp_tip.tip_value = m_gpu_temp_tip_edit->GetValue();
    checkTempTipValue(m_data.gpu_temp_tip.tip_value);

    m_data.hdd_temp_tip.tip_value = m_hdd_temp_tip_edit->GetValue();
    checkTempTipValue(m_data.hdd_temp_tip.tip_value);

    m_data.mainboard_temp_tip.tip_value = m_mbd_temp_tip_edit->GetValue();
    checkTempTipValue(m_data.mainboard_temp_tip.tip_value);

    //获取语言的设置
    m_data.language = LanguageInfo();
    if (m_language_combo->GetCurSel() > 0)
    {
        //选择的不是“跟随系统”
        int current_language_index = m_language_combo->GetCurSel() - 1;
        if (current_language_index >= 0 && current_language_index < static_cast<int>(theApp.m_str_table.GetLanguageList().size()))
        {
            m_data.language = theApp.m_str_table.GetLanguageList()[current_language_index];
        }
    }
    if (m_data.language != theApp.m_general_data.language)
    {
        NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_LANGUAGE_CHANGE_INFO), MB_ICONINFORMATION | MB_OK);
    }
    m_show_all_interface_modified = (m_data.show_all_interface != theApp.m_general_data.show_all_interface);

    //获取数据文件保存位置的设置
    m_data.portable_mode = GetDlgCheck(IDC_SAVE_TO_PROGRAM_DIR_RADIO);
    if (m_data.portable_mode != theApp.m_general_data.portable_mode)
    {
        NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_CFG_DIR_CHANGED_INFO), MB_ICONINFORMATION | MB_OK);
    }

    //m_taskbar_item_modified = (theApp.m_taskbar_data.display_item != taskbar_displat_item_ori);

    CTabDlg::OnOK();
}


void CGeneralSettingsDlg::OnBnClickedTodayTrafficTipCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.traffic_tip_enable = GetDlgCheck(IDC_TODAY_TRAFFIC_TIP_CHECK);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedMemoryUsageTipCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.memory_usage_tip.enable = GetDlgCheck(IDC_MEMORY_USAGE_TIP_CHECK);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedOpenConfigPathButton()
{
    // TODO: 在此添加控件通知处理程序代码
    ShellExecute(NULL, _T("explore"), theApp.m_config_dir.c_str(), NULL, NULL, SW_SHOWNORMAL);
}


void CGeneralSettingsDlg::OnBnClickedShowAllConnectionCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.show_all_interface = GetDlgCheck(IDC_SHOW_ALL_CONNECTION_CHECK);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedUseCpuTimeRadio()
{
    m_data.cpu_usage_acquire_method = GeneralSettingData::CA_CPU_TIME;
}


void CGeneralSettingsDlg::OnBnClickedUsePdhRadio()
{
    m_data.cpu_usage_acquire_method = GeneralSettingData::CA_PDH;
}


void CGeneralSettingsDlg::OnEnChangeMonitorSpanEdit()
{
    //这里限制监控时间间隔只能是100的倍数
    int value = m_monitor_span_edit->GetValue();
    const int snapped = value / MONITOR_SPAN_STEP * MONITOR_SPAN_STEP;
    if (snapped != value && snapped >= MONITOR_TIME_SPAN_MIN)
    {
        value = snapped;
        m_monitor_span_edit->SetValue(value);
    }
    m_data.monitor_time_span = value;
}


void CGeneralSettingsDlg::OnBnClickedCpuTempTipCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.cpu_temp_tip.enable = GetDlgCheck(IDC_CPU_TEMP_TIP_CHECK);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedGpuTempTipCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.gpu_temp_tip.enable = GetDlgCheck(IDC_GPU_TEMP_TIP_CHECK);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedHddTempTipCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.hdd_temp_tip.enable = GetDlgCheck(IDC_HDD_TEMP_TIP_CHECK);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedMbdTempTipCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.mainboard_temp_tip.enable = GetDlgCheck(IDC_MBD_TEMP_TIP_CHECK);
    SetControlEnable();
}


void CGeneralSettingsDlg::OnBnClickedGithubRadio()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.update_source = 0;
    theApp.m_general_data.update_source = 0;        //点击“更新源”的单选按钮时，同时更改theApp中的设置，以确保点击“立即检查”按钮时使用选择的更新源更新
}


void CGeneralSettingsDlg::OnBnClickedGiteeRadio()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.update_source = 1;
    theApp.m_general_data.update_source = 1;
}


void CGeneralSettingsDlg::OnCancel()
{
    // TODO: 在此添加专用代码和/或调用基类
    theApp.m_general_data.update_source = m_update_source_ori;      //点击“取消”时恢复开始的“更新源”选项

    CTabDlg::OnCancel();
}


void CGeneralSettingsDlg::OnBnClickedRestoreDefaultTimeSpanButton()
{
    // TODO: 在此添加控件通知处理程序代码
    m_monitor_span_edit->SetValue(1000);
    m_data.monitor_time_span = m_monitor_span_edit->GetValue();
}


void CGeneralSettingsDlg::OnCbnSelchangeSelectHardDiskCombo()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.hard_disk_name = m_hard_disk_combo->GetCurText().GetString();
}


void CGeneralSettingsDlg::OnBnClickedCpuCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    bool checked = GetDlgCheck(IDC_CPU_CHECK);
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        SetDlgCheck(IDC_CPU_CHECK, false);
    }
    m_data.SetHardwareEnable(HI_CPU, checked);
}


void CGeneralSettingsDlg::OnBnClickedGpuCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    bool checked = GetDlgCheck(IDC_GPU_CHECK);
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        SetDlgCheck(IDC_GPU_CHECK, false);
    }
    m_data.SetHardwareEnable(HI_GPU, checked);
}


void CGeneralSettingsDlg::OnBnClickedHddCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    bool checked = GetDlgCheck(IDC_HDD_CHECK);
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        SetDlgCheck(IDC_HDD_CHECK, false);
    }
    m_data.SetHardwareEnable(HI_HDD, checked);
}


void CGeneralSettingsDlg::OnBnClickedMbdCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    bool checked = GetDlgCheck(IDC_MBD_CHECK);
    if (checked && !ShowHardwareMonitorWarning())
    {
        checked = false;
        SetDlgCheck(IDC_MBD_CHECK, false);
    }
    m_data.SetHardwareEnable(HI_MBD, checked);

}


void CGeneralSettingsDlg::OnCbnSelchangeSelectCpuCombo()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.cpu_core_name = m_select_cpu_combo->GetCurText().GetString();
}


void CGeneralSettingsDlg::OnBnClickedPluginManageButton()
{
    // TODO: 在此添加控件通知处理程序代码
    CPluginManagerDlg dlg;
    dlg.DoModal();
}


void CGeneralSettingsDlg::OnBnClickedShowNotifyIconCheck()
{
    // TODO: 在此添加控件通知处理程序代码
    m_data.show_notify_icon = GetDlgCheck(IDC_SHOW_NOTIFY_ICON_CHECK);
}


void CGeneralSettingsDlg::OnBnClickedSelectConnectionsButton()
{
    // TODO: 在此添加控件通知处理程序代码
    CSelectConnectionsDlg dlg(m_data.connections_hide);
    if (dlg.DoModal() == IDOK)
    {
        m_data.connections_hide = dlg.GetData();
    }
}


void CGeneralSettingsDlg::OnBnClickedResetAutoRunButton()
{
    //先删除开机自动运行
    theApp.SetAutoRunByRegistry(false);
    theApp.SetAutoRunByTaskScheduler(false);
    if (!theApp.SetAutoRun(true, m_data.auto_run_by_task_scheduler))    //重新设置开机自动运行
    {
        NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_SET_AUTO_RUN_FAILED_WARNING), MB_ICONWARNING | MB_OK);
        return;
    }
    //获取开机自动运行的路径
    bool auto_run = theApp.GetAutoRun(&m_auto_run_path, m_data.auto_run_by_task_scheduler);
    //重新勾选“开机自动运行”复选框
    SetDlgCheck(IDC_AUTO_RUN_CHECK, auto_run);
    //更新鼠标提示
    AddOrUpdateAutoRunTooltip(false);
}

void CGeneralSettingsDlg::OnBnClickedAutoRunMethodRegestryRadio()
{
    m_data.auto_run_by_task_scheduler = false;
    m_auto_run_modified = true;
    SetControlEnable();
}

void CGeneralSettingsDlg::OnBnClickedAutoRunMethodTaskScheduleRadio()
{
    m_data.auto_run_by_task_scheduler = true;
    m_auto_run_modified = true;
    SetControlEnable();
}
