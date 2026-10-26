// MessageDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "MessageDlg.h"
#include "DrawCommon.h"
#include "NativeMaterial/Controls.h"
#include "NativeMaterial/ScrollableTextField.h"


// CMessageDlg dialog

#define MESSAGE_DLG_ICON_SIZE (DPI(32))

IMPLEMENT_DYNAMIC(CMessageDlg, CBaseDialog)

CMessageDlg::CMessageDlg(CWnd* pParent /*=NULL*/)
	: CBaseDialog(IDD_MESSAGE_DIALOG, pParent)
{
	EnableMaterialShell();
}

CMessageDlg::~CMessageDlg()
{
}

void CMessageDlg::SetWindowTitle(LPCTSTR str)
{
	m_title = str;
}

void CMessageDlg::SetInfoText(LPCTSTR str)
{
	m_info = str;
}

void CMessageDlg::SetMessageText(LPCTSTR str)
{
	m_message = str;
}

void CMessageDlg::SetMessageIcon(HICON hIcon)
{
    m_icon = hIcon;
}

void CMessageDlg::SetStandarnMessageIcon(StandardIcon standard_icon)
{
	HICON hIcon;
	PCWSTR icon{};
	switch (standard_icon)
	{
	case SI_INFORMATION:
		icon = IDI_INFORMATION;
		break;
	case SI_WARNING:
		icon = IDI_WARNING;
		break;
	case SI_ERROR:
		icon = IDI_ERROR;
		break;
	}
	HRESULT hr = LoadIconWithScaleDown(NULL, icon, theApp.DPI(32), theApp.DPI(32), &hIcon);
	if (SUCCEEDED(hr))
		SetMessageIcon(hIcon);
}

void CMessageDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
}

int CMessageDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
	// Heading row (optional icon plus heading), the scrolling body and the OK button.
	const int margin = DPI(20), gap = DPI(12), button_height = DPI(40), minimum_body = DPI(140);
	const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
	const int left = content.left + margin;
	int y = content.top + margin;

	const bool has_heading = m_info_static != nullptr && (m_icon != NULL || !m_info.IsEmpty());
	if (has_heading)
	{
		const int icon_space = m_icon != NULL ? MESSAGE_DLG_ICON_SIZE + DPI(12) : 0;
		const int text_width = (std::max)(1, width - icon_space);
		const int text_height = m_info_static->MeasureHeight(text_width);
		const int row_height = (std::max)(text_height, m_icon != NULL ? static_cast<int>(MESSAGE_DLG_ICON_SIZE) : 0);
		if (apply)
		{
			m_info_static->MoveWindow(left + icon_space, y + (row_height - text_height) / 2, text_width, text_height);
			m_icon_pos = CPoint(left, y + (row_height - (m_icon != NULL ? MESSAGE_DLG_ICON_SIZE : 0)) / 2);
		}
		y += row_height + gap;
	}

	const int footer = button_height + gap;
	const int body_height = (std::max)(minimum_body, static_cast<int>(content.Height()) - static_cast<int>(y - content.top) - footer - margin);
	if (apply && m_message_edit != nullptr)
		m_message_edit->MoveWindow(left, y, width, body_height);
	y += body_height + gap;

	if (m_ok != nullptr)
	{
		CString ok;
		m_ok->GetWindowText(ok);
		const int ok_width = MeasureMaterialButtonWidth(ok, NativeMaterial::ButtonVariant::Primary, 88);
		if (apply)
			m_ok->MoveWindow(left + width - ok_width, y, ok_width, button_height);
		y += button_height;
	}
	if (apply)
		Invalidate(FALSE);      // the icon is painted by the dialog itself
	return y + margin - content.top;
}


BEGIN_MESSAGE_MAP(CMessageDlg, CBaseDialog)
    ON_WM_PAINT()
END_MESSAGE_MAP()


// CMessageDlg message handlers


BOOL CMessageDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), FALSE);		// small icon

	// Material components replace the stock heading static, the read-only edit and the OK button.
	m_info_static = ReplaceWithLabel(IDC_INFO_STATIC);
	m_message_edit = ReplaceWithReadOnlyText(IDC_HELP_EDIT);
	m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);

	SetWindowText(m_title);
	m_info_static->SetWindowText(m_info);
	m_message_edit->SetWindowText(m_message);
	// No icon and no heading: the heading label stays hidden.
	if (m_icon == NULL && m_info.IsEmpty())
		m_info_static->ShowWindow(SW_HIDE);

	FitMaterialContent(440);
	FinishMaterialShell();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // exception: OCX property pages should return FALSE
}


void CMessageDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
                       // The base class paint is intentionally not called: only the icon is drawn here.
    if (m_icon != NULL)
    {
        CDrawCommon draw;
        draw.Create(&dc, this);
        draw.DrawIcon(m_icon, m_icon_pos, CSize(MESSAGE_DLG_ICON_SIZE, MESSAGE_DLG_ICON_SIZE));
    }
}
