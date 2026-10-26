#pragma once
#include "TabDlg.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/InputControls.h"
#include "NativeMaterial/Tooltip.h"

// CGeneralSettingsDlg dialog (Material page)

class CGeneralSettingsDlg : public CTabDlg
{
    DECLARE_DYNAMIC(CGeneralSettingsDlg)

public:
    CGeneralSettingsDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CGeneralSettingsDlg();

    static void CheckTaskbarDisplayItem();

    //选项设置数据
    GeneralSettingData m_data;

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GENERAL_SETTINGS_DIALOG };
#endif

public:
    bool IsAutoRunModified() const { return m_auto_run_modified; }
    bool IsShowAllInterfaceModified() const { return m_show_all_interface_modified; }
    bool IsMonitorTimeSpanModified() const;
    void SetMonitorTimeSpanBaseline(int value) { m_monitor_time_span_ori = value; }
    //bool IsTaskbarItemModified() const { return m_taskbar_item_modified; }

protected:
    bool m_auto_run_modified{ false };      //如果更改了开机自动运行的设置，则会置为true
    bool m_show_all_interface_modified{ false };
    int m_monitor_time_span_ori{};
    int m_update_source_ori{};
    //bool m_taskbar_item_modified{ false };
    wstring m_auto_run_path;

    // Material components. Every one keeps the control ID of the dialog template, so the message map and the
    // data exchange keep working by ID.
    // Section headings (they replace the group boxes)
    NativeMaterial::Label* m_heading_application{};
    NativeMaterial::Label* m_heading_config_files{};
    NativeMaterial::Label* m_heading_display{};
    NativeMaterial::Label* m_heading_auto_run{};
    NativeMaterial::Label* m_heading_notification{};
    NativeMaterial::Label* m_heading_hardware{};
    NativeMaterial::Label* m_heading_advanced{};
    NativeMaterial::Label* m_update_source_label{};
    NativeMaterial::Label* m_auto_run_method_label{};
    NativeMaterial::Label* m_cpu_method_label{};
    // Check boxes
    NativeMaterial::Checkbox* m_check_update_check{};
    NativeMaterial::Checkbox* m_auto_run_check{};
    NativeMaterial::Checkbox* m_show_notify_icon_check{};
    NativeMaterial::Checkbox* m_traffic_tip_check{};
    NativeMaterial::Checkbox* m_memory_tip_check{};
    NativeMaterial::Checkbox* m_cpu_temp_tip_check{};
    NativeMaterial::Checkbox* m_gpu_temp_tip_check{};
    NativeMaterial::Checkbox* m_hdd_temp_tip_check{};
    NativeMaterial::Checkbox* m_mbd_temp_tip_check{};
    NativeMaterial::Checkbox* m_cpu_check{};
    NativeMaterial::Checkbox* m_gpu_check{};
    NativeMaterial::Checkbox* m_hdd_check{};
    NativeMaterial::Checkbox* m_mbd_check{};
    NativeMaterial::Checkbox* m_show_all_connection_check{};
    // Radio buttons (the four pairs are four groups)
    NativeMaterial::RadioButton* m_github_radio{};
    NativeMaterial::RadioButton* m_gitee_radio{};
    NativeMaterial::RadioButton* m_save_to_appdata_radio{};
    NativeMaterial::RadioButton* m_save_to_program_dir_radio{};
    NativeMaterial::RadioButton* m_auto_run_registry_radio{};
    NativeMaterial::RadioButton* m_auto_run_task_schedule_radio{};
    NativeMaterial::RadioButton* m_use_cpu_time_radio{};
    NativeMaterial::RadioButton* m_use_pdh_radio{};
    // Number steppers
    NativeMaterial::NumberStepper* m_traffic_tip_edit{};
    NativeMaterial::NumberStepper* m_memory_tip_edit{};
    NativeMaterial::NumberStepper* m_cpu_temp_tip_edit{};
    NativeMaterial::NumberStepper* m_gpu_temp_tip_edit{};
    NativeMaterial::NumberStepper* m_hdd_temp_tip_edit{};
    NativeMaterial::NumberStepper* m_mbd_temp_tip_edit{};
    NativeMaterial::NumberStepper* m_monitor_span_edit{};
    // Dropdowns
    NativeMaterial::Dropdown* m_language_combo{};
    NativeMaterial::Dropdown* m_traffic_tip_combo{};
    NativeMaterial::Dropdown* m_hard_disk_combo{};
    NativeMaterial::Dropdown* m_select_cpu_combo{};
    // Buttons
    NativeMaterial::Button* m_check_now_btn{};
    NativeMaterial::Button* m_reset_auto_run_btn{};
    NativeMaterial::Button* m_open_config_path_btn{};
    NativeMaterial::Button* m_select_connection_btn{};
    NativeMaterial::Button* m_restore_default_time_span_btn{};
    NativeMaterial::Button* m_plugin_manager_btn{};
    // The Material tooltip that replaces the system tooltip control
    NativeMaterial::Tooltip m_tooltip;

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    //设置控件的启用和禁用
    void SetControlEnable();

    virtual void OnSettingsApplied() override;
    virtual int LayoutPage(const CRect& area, bool apply) override;
    virtual void OnMaterialAppearanceChanged() override;

    //显示开启硬件监控时的提示，如果用户选择了“是”则返回true，否则返回false
    //“以后不再显示该对话框”的标记保存在注册表“\HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\DontShowMeThisDialogAgain”
    bool ShowHardwareMonitorWarning();

    //The template group box or static text whose translated caption is key (nullptr when absent)
    CWnd* FindTemplateText(bool group_box, const wchar_t* key);

    //添加或更新开机自动运行的鼠标提示
    void AddOrUpdateAutoRunTooltip(bool add);

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedCheckNowButton();
    afx_msg void OnBnClickedCheckUpdateCheck();
    afx_msg void OnBnClickedAutoRunCheck();
    virtual void OnOK();
    afx_msg void OnBnClickedTodayTrafficTipCheck();
    afx_msg void OnBnClickedMemoryUsageTipCheck();
    afx_msg void OnBnClickedOpenConfigPathButton();
    afx_msg void OnBnClickedShowAllConnectionCheck();
    afx_msg void OnBnClickedUseCpuTimeRadio();
    afx_msg void OnBnClickedUsePdhRadio();
    afx_msg void OnEnChangeMonitorSpanEdit();
    afx_msg void OnBnClickedCpuTempTipCheck();
    afx_msg void OnBnClickedGpuTempTipCheck();
    afx_msg void OnBnClickedHddTempTipCheck();
    afx_msg void OnBnClickedMbdTempTipCheck();
    afx_msg void OnBnClickedGithubRadio();
    afx_msg void OnBnClickedGiteeRadio();
    virtual void OnCancel();
    afx_msg void OnBnClickedRestoreDefaultTimeSpanButton();
    afx_msg void OnCbnSelchangeSelectHardDiskCombo();
    afx_msg void OnBnClickedCpuCheck();
    afx_msg void OnBnClickedGpuCheck();
    afx_msg void OnBnClickedHddCheck();
    afx_msg void OnBnClickedMbdCheck();
    afx_msg void OnCbnSelchangeSelectCpuCombo();
    afx_msg void OnBnClickedPluginManageButton();
    afx_msg void OnBnClickedShowNotifyIconCheck();
    afx_msg void OnBnClickedSelectConnectionsButton();
    afx_msg void OnBnClickedResetAutoRunButton();
    afx_msg void OnBnClickedAutoRunMethodRegestryRadio();
    afx_msg void OnBnClickedAutoRunMethodTaskScheduleRadio();
};
