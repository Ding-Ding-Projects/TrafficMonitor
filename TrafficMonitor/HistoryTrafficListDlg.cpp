// HistoryTrafficListDlg.cpp: implementation file
//

#include "stdafx.h"
#include "NativeMenus/SearchableMenu.h"
#include "TrafficMonitor.h"
#include "HistoryTrafficListDlg.h"
#include "HistoryTrafficColor.h"
#include "afxdialogex.h"
#include "CalendarHelper.h"
#include "NativeMaterial/InputControls.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
    // Columns of the table.
    enum Column { ColDate, ColUpload, ColDownload, ColTotal, ColFigure };

    // The caption statics of the template (control ID IDC_STATIC), in template order.
    std::vector<CString> TakeCaptionStatics(CWnd& dialog)
    {
        std::vector<HWND> statics;
        for (HWND child = ::GetWindow(dialog.GetSafeHwnd(), GW_CHILD); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
        {
            wchar_t class_name[16]{};
            ::GetClassNameW(child, class_name, 15);
            if (::GetDlgCtrlID(child) == IDC_STATIC && _wcsicmp(class_name, L"Static") == 0)
                statics.push_back(child);
        }
        std::vector<CString> captions;
        for (HWND window : statics)
        {
            CString text;
            CWnd::FromHandle(window)->GetWindowText(text);
            captions.push_back(text);
            ::DestroyWindow(window);
        }
        return captions;
    }
}


// CHistoryTrafficListDlg

IMPLEMENT_DYNAMIC(CHistoryTrafficListDlg, CTabDlg)

CHistoryTrafficListDlg::CHistoryTrafficListDlg(deque<HistoryTraffic>& history_traffics, CWnd* pParent /*=nullptr*/)
	: CTabDlg(IDD_HISTORY_TRAFFIC_LIST_DIALOG, pParent), m_history_traffics(history_traffics)
{
	EnableMaterialEmbedded();
}

CHistoryTrafficListDlg::~CHistoryTrafficListDlg()
{
}


int CHistoryTrafficListDlg::RequiredHeight(int width_px)
{
	return LayoutMaterialControls(CRect(0, 0, width_px, 0), false);
}

int CHistoryTrafficListDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
	const int gap = DPI(12), table_minimum = DPI(240);
	const int width = (std::max)(1, static_cast<int>(content.Width()));
	int y = content.top + DPI(4);
	if (m_view_type_combo != nullptr && m_view_scale_combo != nullptr)
	{
		const int height = m_view_type_combo->MinimumSize().cy;
		const int each = (std::max)(1, (std::min)(DPI(240), (width - gap) / 2));
		if (apply)
		{
			m_view_type_combo->MoveWindow(content.left, y, each, height);
			m_view_scale_combo->MoveWindow(content.left + each + gap, y, each, height);
		}
		y += height + gap;
	}
	const int table_height = apply ? (std::max)(table_minimum, static_cast<int>(content.bottom) - y) : table_minimum;
	if (apply && m_table.GetSafeHwnd() != nullptr)
		m_table.MoveWindow(content.left, y, width, table_height);
	return y + table_height - content.top;
}

void CHistoryTrafficListDlg::OnMaterialAppearanceChanged()
{
	if (m_table.GetSafeHwnd() != nullptr)
	{
		m_table.SetAppearance(MaterialAppearance());
		m_table.SetSurfaceColor(MaterialSurfaceColor());
	}
}

CString CHistoryTrafficListDlg::DateText(const HistoryRow& row) const
{
	const auto view_type = theApp.m_cfg_data.m_view_type;
	CString date_str;
	if (view_type == HistoryTrafficViewType::HV_DAY)
	{
		date_str.Format(_T("%.4d/%.2d/%.2d ("), row.year, row.month, row.day);
		static const wchar_t* const week_days[7] = { IDS_SUNDAY, IDS_MONDAY, IDS_TUESDAY, IDS_WEDNESDAY, IDS_THURSDAY, IDS_FRIDAY, IDS_SATURDAY };
		const int week_day = CCalendarHelper::CaculateWeekDay(row.year, row.month, row.day);
		if (week_day >= 0 && week_day <= 6)
			date_str += CCommon::LoadText(week_days[week_day]);
		date_str += _T(')');
	}
	else if (view_type == HistoryTrafficViewType::HV_WEEK)
	{
		date_str.Format(_T("%.4d/"), row.year);
		date_str += CCommon::LoadTextFormat(IDS_WEEK_NUM, { row.week });
	}
	else if (view_type == HistoryTrafficViewType::HV_MONTH)
	{
		date_str.Format(_T("%.4d/%.2d"), row.year, row.month);
	}
	else if (view_type == HistoryTrafficViewType::HV_QUARTER)
	{
		date_str.Format(_T("%.4d/"), row.year);
		if (row.month <= 3)
			date_str += _T("Q1");
		else if (row.month <= 6)
			date_str += _T("Q2");
		else if (row.month <= 9)
			date_str += _T("Q3");
		else
			date_str += _T("Q4");
	}
	else
	{
		date_str.Format(_T("%.4d"), row.year);
	}
	return date_str;
}

void CHistoryTrafficListDlg::BuildRows()
{
	m_rows.clear();
	const auto view_type = theApp.m_cfg_data.m_view_type;
	CString last_label;
	for (const auto& traffic : m_history_traffics)
	{
		HistoryRow row;
		row.year = traffic.year;
		row.month = traffic.month;
		row.day = traffic.day;
		row.up_kBytes = traffic.up_kBytes;
		row.down_kBytes = traffic.down_kBytes;
		if (view_type == HistoryTrafficViewType::HV_DAY)
		{
			row.mixed = traffic.mixed;
			m_rows.push_back(row);
			continue;
		}
		// Week, month, quarter and year rows add up consecutive days that share the same label.
		if (view_type == HistoryTrafficViewType::HV_WEEK)
			row.week = traffic.week();
		const CString label = DateText(row);
		if (m_rows.empty() || label != last_label)
		{
			last_label = label;
			m_rows.push_back(row);
		}
		else
		{
			m_rows.back().up_kBytes += traffic.up_kBytes;
			m_rows.back().down_kBytes += traffic.down_kBytes;
		}
	}

	m_max_traffic = 0;
	for (const auto& row : m_rows)
		m_max_traffic = (std::max)(m_max_traffic, row.up_kBytes + row.down_kBytes);

	m_order.resize(m_rows.size());
	std::iota(m_order.begin(), m_order.end(), 0);
}

void CHistoryTrafficListDlg::ApplySort()
{
	std::iota(m_order.begin(), m_order.end(), 0);
	const int column = m_table.SortColumn();
	const auto direction = m_table.SortOrder();
	if (column < 0 || direction == NativeMaterial::SortDirection::None)
		return;			// the natural order: newest first
	auto key = [&](int index) -> unsigned __int64
	{
		const HistoryRow& row = m_rows[index];
		switch (column)
		{
		case ColUpload: return row.mixed ? 0 : row.up_kBytes;
		case ColDownload: return row.mixed ? 0 : row.down_kBytes;
		case ColTotal: case ColFigure: return row.up_kBytes + row.down_kBytes;
		default: return 0;
		}
	};
	const bool ascending = direction == NativeMaterial::SortDirection::Ascending;
	std::stable_sort(m_order.begin(), m_order.end(), [&](int a, int b)
	{
		if (column == ColDate)		// the natural order is newest first, so ascending dates run from the highest index down
			return ascending ? a > b : a < b;
		return ascending ? key(a) < key(b) : key(a) > key(b);
	});
}

double CHistoryTrafficListDlg::BarFraction(unsigned __int64 total_kbytes) const
{
	if (m_max_traffic == 0)
		return 0;
	const double range = (std::min)(1000.0, static_cast<double>(total_kbytes) * 1000 / m_max_traffic);		// 0..1000
	if (theApp.m_cfg_data.m_use_log_scale)		// logarithmic scale (y = ln(x + 1))
		return std::log(range + 1) / std::log(1000 + 1);
	return range / 1000;						// linear scale (y = x)
}

void CHistoryTrafficListDlg::FillCell(int row, int column, NativeMaterial::TableCell& cell) const
{
	if (row < 0 || row >= static_cast<int>(m_order.size()))
		return;
	const HistoryRow& data = m_rows[m_order[row]];
	const unsigned __int64 total_kbytes = data.up_kBytes + data.down_kBytes;
	switch (column)
	{
	case ColDate:
		cell.text = DateText(data);
		break;
	case ColUpload:
		cell.text = data.mixed ? CString(_T("-")) : CCommon::KBytesToString(data.up_kBytes);
		break;
	case ColDownload:
		cell.text = data.mixed ? CString(_T("-")) : CCommon::KBytesToString(data.down_kBytes);
		break;
	case ColTotal:
		cell.text = CCommon::KBytesToString(total_kbytes);
		break;
	case ColFigure:
		cell.text = CCommon::KBytesToString(total_kbytes);		// accessible value of the bar
		cell.hasBar = true;
		cell.barFraction = static_cast<float>(BarFraction(total_kbytes));
		cell.barColor = HistoryTrafficColor(total_kbytes);
		break;
	default:
		break;
	}
}

void CHistoryTrafficListDlg::ShowListData()
{
	BuildRows();
	ApplySort();
	m_table.SetVirtualRows(static_cast<int>(m_rows.size()), [this](int row, int column, NativeMaterial::TableCell& cell) { FillCell(row, column, cell); });
}

void CHistoryTrafficListDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlg::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CHistoryTrafficListDlg, CTabDlg)
	ON_WM_INITMENU()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_USE_LINEAR_SCALE, &CHistoryTrafficListDlg::OnUseLinearScale)
	ON_COMMAND(ID_USE_LOG_SCALE, &CHistoryTrafficListDlg::OnUseLogScale)
	ON_CONTROL(NativeMaterial::TableSortChanged, IDC_HISTORY_INFO_LIST, &CHistoryTrafficListDlg::OnTableSortChanged)
    ON_CBN_SELCHANGE(IDC_VIEW_TYPE_COMBO, &CHistoryTrafficListDlg::OnCbnSelchangeViewTypeCombo)
    ON_CBN_SELCHANGE(IDC_VIEW_SCALE_COMBO, &CHistoryTrafficListDlg::OnCbnSelchangeViewScaleCombo)
END_MESSAGE_MAP()


// CHistoryTrafficListDlg message handlers


BOOL CHistoryTrafficListDlg::OnInitDialog()
{
	CTabDlg::OnInitDialog();

	// CTabDlg paints its pages white; the Material surface color of the shared appearance wins.
	SetBackgroundColor(MaterialSurfaceColor(), FALSE);

	// The two caption statics become the labels of the dropdowns that replace the combo boxes.
	const std::vector<CString> captions = TakeCaptionStatics(*this);
	m_view_type_combo = ReplaceWithDropdown(IDC_VIEW_TYPE_COMBO, captions.size() > 0 ? captions[0] : CString());
	m_view_scale_combo = ReplaceWithDropdown(IDC_VIEW_SCALE_COMBO, captions.size() > 1 ? captions[1] : CString());

	m_view_type_combo->AddItem(CCommon::LoadText(IDS_DAY_VIEW));
	m_view_type_combo->AddItem(CCommon::LoadText(IDS_WEEK_VIEW));
	m_view_type_combo->AddItem(CCommon::LoadText(IDS_MONTH_VIEW));
	m_view_type_combo->AddItem(CCommon::LoadText(IDS_QUARTER_VIEW));
	m_view_type_combo->AddItem(CCommon::LoadText(IDS_YEAR_VIEW));
	m_view_type_combo->SetCurSel(static_cast<int>(theApp.m_cfg_data.m_view_type));

	m_view_scale_combo->AddItem(CCommon::LoadText(IDS_LINEAR_SCALE));
	m_view_scale_combo->AddItem(CCommon::LoadText(IDS_LOG_SCALE));
	m_view_scale_combo->SetCurSel(theApp.m_cfg_data.m_use_log_scale ? 1 : 0);

	// The Material data table takes the control ID of the list view.
	CRect list_rect = GetControlRect(IDC_HISTORY_INFO_LIST);
	RemoveStockControl(IDC_HISTORY_INFO_LIST);
	m_table.Create(list_rect, this, IDC_HISTORY_INFO_LIST);
	m_table.SetAccessibleName(CCommon::LoadText(IDS_LIST_VIEW));
	m_table.SetColumns({
		{ CCommon::LoadText(IDS_DATE), 200, NativeMaterial::ColumnAlign::Left, true },
		{ CCommon::LoadText(IDS_UPLOAD), 120, NativeMaterial::ColumnAlign::Right, true },
		{ CCommon::LoadText(IDS_DOWNLOAD), 120, NativeMaterial::ColumnAlign::Right, true },
		{ CCommon::LoadText(IDS_TRAFFIC_USED), 120, NativeMaterial::ColumnAlign::Right, true },
		{ CCommon::LoadText(IDS_FIGURE), 160, NativeMaterial::ColumnAlign::Left, true },
	});
	// Keyboard order: view type, scale, table.
	::SetWindowPos(m_table.GetSafeHwnd(), m_view_scale_combo->GetSafeHwnd(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	OnMaterialAppearanceChanged();

	ShowListData();

	CCommon::LoadMenuResource(m_menu, IDR_HISTORY_TRAFFIC_MENU);

	FinishMaterialShell();
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CHistoryTrafficListDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// The table sends WM_CONTEXTMENU with the screen position (or the focused row for Shift+F10).
	if (pWnd != nullptr && pWnd->GetSafeHwnd() == m_table.GetSafeHwnd())
	{
		CMenu* pMenu = m_menu.GetSubMenu(0);
		if (pMenu != nullptr)
			NativeMenus::Track(pMenu, TPM_RIGHTBUTTON, point.x, point.y, this);
		return;
	}
	CTabDlg::OnContextMenu(pWnd, point);
}


void CHistoryTrafficListDlg::OnInitMenu(CMenu* pMenu)
{
	CTabDlg::OnInitMenu(pMenu);

	if (theApp.m_cfg_data.m_use_log_scale)
		pMenu->CheckMenuRadioItem(ID_USE_LINEAR_SCALE, ID_USE_LOG_SCALE, ID_USE_LOG_SCALE, MF_BYCOMMAND | MF_CHECKED);
	else
		pMenu->CheckMenuRadioItem(ID_USE_LINEAR_SCALE, ID_USE_LOG_SCALE, ID_USE_LINEAR_SCALE, MF_BYCOMMAND | MF_CHECKED);
}


void CHistoryTrafficListDlg::OnUseLinearScale()
{
	theApp.m_cfg_data.m_use_log_scale = false;
	m_view_scale_combo->SetCurSel(0);
	m_table.Refresh();
}


void CHistoryTrafficListDlg::OnUseLogScale()
{
	theApp.m_cfg_data.m_use_log_scale = true;
	m_view_scale_combo->SetCurSel(1);
	m_table.Refresh();
}


void CHistoryTrafficListDlg::OnTableSortChanged()
{
	ApplySort();
	m_table.Refresh();
}


void CHistoryTrafficListDlg::OnCbnSelchangeViewTypeCombo()
{
    theApp.m_cfg_data.m_view_type = static_cast<HistoryTrafficViewType>(m_view_type_combo->GetCurSel());
    ShowListData();
}


void CHistoryTrafficListDlg::OnCbnSelchangeViewScaleCombo()
{
    theApp.m_cfg_data.m_use_log_scale = (m_view_scale_combo->GetCurSel() != 0);
    m_table.Refresh();
}
