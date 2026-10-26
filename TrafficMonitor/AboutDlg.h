#pragma once
#include <map>
#include "LinkStatic.h"
#include "BaseDialog.h"

// CAboutDlg dialog of the application "About" menu item (Material shell)

class CAboutDlg : public CBaseDialog
{
public:
    CAboutDlg();

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    // The former link statics are Material text buttons with the same control IDs.
    NativeMaterial::Button* m_mail{};
    NativeMaterial::Button* m_acknowledgement{};
    NativeMaterial::Button* m_github{};
    NativeMaterial::Button* m_gitee{};
    NativeMaterial::Button* m_license{};
    NativeMaterial::Button* m_openhardwaremonitor_link{};
    NativeMaterial::Button* m_tinyxml2_link{};
    NativeMaterial::Button* m_musicplayer2_link{};
    NativeMaterial::Button* m_simplenotepad_link{};
    NativeMaterial::Button* m_translator_link{};        // present when the translation has a contact address
    NativeMaterial::Label* m_translator_text{};         // present when the translation has no contact address
    NativeMaterial::Label* m_version{};
    NativeMaterial::Label* m_copyright{};
    NativeMaterial::Label* m_third_party_caption{};
    NativeMaterial::Label* m_other_software_caption{};
    NativeMaterial::Button* m_ok{};
    std::map<UINT, CString> m_urls;                     // control ID -> address opened by the link
    CToolTipCtrl m_tool_tip;                            // hover text of the links (stock tool tip, see the conversion plan)

    CBitmap m_about_pic;
    CRect m_pic_rect{};                                 // header picture area, set by the layout

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    CString GetDonateList();        // load the donor list from the resource file
    virtual CString GetDialogName() const override;
    virtual int LayoutMaterialControls(const CRect& content, bool apply) override;
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;

    // Implementation
protected:
    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnPaint();
};
