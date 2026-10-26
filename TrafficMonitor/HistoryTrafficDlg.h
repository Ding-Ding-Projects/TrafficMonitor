#pragma once
#include "Common.h"
#include "HistoryTrafficListDlg.h"
#include "HistoryTrafficCalendarDlg.h"
#include "BaseDialog.h"
#include "NativeMaterial/Tabs.h"

// CHistoryTrafficDlg: the history traffic window (Material shell). The list view and the calendar view are tab pages
// hosted by a Material tab bar.

class CHistoryTrafficDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CHistoryTrafficDlg)

public:
    CHistoryTrafficDlg(deque<HistoryTraffic>& history_traffics, CWnd* pParent = NULL);
    virtual ~CHistoryTrafficDlg();

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_HISTORY_TRAFFIC_DIALOG };
#endif

public:
    CHistoryTrafficListDlg m_tab1_dlg;
    CHistoryTrafficCalendarDlg m_tab2_dlg;

protected:
    deque<HistoryTraffic>& m_history_traffics;

    NativeMaterial::Tabs m_tab;             // replaces the stock tab control; same control ID
    NativeMaterial::Button* m_ok{};
    int m_tab_selected{};

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    void ArrangeTabOrder();                 // tab bar, list page, calendar page, then the OK button
    virtual CString GetDialogName() const override;
    virtual int LayoutMaterialControls(const CRect& content, bool apply) override;
    virtual void OnMaterialAppearanceChanged() override;

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
};
