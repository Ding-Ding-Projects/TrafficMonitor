#pragma once
#include <map>
#include <memory>
#include "CSkinPreviewView.h"
#include "BaseDialog.h"

namespace NativeMaterial { class MaterialList; }

// CSkinDlg 对话框 (Material shell: single-select skin list, scroll rail based preview, text button links)

class CSkinDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CSkinDlg)

public:
    CSkinDlg(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~CSkinDlg();

    int m_skin_selected{};			//选择的皮肤

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SKIN_DIALOG };
#endif

    const CSkinFile& GetSkinData() const { return m_skin_data; }

protected:
    virtual CString GetDialogName() const override;
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual int LayoutMaterialControls(const CRect& content, bool apply) override;
    virtual void OnMaterialAppearanceChanged() override;
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;

protected:
    //控件变量
    std::unique_ptr<NativeMaterial::MaterialList> m_skin_list_box;   //选择皮肤列表（单选）
    // The former link statics are Material text buttons with the same control IDs.
    NativeMaterial::Button* m_skin_course{};	//“皮肤制作教程”超链接
    NativeMaterial::Button* m_skin_download{};	//“更多皮肤下载”超链接
    NativeMaterial::Button* m_open_skin_dir_lnk{};
    NativeMaterial::Button* m_auto_adapt_button{};
    NativeMaterial::Button* m_ok{};
    NativeMaterial::Button* m_cancel{};
    NativeMaterial::Checkbox* m_auto_adapt_check{};
    NativeMaterial::Label* m_select_caption{};
    NativeMaterial::Label* m_preview_heading{};     // the former group box caption
    NativeMaterial::Label* m_skin_info{};           // skin author
    NativeMaterial::Label* m_notify{};              // notice text (empty unless a notice is shown)
    CSkinPreviewView m_view;	//预览区视图类
    std::map<UINT, CString> m_urls;                 // control ID -> address opened by the link

    CSkinFile m_skin_data;		//皮肤数据

    void ShowPreview();		//显示皮肤预览

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnTableSelectionChanged();
    afx_msg void OnBnClickedSkinAutoAdaptButton();
    virtual void OnOK();
    afx_msg void OnBnClickedSkinAutoAdaptCheck();
};
