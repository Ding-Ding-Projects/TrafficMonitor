// HistoryTrafficCalendarDlg.cpp: implementation file
//

#include "stdafx.h"
#include "NativeMenus/SearchableMenu.h"
#include "TrafficMonitor.h"
#include "HistoryTrafficCalendarDlg.h"
#include "HistoryTrafficColor.h"
#include "afxdialogex.h"
#include "NativeMaterial/Controls.h"
#include <algorithm>

namespace
{
    constexpr UINT kCalendarId = 0x7A10;        // control ID of the Material calendar (the template has no calendar)

    // A compact total that fits under the day number of a calendar cell, for example "812K", "3.4M", "1.2G".
    CString CompactTraffic(unsigned __int64 kBytes)
    {
        CString text;
        if (kBytes < 1024)
            text.Format(_T("%lluK"), kBytes);
        else if (kBytes < 1024ull * 1024)
            text.Format(_T("%.1fM"), kBytes / 1024.0);
        else if (kBytes < 1024ull * 1024 * 1024)
            text.Format(_T("%.1fG"), kBytes / (1024.0 * 1024));
        else
            text.Format(_T("%.1fT"), kBytes / (1024.0 * 1024 * 1024));
        return text;
    }
}


// CHistoryTrafficCalendarDlg

IMPLEMENT_DYNAMIC(CHistoryTrafficCalendarDlg, CTabDlg)

CHistoryTrafficCalendarDlg::CHistoryTrafficCalendarDlg(deque<HistoryTraffic>& history_traffics, CWnd* pParent /*=nullptr*/)
    : CTabDlg(IDD_HISTORY_TRAFFIC_CALENDAR_DIALOG, pParent), m_history_traffics(history_traffics)
{
    EnableMaterialEmbedded();
}

CHistoryTrafficCalendarDlg::~CHistoryTrafficCalendarDlg()
{
}

const HistoryTraffic* CHistoryTrafficCalendarDlg::FindDay(const NativeMaterial::CalendarDate& date) const
{
    if (!date.Valid())
        return nullptr;
    HistoryTraffic key;
    key.year = date.year;
    key.month = date.month;
    key.day = date.day;
    // The history is ordered newest first; look the day up with a binary search.
    auto iter = std::lower_bound(m_history_traffics.begin(), m_history_traffics.end(), key, HistoryTraffic::DateGreater);
    if (iter == m_history_traffics.end() || HistoryTraffic::DateGreater(key, *iter))
        return nullptr;
    return &*iter;
}

CString CHistoryTrafficCalendarDlg::TrafficText(const HistoryTraffic* traffic, const wchar_t* separator) const
{
    const unsigned __int64 total = traffic != nullptr ? traffic->kBytes() : 0;
    CString text = CCommon::LoadText(IDS_TRAFFIC_USED1);
    text += CCommon::KBytesToString(total);
    if (traffic != nullptr && !traffic->mixed && total > 0)
    {
        text += separator;
        text += CCommon::LoadText(IDS_UPLOAD, _T(": "));
        text += CCommon::KBytesToString(traffic->up_kBytes);
        text += separator;
        text += CCommon::LoadText(IDS_DOWNLOAD, _T(": "));
        text += CCommon::KBytesToString(traffic->down_kBytes);
    }
    return text;
}

CString CHistoryTrafficCalendarDlg::TipText(const NativeMaterial::CalendarDate& date) const
{
    CString tip_info;
    tip_info.Format(_T("%d/%d/%d\n"), date.year, date.month, date.day);
    tip_info += TrafficText(FindDay(date), _T("\n"));
    return tip_info;
}

void CHistoryTrafficCalendarDlg::FillDay(const NativeMaterial::CalendarDate& date, NativeMaterial::CalendarDayContent& content) const
{
    const HistoryTraffic* traffic = FindDay(date);
    if (traffic == nullptr)
        return;
    const unsigned __int64 total = traffic->kBytes();
    content.description = TrafficText(traffic, _T(", "));
    if (total > 0)
    {
        content.secondary = CompactTraffic(total);
        content.hasIndicator = true;
        content.indicator = HistoryTrafficColor(total);
    }
}

void CHistoryTrafficCalendarDlg::UpdateTip(const NativeMaterial::CalendarDate& date)
{
    const int day = date.Valid() ? date.day : 0;
    if (day == m_tip_day)
        return;
    m_tip_day = day;
    if (day > 0)
    {
        m_tool_tip.UpdateTipText(TipText(date), &m_calendar.Grid());
    }
    else
    {
        m_tool_tip.UpdateTipText(CString(), &m_calendar.Grid());
        m_tool_tip.Hide();
    }
}

void CHistoryTrafficCalendarDlg::UpdateSummary()
{
    if (m_summary == nullptr)
        return;
    // Total traffic of the shown month.
    unsigned __int64 up = 0, down = 0;
    const int days = NativeMaterial::DaysInMonth(m_calendar.VisibleYear(), m_calendar.VisibleMonth());
    for (int day = 1; day <= days; day++)
    {
        if (const HistoryTraffic* traffic = FindDay(NativeMaterial::CalendarDate{ m_calendar.VisibleYear(), m_calendar.VisibleMonth(), day }))
        {
            up += traffic->up_kBytes;
            down += traffic->down_kBytes;
        }
    }
    CString info;
    info.Format(_T("%s %s (%s: %s, %s: %s)"), CCommon::LoadText(IDS_CURRENT_MONTH_TOTAL_TRAFFIC),
        CCommon::KBytesToString(up + down),
        CCommon::LoadText(IDS_UPLOAD), CCommon::KBytesToString(up),
        CCommon::LoadText(IDS_DOWNLOAD), CCommon::KBytesToString(down)
    );
    m_summary->SetWindowText(info);
    RelayoutMaterial();
}

int CHistoryTrafficCalendarDlg::RequiredHeight(int width_px)
{
    return LayoutMaterialControls(CRect(0, 0, width_px, 0), false);
}

int CHistoryTrafficCalendarDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    // Left: the calendar with the month summary under it. Right: the menu button and the legend.
    const int gap = DPI(16), legend_width = DPI(172), swatch = DPI(12), row = DPI(28), button_height = DPI(40);
    const int width = (std::max)(1, static_cast<int>(content.Width()));
    const int calendar_width = (std::max)(DPI(308), width - legend_width - gap);
    int y = content.top + DPI(4);

    const int summary_height = m_summary != nullptr ? m_summary->MeasureHeight(width) : 0;
    const int calendar_minimum = DPI(384);
    const int calendar_height = apply ? (std::max)(calendar_minimum, static_cast<int>(content.bottom) - y - gap - summary_height) : calendar_minimum;
    if (apply && m_calendar.GetSafeHwnd() != nullptr)
        m_calendar.MoveWindow(content.left, y, calendar_width, calendar_height);

    const int column = content.left + calendar_width + gap;
    int side = y;
    if (m_menu_button != nullptr)
    {
        if (apply)
            m_menu_button->MoveWindow(column, side, DPI(56), button_height);
        side += button_height + gap;
    }
    if (m_legend_heading != nullptr)
    {
        const int height = m_legend_heading->MeasureHeight(legend_width);
        if (apply)
            m_legend_heading->MoveWindow(column, side, legend_width, height);
        side += height + DPI(8);
    }
    for (auto& entry : m_legend)
    {
        if (entry.label == nullptr)
            continue;
        const int label_left = column + swatch + DPI(8);
        const int height = entry.label->MeasureHeight((std::max)(1, legend_width - swatch - DPI(8)));
        if (apply)
        {
            entry.label->MoveWindow(label_left, side, legend_width - swatch - DPI(8), (std::max)(height, swatch));
            entry.swatch = CRect(column, side + ((std::max)(height, swatch) - swatch) / 2, column + swatch, side + ((std::max)(height, swatch) - swatch) / 2 + swatch);
        }
        side += (std::max)(height, row);
    }

    y += calendar_height + gap;
    if (m_summary != nullptr)
    {
        if (apply)
            m_summary->MoveWindow(content.left, y, width, summary_height);
        y += summary_height;
    }
    if (apply)
        Invalidate(FALSE);      // the legend swatches are painted by the page itself
    return (std::max)(y, side) + DPI(8) - content.top;
}

void CHistoryTrafficCalendarDlg::OnMaterialAppearanceChanged()
{
    if (m_calendar.GetSafeHwnd() != nullptr)
    {
        m_calendar.SetAppearance(MaterialAppearance());
        m_calendar.SetSurface(MaterialSurfaceColor());
    }
    m_tool_tip.SetAppearance(MaterialAppearance());
}

void CHistoryTrafficCalendarDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlg::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHistoryTrafficCalendarDlg, CTabDlg)
    ON_WM_PAINT()
    ON_WM_MOUSEWHEEL()
    ON_BN_CLICKED(IDC_MENU_BUTTON, &CHistoryTrafficCalendarDlg::OnBnClickedMenuButton)
    ON_WM_INITMENU()
    ON_COMMAND(ID_FIRST_DAY_OF_WEEK_SUNDAY, &CHistoryTrafficCalendarDlg::OnFirstDayOfWeekSunday)
    ON_COMMAND(ID_FIRST_DAY_OF_WEEK_MONDAY, &CHistoryTrafficCalendarDlg::OnFirstDayOfWeekMonday)
    ON_COMMAND(ID_CALENDAR_JUMP_TO_TODAY, &CHistoryTrafficCalendarDlg::OnCalendarJumpToToday)
    ON_CONTROL(NativeMaterial::CalendarMonthChanged, kCalendarId, &CHistoryTrafficCalendarDlg::OnCalendarMonthChanged)
END_MESSAGE_MAP()


// CHistoryTrafficCalendarDlg message handlers


BOOL CHistoryTrafficCalendarDlg::OnInitDialog()
{
    CTabDlg::OnInitDialog();

    // CTabDlg paints its pages white; the Material surface color of the shared appearance wins.
    SetBackgroundColor(MaterialSurfaceColor(), FALSE);

    // The Material calendar has its own year and month dropdowns and previous / next buttons, so the stock ones go.
    RemoveStockControl(IDC_YEAR_COMBO);
    RemoveStockControl(IDC_MONTH_COMBO);
    RemoveStockControl(IDC_PREVIOUS_BUTTON);
    RemoveStockControl(IDC_NEXT_BUTTON);
    // The two caption statics ("Year", "Month") went with the combo boxes: the calendar labels its own dropdowns.
    for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child != nullptr; )
    {
        const HWND next = ::GetWindow(child, GW_HWNDNEXT);
        wchar_t class_name[16]{};
        ::GetClassNameW(child, class_name, 15);
        if (::GetDlgCtrlID(child) == IDC_STATIC && _wcsicmp(class_name, L"Static") == 0)
            ::DestroyWindow(child);
        child = next;
    }
    m_menu_button = ReplaceWithButton(IDC_MENU_BUTTON, NativeMaterial::ButtonVariant::Tonal);

    // Legend and month summary.
    m_legend_heading = AddMaterialLabel(CCommon::LoadText(IDS_LEGEND, _T(":")), NativeMaterial::LabelRole::Secondary);
    const wchar_t* const legend_text[5] = { _T("0~1GB"), _T("1GB~10GB"), _T("10GB~100GB"), _T("100GB~1TB"), _T("1TB~") };
    const COLORREF legend_color[5] = { TRAFFIC_COLOR_BLUE, TRAFFIC_COLOR_GREEN, TRAFFIC_COLOR_YELLOE, TRAFFIC_COLOR_RED, TRAFFIC_COLOR_DARK_RED };
    for (int i = 0; i < 5; i++)
    {
        m_legend[i].label = AddMaterialLabel(legend_text[i]);
        m_legend[i].color = legend_color[i];
    }
    m_summary = AddMaterialLabel(CString());

    // The calendar: years of the history, first day of the week from the configuration, the newest day of the
    // history as the "today" marker, weekends highlighted, and the traffic of each day from the history.
    m_calendar.Create(CCommon::LoadText(IDS_CALENDAR_VIEW), CRect(0, 0, 308, 384), this, kCalendarId);
    if (!m_history_traffics.empty())
    {
        const HistoryTraffic& newest = m_history_traffics.front();
        m_calendar.SetYearRange(m_history_traffics.back().year, newest.year);
        m_calendar.SetToday(NativeMaterial::CalendarDate{ newest.year, newest.month, newest.day });
        m_calendar.SetVisibleMonth(newest.year, newest.month);
    }
    m_calendar.SetFirstDayOfWeek(theApp.m_cfg_data.m_sunday_first ? 0 : 1);
    m_calendar.SetWeekendHighlight(true);
    m_calendar.SetDaySource([this](const NativeMaterial::CalendarDate& date, NativeMaterial::CalendarDayContent& content) { FillDay(date, content); });
    OnMaterialAppearanceChanged();

    // Keyboard order: calendar, then the menu button.
    ::SetWindowPos(m_calendar.GetSafeHwnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ::SetWindowPos(m_menu_button->GetSafeHwnd(), m_calendar.GetSafeHwnd(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Hover details of a day.
    m_tool_tip.SetAppearance(MaterialAppearance());
    m_tool_tip.AddTool(&m_calendar.Grid(), CString());

    CCommon::LoadMenuResource(m_menu, IDR_HISTORY_TRAFFIC_MENU);

    UpdateSummary();
    FinishMaterialShell();
    return TRUE;  // return TRUE unless you set the focus to a control
}


void CHistoryTrafficCalendarDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
                       // The base class paint is intentionally not called: only the legend swatches are drawn here.
    for (const auto& entry : m_legend)
    {
        if (!entry.swatch.IsRectEmpty())
            dc.FillSolidRect(entry.swatch, entry.color);
    }
}


BOOL CHistoryTrafficCalendarDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_calendar.GetSafeHwnd() != nullptr && pMsg->hwnd == m_calendar.Grid().GetSafeHwnd())
    {
        // Keep the tooltip text on the day under the pointer (the pointer position is in grid client coordinates).
        if (pMsg->message == WM_MOUSEMOVE)
            UpdateTip(m_calendar.DateAt(CPoint(GET_X_LPARAM(pMsg->lParam), GET_Y_LPARAM(pMsg->lParam))));
        else if (pMsg->message == WM_MOUSELEAVE)
            UpdateTip(NativeMaterial::CalendarDate{});
    }

    // Ctrl+Left and Ctrl+Right page to the previous and next month (the arrow keys alone move the focused day).
    if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT) && (GetKeyState(VK_CONTROL) & 0x8000)
        && m_calendar.GetSafeHwnd() != nullptr && ::IsChild(m_calendar.GetSafeHwnd(), pMsg->hwnd))
    {
        m_calendar.Navigate(pMsg->wParam == VK_LEFT ? NativeMaterial::CalendarMove::PreviousMonth : NativeMaterial::CalendarMove::NextMonth);
        return TRUE;
    }

    return CTabDlg::PreTranslateMessage(pMsg);
}


BOOL CHistoryTrafficCalendarDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // Page through the months with the mouse wheel (over the calendar grid the grid itself does the same).
    if (zDelta > 0)
        m_calendar.Navigate(NativeMaterial::CalendarMove::PreviousMonth);
    if (zDelta < 0)
        m_calendar.Navigate(NativeMaterial::CalendarMove::NextMonth);

    return CTabDlg::OnMouseWheel(nFlags, zDelta, pt);
}


void CHistoryTrafficCalendarDlg::OnBnClickedMenuButton()
{
    if (m_menu_button != nullptr)
    {
        CRect rect;
        m_menu_button->GetWindowRect(rect);
        CPoint point;
        point.x = rect.left;
        point.y = rect.bottom;
        CMenu* pMenu = m_menu.GetSubMenu(1);
        if (pMenu != NULL)
            NativeMenus::Track(pMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
    }
}


void CHistoryTrafficCalendarDlg::OnInitMenu(CMenu* pMenu)
{
    CTabDlg::OnInitMenu(pMenu);

    if (theApp.m_cfg_data.m_sunday_first)
        pMenu->CheckMenuRadioItem(ID_FIRST_DAY_OF_WEEK_SUNDAY, ID_FIRST_DAY_OF_WEEK_MONDAY, ID_FIRST_DAY_OF_WEEK_SUNDAY, MF_BYCOMMAND | MF_CHECKED);
    else
        pMenu->CheckMenuRadioItem(ID_FIRST_DAY_OF_WEEK_SUNDAY, ID_FIRST_DAY_OF_WEEK_MONDAY, ID_FIRST_DAY_OF_WEEK_MONDAY, MF_BYCOMMAND | MF_CHECKED);

}


void CHistoryTrafficCalendarDlg::OnFirstDayOfWeekSunday()
{
    theApp.m_cfg_data.m_sunday_first = true;
    m_calendar.SetFirstDayOfWeek(0);
}


void CHistoryTrafficCalendarDlg::OnFirstDayOfWeekMonday()
{
    theApp.m_cfg_data.m_sunday_first = false;
    m_calendar.SetFirstDayOfWeek(1);
}


void CHistoryTrafficCalendarDlg::OnCalendarJumpToToday()
{
    if (m_history_traffics.empty())
        return;
    m_calendar.SetVisibleMonth(m_history_traffics.front().year, m_history_traffics.front().month);
    UpdateSummary();        // programmatic changes of the shown month do not notify
}


void CHistoryTrafficCalendarDlg::OnCalendarMonthChanged()
{
    m_tip_day = 0;
    UpdateSummary();
}
