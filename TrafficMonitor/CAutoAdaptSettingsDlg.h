#pragma once
#include "BaseDialog.h"

// CAutoAdaptSettingsDlg dialog (Material shell)

class CAutoAdaptSettingsDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CAutoAdaptSettingsDlg)

public:
	CAutoAdaptSettingsDlg(TaskBarSettingData& data, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CAutoAdaptSettingsDlg();

// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ATUO_ADAPT_SETTING_DIALOG };
#endif

private:
	NativeMaterial::Dropdown* m_dark_mode_default_style_combo{};
	NativeMaterial::Dropdown* m_light_mode_default_style_combo{};
	NativeMaterial::Checkbox* m_auto_save_check{};
	NativeMaterial::Label* m_auto_save_tip{};       // the former tool tip, shown as supporting text
	NativeMaterial::Button* m_ok{};
	NativeMaterial::Button* m_cancel{};
	TaskBarSettingData& m_data;

private:
	void InitComboBox(NativeMaterial::Dropdown& combo_box, int style_sel);
	int GetComboBoxSel(const NativeMaterial::Dropdown& combo_box);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual CString GetDialogName() const override;
    virtual int LayoutMaterialControls(const CRect& content, bool apply) override;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};
