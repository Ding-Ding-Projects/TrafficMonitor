#pragma once
#include "afxwin.h"
#include "BaseDialog.h"

// CMessageDlg dialog (Material shell)

class CMessageDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CMessageDlg)

public:
	CMessageDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMessageDlg();

	void SetWindowTitle(LPCTSTR str);	// window title
	void SetInfoText(LPCTSTR str);		// message heading
	void SetMessageText(LPCTSTR str);	// message body
    void SetMessageIcon(HICON hIcon);

	// Standard system icons of the message dialog
	enum StandardIcon
	{
		SI_INFORMATION,
		SI_WARNING,
		SI_ERROR
	};

	// Sets a standard system icon on the message dialog
	void SetStandarnMessageIcon(StandardIcon standard_icon);

// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MESSAGE_DIALOG };
#endif

protected:
	NativeMaterial::ScrollableTextField* m_message_edit{};	// read-only body with the Material scroll rail
	NativeMaterial::Label* m_info_static{};
	NativeMaterial::Button* m_ok{};

	CString m_title;
	CString m_info;
	CString m_message;

    HICON m_icon{};
    CPoint m_icon_pos{};        // icon position, set by the layout

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual int LayoutMaterialControls(const CRect& content, bool apply) override;

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
};
