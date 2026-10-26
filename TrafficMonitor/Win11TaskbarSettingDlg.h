#pragma once
#include "afxdialogex.h"
#include "BaseDialog.h"

// CWin11TaskbarSettingDlg dialog (Material shell)

class CWin11TaskbarSettingDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CWin11TaskbarSettingDlg)

public:
	CWin11TaskbarSettingDlg(TaskBarSettingData& data, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CWin11TaskbarSettingDlg();

// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WIN11_TASKBAR_SETTING_DLG };
#endif

private:
    TaskBarSettingData& m_data;
    NativeMaterial::Checkbox* m_snap_check{};
    NativeMaterial::Checkbox* m_avoid_overlap_check{};
    NativeMaterial::NumberStepper* m_window_offset_top_edit{};
    NativeMaterial::NumberStepper* m_window_offset_left_edit{};
    NativeMaterial::NumberStepper* m_widgets_width_edit{};
    NativeMaterial::Button* m_restore_default{};
    NativeMaterial::Button* m_ok{};
    NativeMaterial::Button* m_cancel{};

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual CString GetDialogName() const override;
    virtual int LayoutMaterialControls(const CRect& content, bool apply) override;

    void EnableDlgCtrl(UINT id, bool enable);

	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnBnClickedRestoreDefaultButton();
};
