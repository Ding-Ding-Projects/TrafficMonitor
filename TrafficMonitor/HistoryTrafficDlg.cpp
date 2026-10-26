// HistoryTrafficDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "HistoryTrafficDlg.h"
#include "afxdialogex.h"
#include "NativeMaterial/Controls.h"


// CHistoryTrafficDlg

IMPLEMENT_DYNAMIC(CHistoryTrafficDlg, CBaseDialog)

CHistoryTrafficDlg::CHistoryTrafficDlg(deque<HistoryTraffic>& history_traffics, CWnd* pParent /*=NULL*/)
    : CBaseDialog(IDD_HISTORY_TRAFFIC_DIALOG, pParent), m_history_traffics(history_traffics), m_tab1_dlg(history_traffics, this), m_tab2_dlg(history_traffics, this)
{
    EnableMaterialShell();
}

CHistoryTrafficDlg::~CHistoryTrafficDlg()
{
}


CString CHistoryTrafficDlg::GetDialogName() const
{
    return _T("HistoryTrafficDlg");
}

void CHistoryTrafficDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

void CHistoryTrafficDlg::ArrangeTabOrder()
{
    // Keyboard order follows the z-order: tab bar, the two pages, then the OK button.
    HWND previous = MaterialTitleBar().IsAttached() ? MaterialTitleBar().BarWindow() : HWND_TOP;
    for (HWND window : { m_tab.GetSafeHwnd(), m_tab1_dlg.GetSafeHwnd(), m_tab2_dlg.GetSafeHwnd(), m_ok != nullptr ? m_ok->GetSafeHwnd() : static_cast<HWND>(nullptr) })
    {
        if (window == nullptr)
            continue;
        ::SetWindowPos(window, previous, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        previous = window;
    }
}

int CHistoryTrafficDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    const int margin = DPI(16), gap = DPI(8), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    int y = content.top;

    // The tab bar spans the whole content width so its divider reaches both edges.
    const int tab_height = m_tab.GetSafeHwnd() != nullptr ? m_tab.PreferredHeight() : DPI(48);
    if (apply && m_tab.GetSafeHwnd() != nullptr)
        m_tab.MoveWindow(content.left, y, content.Width(), tab_height);
    y += tab_height + gap;

    const int footer = gap + button_height + margin;
    int page_minimum = 0;
    if (m_tab1_dlg.GetSafeHwnd() != nullptr)
        page_minimum = (std::max)(m_tab1_dlg.RequiredHeight(width), m_tab2_dlg.RequiredHeight(width));
    const int page_height = apply ? (std::max)(page_minimum, static_cast<int>(content.bottom) - footer - y) : page_minimum;
    if (apply && m_tab1_dlg.GetSafeHwnd() != nullptr)
    {
        m_tab1_dlg.MoveWindow(content.left + margin, y, width, page_height);
        m_tab2_dlg.MoveWindow(content.left + margin, y, width, page_height);
    }
    y += page_height + gap;

    if (m_ok != nullptr)
    {
        CString ok;
        m_ok->GetWindowText(ok);
        const int ok_width = MeasureMaterialButtonWidth(ok, NativeMaterial::ButtonVariant::Primary, 88);
        if (apply)
            m_ok->MoveWindow(content.left + margin + width - ok_width, y, ok_width, button_height);
        y += button_height;
    }
    return y + margin - content.top;
}

void CHistoryTrafficDlg::OnMaterialAppearanceChanged()
{
    if (m_tab.GetSafeHwnd() != nullptr)
    {
        m_tab.SetAppearance(MaterialAppearance());
        m_tab.SetSurfaceColor(MaterialSurfaceColor());
    }
}


BEGIN_MESSAGE_MAP(CHistoryTrafficDlg, CBaseDialog)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CHistoryTrafficDlg::OnTcnSelchangeTab1)
END_MESSAGE_MAP()


// CHistoryTrafficDlg message handlers


BOOL CHistoryTrafficDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    SetIcon(theApp.GetMenuIcon(IDI_STATISTICS), FALSE);		// small icon

    // The Material tab bar takes the control ID of the stock tab control, so the TCN_SELCHANGE map entry keeps working.
    RemoveStockControl(IDC_TAB1);
    m_tab.Create(CRect(0, 0, 10, 10), this, IDC_TAB1);
    {
        CString title;
        GetWindowText(title);
        m_tab.SetAccessibleName(title);
    }
    m_tab.AddTab(CCommon::LoadText(IDS_LIST_VIEW));
    m_tab.AddTab(CCommon::LoadText(IDS_CALENDAR_VIEW));
    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);

    // The two pages are child dialogs of this window now (they used to be children of the tab control).
    m_tab1_dlg.Create(IDD_HISTORY_TRAFFIC_LIST_DIALOG, this);
    m_tab2_dlg.Create(IDD_HISTORY_TRAFFIC_CALENDAR_DIALOG, this);
    m_tab.BindPage(0, &m_tab1_dlg);
    m_tab.BindPage(1, &m_tab2_dlg);
    OnMaterialAppearanceChanged();
    ArrangeTabOrder();

    // The window is wide enough for the widest page (the calendar with its legend column).
    const int page_width = (std::max)(CHistoryTrafficListDlg::MinimumWidthDip, CHistoryTrafficCalendarDlg::MinimumWidthDip);
    FitMaterialContent(page_width + 32);
    FinishMaterialShell();

    // Select the default tab; this shows its page and hides the other one without notifying.
    m_tab.SetSelected(m_tab_selected);

    return TRUE;  // return TRUE unless you set the focus to a control
}



BOOL CHistoryTrafficDlg::PreTranslateMessage(MSG* pMsg)
{
    // Ctrl+Tab and Ctrl+Shift+Tab cycle the tabs while the focus is on the bar or inside a page.
    if (m_tab.GetSafeHwnd() != nullptr && m_tab.HandleTabNavigation(*pMsg))
        return TRUE;

    return CBaseDialog::PreTranslateMessage(pMsg);
}

void CHistoryTrafficDlg::OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
    // The tab bar has already shown the page of the new tab and hidden the other one; the focus stays on the bar so
    // that the arrow keys can keep changing the selection.
    m_tab_selected = m_tab.GetSelected();
    *pResult = 0;
}

