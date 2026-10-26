#pragma once
#include <map>
#include <memory>
#include "BaseDialog.h"

namespace NativeMaterial { class DataTable; }

// CPluginManagerDlg 对话框 (Material shell: data table with plugin icons and a status indicator)

class CPluginManagerDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CPluginManagerDlg)

public:
    CPluginManagerDlg(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CPluginManagerDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PLUGIN_MANAGER_DIALOG };
#endif

private:
    std::unique_ptr<NativeMaterial::DataTable> m_list_ctrl;     // replaces the list view; the row data is the plugin index
    int m_item_selected{ -1 };
    CMenu m_menu;
    // The former link statics are Material text buttons with the same control IDs.
    NativeMaterial::Button* m_plugin_download_lnk{};
    NativeMaterial::Button* m_plugin_dev_guide_lnk{};
    NativeMaterial::Button* m_open_plugin_dir_lnk{};
    NativeMaterial::Button* m_options_button{};
    NativeMaterial::Button* m_info_button{};
    NativeMaterial::Button* m_close_button{};
    std::map<UINT, CString> m_urls;                             // control ID -> address opened by the link

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    void EnableControl();
    bool IsSelectedValid();
    bool IsSelectedPluginEnable();     //选中插件是否可用

    enum Column
    {
        COL_FILE_NAME,
        COL_NAME,
        COL_VERSION,
        COL_STATUS,
        COL_MAX
    };

    DECLARE_MESSAGE_MAP()

    // 通过 CBaseDialog 继承
    virtual CString GetDialogName() const override;
    virtual int LayoutMaterialControls(const CRect& content, bool apply) override;
    virtual void OnMaterialAppearanceChanged() override;

public:
    virtual BOOL OnInitDialog();
    afx_msg void OnTableSelectionChanged();
    afx_msg void OnTableActivated();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnBnClickedOptinsButton();
    afx_msg void OnBnClickedPluginInfoButton();
    afx_msg void OnInitMenu(CMenu* pMenu);
    afx_msg void OnPluginDetail();
    afx_msg void OnPluginOptions();
    afx_msg void OnPluginDisable();
protected:
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};
