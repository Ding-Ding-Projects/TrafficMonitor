#include "../stdafx.h"
#include "DataTable.h"
#include "InputControls.Internal.h"
#include <algorithm>
#include <climits>
#include <windowsx.h>

namespace NativeMaterial
{
    namespace
    {
        constexpr UINT ReleaseEditorMessage = WM_APP + 0x545;
        constexpr UINT RailId = 2, EditorId = 3;
        constexpr int TypeaheadMilliseconds = 1000;
        constexpr int TypeaheadScanLimit = 200000;

        int CompareText(const CString& a, const CString& b)
        {
            const int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, a, a.GetLength(), b, b.GetLength(), nullptr, nullptr, 0);
            return result - CSTR_EQUAL;
        }
    }

    // The only child besides the scroll rail, and only while a cell is being edited: a borderless single-line EDIT
    // that keeps native IME, selection, clipboard and undo. Enter commits, Escape cancels.
    class TableCellEditor final : public TextField
    {
    public:
        DataTable* owner{};
    protected:
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
        {
            if (owner)
            {
                if (message == WM_KEYDOWN && (w == VK_RETURN || w == VK_ESCAPE)) { owner->FinishEdit(w == VK_RETURN); return 0; }
                if (message == WM_CHAR && (w == VK_RETURN || w == VK_ESCAPE)) return 0;
                if (message == WM_GETDLGCODE)
                {
                    auto result = TextField::WindowProc(message, w, l);
                    const auto* key = reinterpret_cast<const MSG*>(l);
                    const UINT code = key ? static_cast<UINT>(key->wParam) : static_cast<UINT>(w);
                    if (code == VK_RETURN || code == VK_ESCAPE) result |= DLGC_WANTMESSAGE;
                    return result;
                }
                if (message == WM_KILLFOCUS)
                {
                    const auto result = TextField::WindowProc(message, w, l);
                    owner->EditorLostFocus();
                    return result;
                }
            }
            return TextField::WindowProc(message, w, l);
        }
    };

    DataTable::DataTable() = default;
    DataTable::~DataTable() = default;

    BOOL DataTable::Create(const RECT& bounds, CWnd* parent, UINT id) { return CreateTable(InputKind::Table, bounds, parent, id); }

    BOOL DataTable::CreateTable(InputKind kind, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_table = std::make_shared<TableAccess>();
        if (!CreateInput(kind, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN, 0, bounds, parent, id)) return FALSE;
        { std::lock_guard<std::mutex> lock(m_table->mutex); m_table->window = m_hWnd; }
        if (!m_rail.Create(this, RailId)) return FALSE;
        m_rail.changed = [this](int position) { SetScroll(position, true); };
        m_rail.SetAppearance(GetAppearance());
        CreateInputFont(m_font, Dpi());
        Relayout();
        SyncAccess();
        return TRUE;
    }

    void DataTable::SetAccessibleName(const CString& name)
    {
        m_name = name;
        if (m_hWnd) SetWindowText(name);
        SyncAccess();
    }

    void DataTable::SetListMode(bool list)
    {
        m_listMode = list;
        if (m_table) { Relayout(); SyncAccess(); }
        if (m_hWnd) Invalidate(FALSE);
    }

    // ------------------------------------------------------------------ geometry
    int DataTable::RowPx() const { return (std::max)(1, Px(TableMetrics::RowHeight)); }
    int DataTable::HeaderPx() const { return m_listMode ? 0 : Px(TableMetrics::HeaderHeight); }
    int DataTable::BodyPx() const
    {
        CRect client;
        GetClientRect(client);
        return (std::max)(0, client.Height() - HeaderPx());
    }
    int DataTable::ViewportPx() const
    {
        CRect client;
        GetClientRect(client);
        return (std::max)(0, client.Width() - (m_railShown ? Px(TableMetrics::RailWidth) : 0));
    }
    TableLayout DataTable::CurrentLayout() const { return LayoutColumns(m_columns, Dip(ViewportPx()), m_checkable, m_fillLast); }

    CRect DataTable::RowRect(int row) const
    {
        const int top = HeaderPx() + row * RowPx() - m_scrollY;
        return CRect(0, top, ViewportPx(), top + RowPx());
    }
    CRect DataTable::CellRect(int row, int column) const
    {
        const auto layout = CurrentLayout();
        if (column < 0 || column >= static_cast<int>(layout.x.size())) return CRect();
        const int top = HeaderPx() + row * RowPx() - m_scrollY;
        return CRect(Px(layout.x[column]) - m_scrollX, top, Px(layout.x[column] + layout.width[column]) - m_scrollX, top + RowPx());
    }
    CRect DataTable::HeaderRect(int column) const
    {
        CRect r = CellRect(0, column);
        r.top = 0; r.bottom = HeaderPx();
        return r;
    }
    CRect DataTable::CheckRect(int row) const
    {
        const int top = HeaderPx() + row * RowPx() - m_scrollY;
        return CRect(-m_scrollX, top, Px(TableMetrics::CheckColumn) - m_scrollX, top + RowPx());
    }
    int DataTable::FirstVisibleRow() const { return GetRowCount() ? (std::min)(GetRowCount() - 1, m_scrollY / RowPx()) : 0; }
    int DataTable::LastVisibleRow() const
    {
        const int count = GetRowCount();
        if (!count) return -1;
        return (std::min)(count - 1, (m_scrollY + (std::max)(1, BodyPx()) - 1) / RowPx());
    }

    DataTable::Hit DataTable::HitTest(CPoint point) const
    {
        Hit hit;
        CRect client;
        GetClientRect(client);
        if (!client.PtInRect(point) || point.x >= ViewportPx()) return hit;
        const int headerHeight = HeaderPx();
        const auto layout = CurrentLayout();
        if (point.y < headerHeight)
        {
            for (int c = 0; c < static_cast<int>(layout.x.size()); ++c)
            {
                const int left = Px(layout.x[c]) - m_scrollX, right = Px(layout.x[c] + layout.width[c]) - m_scrollX;
                if (m_columns[c].resizable && std::abs(point.x - right) <= Px(TableMetrics::ResizeGrip)) { hit.zone = Zone::Divider; hit.column = c; return hit; }
                if (point.x >= left && point.x < right) { hit.zone = Zone::Header; hit.column = c; return hit; }
            }
            return hit;
        }
        const int row = (point.y - headerHeight + m_scrollY) / RowPx();
        if (row < 0 || row >= GetRowCount()) return hit;
        hit.row = row;
        if (m_checkable && point.x + m_scrollX < Px(TableMetrics::CheckColumn)) { hit.zone = Zone::Check; return hit; }
        hit.zone = Zone::Body;
        for (int c = 0; c < static_cast<int>(layout.x.size()); ++c)
            if (point.x >= Px(layout.x[c]) - m_scrollX && point.x < Px(layout.x[c] + layout.width[c]) - m_scrollX) { hit.column = c; break; }
        return hit;
    }

    bool DataTable::Down(int virtualKey) const { return m_modifier ? m_modifier(virtualKey) : (GetKeyState(virtualKey) < 0); }
    bool DataTable::IsFocused() const
    {
        const HWND focus = ::GetFocus();
        return focus && (focus == m_hWnd || (m_editing && m_editor && focus == m_editor->GetSafeHwnd()));
    }

    void DataTable::Relayout()
    {
        if (!m_hWnd || !m_rail.GetSafeHwnd()) return;
        UpdateScroll();
        CRect client;
        GetClientRect(client);
        const int rail = Px(TableMetrics::RailWidth), top = HeaderPx();
        m_rail.MoveWindow(client.right - rail, top, rail, (std::max)(0, client.Height() - top));
        RepositionEditor();
    }
    void DataTable::DpiChanged()
    {
        CreateInputFont(m_font, Dpi());
        Relayout();
    }
    void DataTable::AppearanceChanged()
    {
        if (m_rail.GetSafeHwnd()) m_rail.SetAppearance(GetAppearance());
        if (m_editor && m_editor->GetSafeHwnd()) m_editor->SetAppearance(GetAppearance());
    }

    void DataTable::UpdateScroll()
    {
        if (!m_hWnd || !m_rail.GetSafeHwnd()) return;
        const long long content = static_cast<long long>(GetRowCount()) * RowPx();
        const int page = BodyPx();
        m_railShown = content > page && page > 0;
        const int most = static_cast<int>((std::max)(0LL, (std::min)(static_cast<long long>(INT_MAX), content - page)));
        m_scrollY = (std::max)(0, (std::min)(m_scrollY, most));
        m_rail.SetStep(RowPx());
        m_rail.SetMetrics(static_cast<int>((std::min)(content, static_cast<long long>(INT_MAX))), page, m_scrollY);
        const auto layout = CurrentLayout();
        m_scrollX = (std::max)(0, (std::min)(m_scrollX, (std::max)(0, Px(layout.total) - ViewportPx())));
    }
    void DataTable::SetScroll(int pixels, bool fromRail)
    {
        const long long content = static_cast<long long>(GetRowCount()) * RowPx();
        const int most = static_cast<int>((std::max)(0LL, (std::min)(static_cast<long long>(INT_MAX), content - BodyPx())));
        pixels = (std::max)(0, (std::min)(pixels, most));
        if (pixels == m_scrollY) return;
        if (m_editing) FinishEdit(true);
        m_scrollY = pixels;
        if (!fromRail) UpdateScroll();
        Invalidate(FALSE);
        SyncAccess();
    }
    void DataTable::ScrollTo(int pixels) { SetScroll(pixels, false); }
    void DataTable::EnsureVisible(int row)
    {
        if (row < 0 || row >= GetRowCount()) return;
        const int top = row * RowPx(), bottom = top + RowPx(), page = BodyPx();
        if (top < m_scrollY) SetScroll(top, false);
        else if (bottom > m_scrollY + page) SetScroll(bottom - page, false);
    }

    // -------------------------------------------------------------------- data
    void DataTable::SetColumns(const std::vector<TableColumn>& columns)
    {
        if (m_editing) CancelEdit();
        m_columns = columns;
        if (m_sortColumn >= static_cast<int>(m_columns.size())) { m_sortColumn = -1; m_sortDirection = SortDirection::None; }
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    void DataTable::SetColumnWidth(int column, int widthDip)
    {
        if (column < 0 || column >= static_cast<int>(m_columns.size())) return;
        m_columns[column].width = (std::max)(static_cast<int>(TableMetrics::MinimumColumn), widthDip);
        Relayout();
        if (m_hWnd) Invalidate(FALSE);
    }
    void DataTable::SetFillLastColumn(bool fill) { m_fillLast = fill; Relayout(); if (m_hWnd) Invalidate(FALSE); }
    void DataTable::SetEmptyText(const CString& text) { m_emptyText = text; if (m_hWnd) Invalidate(FALSE); }

    void DataTable::FetchCell(int row, int column, TableCell& cell) const
    {
        cell = TableCell();
        ++m_fetches;
        if (m_virtual) { if (m_source) m_source(row, column, cell); }
        else if (row >= 0 && row < static_cast<int>(m_rows.size()) && column >= 0 && column < static_cast<int>(m_rows[row].cells.size()))
            cell = m_rows[row].cells[column];
    }
    CString DataTable::GetCellText(int row, int column) const
    {
        if (row < 0 || row >= GetRowCount()) return CString();
        TableCell cell;
        FetchCell(row, column, cell);
        return cell.text;
    }
    bool DataTable::SetCellText(int row, int column, const CString& text)
    {
        if (m_virtual || row < 0 || row >= static_cast<int>(m_rows.size()) || column < 0) return false;
        auto& cells = m_rows[row].cells;
        if (column >= static_cast<int>(cells.size())) cells.resize(column + 1);
        cells[column].text = text;
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
        return true;
    }
    LPARAM DataTable::GetRowData(int row) const { return !m_virtual && row >= 0 && row < static_cast<int>(m_rows.size()) ? m_rows[row].data : 0; }
    int DataTable::RowIdentity(int row) const { return !m_virtual && row >= 0 && row < static_cast<int>(m_rows.size()) ? m_rows[row].id : -1; }

    void DataTable::SetRows(std::vector<TableRow> rows)
    {
        if (m_editing) CancelEdit();
        m_virtual = false; m_source = nullptr; m_virtualCount = 0;
        m_rows = std::move(rows);
        for (auto& row : m_rows) if (!row.id) row.id = m_nextId++;
        m_selected.clear(); m_focus = m_anchor = -1;
        SortRows();
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    int DataTable::AddRow(const std::vector<TableCell>& cells, LPARAM data)
    {
        if (m_virtual) return -1;
        TableRow row;
        row.cells = cells; row.data = data; row.id = m_nextId++;
        const int id = row.id;
        m_rows.push_back(std::move(row));
        SortRows();
        int index = static_cast<int>(m_rows.size()) - 1;
        for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) if (m_rows[i].id == id) { index = i; break; }
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
        return index;
    }
    bool DataTable::RemoveRow(int row)
    {
        if (m_virtual || row < 0 || row >= static_cast<int>(m_rows.size())) return false;
        if (m_editing) CancelEdit();
        m_rows.erase(m_rows.begin() + row);
        std::set<int> next;
        for (int selected : m_selected) if (selected != row) next.insert(selected > row ? selected - 1 : selected);
        m_selected = std::move(next);
        auto shift = [row](int& index) { if (index == row) index = -1; else if (index > row) --index; };
        shift(m_focus); shift(m_anchor);
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
        return true;
    }
    void DataTable::ClearRows()
    {
        if (m_editing) CancelEdit();
        m_rows.clear(); m_selected.clear(); m_focus = m_anchor = -1;
        if (m_virtual) m_virtualCount = 0;
        m_scrollY = 0;
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    void DataTable::SetVirtualRows(int count, TableCellSource source)
    {
        if (m_editing) CancelEdit();
        m_virtual = true; m_source = std::move(source); m_virtualCount = (std::max)(0, count);
        m_rows.clear(); m_selected.clear(); m_focus = m_anchor = -1;
        m_scrollY = 0;
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    void DataTable::SetRowCount(int count)
    {
        if (!m_virtual) return;
        m_virtualCount = (std::max)(0, count);
        m_selected.erase(m_selected.lower_bound(m_virtualCount), m_selected.end());
        if (m_focus >= m_virtualCount) m_focus = -1;
        if (m_anchor >= m_virtualCount) m_anchor = -1;
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    void DataTable::Refresh()
    {
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }

    // --------------------------------------------------------------- selection
    void DataTable::SetMultiSelect(bool multi)
    {
        m_multi = multi;
        if (!multi && m_selected.size() > 1) Select({ m_focus >= 0 && m_selected.count(m_focus) ? m_focus : *m_selected.begin() }, m_focus, m_focus, false);
        SyncAccess();
    }
    void DataTable::Select(std::set<int> rows, int focus, int anchor, bool notify)
    {
        const bool changed = rows != m_selected;
        m_selected = std::move(rows);
        m_focus = focus; m_anchor = anchor;
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
        if (changed && notify)
        {
            ::NotifyWinEvent(EVENT_OBJECT_SELECTION, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
            Notify(TableSelectionChanged);
        }
    }
    void DataTable::SelectRow(int row)
    {
        if (row < 0 || row >= GetRowCount()) { ClearSelection(); return; }
        Select({ row }, row, row, false);
        EnsureVisible(row);
    }
    void DataTable::SetSelectedRows(const std::vector<int>& rows)
    {
        std::set<int> next;
        for (int row : rows) if (row >= 0 && row < GetRowCount() && (m_multi || next.empty())) next.insert(row);
        const int focus = next.empty() ? -1 : *next.rbegin();
        Select(std::move(next), focus, focus, false);
    }
    void DataTable::SelectAll()
    {
        if (!m_multi) return;
        std::set<int> all;
        for (int i = 0; i < GetRowCount(); ++i) all.insert(all.end(), i);
        Select(std::move(all), m_focus, m_anchor, false);
    }
    void DataTable::ClearSelection() { Select({}, -1, -1, false); }

    void DataTable::NavigateTo(int row, bool shift, bool ctrl)
    {
        const int count = GetRowCount();
        if (!count) return;
        row = (std::max)(0, (std::min)(count - 1, row));
        if (m_multi && shift)
        {
            const int anchor = m_anchor >= 0 ? m_anchor : (m_focus >= 0 ? m_focus : row);
            std::set<int> range;
            for (int i = (std::min)(anchor, row); i <= (std::max)(anchor, row); ++i) range.insert(range.end(), i);
            Select(std::move(range), row, anchor, true);
        }
        else if (m_multi && ctrl)
        {
            m_focus = row;
            if (m_hWnd) Invalidate(FALSE);
        }
        else Select({ row }, row, row, true);
        EnsureVisible(row);
    }

    // ----------------------------------------------------------------- sorting
    void DataTable::SetSort(int column, SortDirection direction)
    {
        m_sortColumn = direction == SortDirection::None ? -1 : column;
        m_sortDirection = m_sortColumn < 0 ? SortDirection::None : direction;
        if (m_autoSort) SortRows();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    void DataTable::SortRows()
    {
        if (m_virtual || !m_autoSort || m_sortColumn < 0 || m_sortDirection == SortDirection::None) return;
        std::set<int> selectedIds;
        for (int row : m_selected) if (row < static_cast<int>(m_rows.size())) selectedIds.insert(m_rows[row].id);
        const int focusId = m_focus >= 0 && m_focus < static_cast<int>(m_rows.size()) ? m_rows[m_focus].id : -1;
        const int anchorId = m_anchor >= 0 && m_anchor < static_cast<int>(m_rows.size()) ? m_rows[m_anchor].id : -1;
        const int column = m_sortColumn;
        const bool ascending = m_sortDirection == SortDirection::Ascending;
        auto text = [column](const TableRow& row) { return column < static_cast<int>(row.cells.size()) ? row.cells[column].text : CString(); };
        std::stable_sort(m_rows.begin(), m_rows.end(), [&](const TableRow& a, const TableRow& b)
        {
            const int c = CompareText(text(a), text(b));
            return ascending ? c < 0 : c > 0;
        });
        m_selected.clear(); m_focus = m_anchor = -1;
        for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
        {
            if (selectedIds.count(m_rows[i].id)) m_selected.insert(i);
            if (m_rows[i].id == focusId) m_focus = i;
            if (m_rows[i].id == anchorId) m_anchor = i;
        }
    }
    void DataTable::ToggleSort(int column)
    {
        if (column < 0 || column >= static_cast<int>(m_columns.size()) || !m_columns[column].sortable) return;
        if (m_editing) CancelEdit();
        m_sortDirection = (column == m_sortColumn && m_sortDirection == SortDirection::Ascending) ? SortDirection::Descending : SortDirection::Ascending;
        m_sortColumn = column;
        SortRows();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
        Notify(TableSortChanged);
    }

    // --------------------------------------------------------- check and reorder
    void DataTable::SetCheckable(bool checkable)
    {
        m_checkable = checkable;
        Relayout();
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    bool DataTable::GetChecked(int row) const { return !m_virtual && row >= 0 && row < static_cast<int>(m_rows.size()) && m_rows[row].check == CheckState::Checked; }
    void DataTable::SetChecked(int row, bool checked)
    {
        if (m_virtual || row < 0 || row >= static_cast<int>(m_rows.size())) return;
        m_rows[row].check = checked ? CheckState::Checked : CheckState::Unchecked;
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
    }
    bool DataTable::ToggleChecked(int row)
    {
        if (!m_checkable || m_virtual || row < 0 || row >= static_cast<int>(m_rows.size())) return false;
        SetChecked(row, !GetChecked(row));
        m_checkedRow = row;
        ::NotifyWinEvent(EVENT_OBJECT_STATECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
        Notify(TableCheckChanged);
        return true;
    }
    int DataTable::RemapRow(int row, int from, int to) const
    {
        if (row == from) return to;
        if (from < to && row > from && row <= to) return row - 1;
        if (from > to && row >= to && row < from) return row + 1;
        return row;
    }
    bool DataTable::MoveRow(int from, int to)
    {
        const int count = static_cast<int>(m_rows.size());
        if (m_virtual || from < 0 || to < 0 || from >= count || to >= count || from == to) return false;
        if (m_editing) CancelEdit();
        TableRow moved = std::move(m_rows[from]);
        m_rows.erase(m_rows.begin() + from);
        m_rows.insert(m_rows.begin() + to, std::move(moved));
        std::set<int> next;
        for (int row : m_selected) next.insert(RemapRow(row, from, to));
        m_selected = std::move(next);
        if (m_focus >= 0) m_focus = RemapRow(m_focus, from, to);
        if (m_anchor >= 0) m_anchor = RemapRow(m_anchor, from, to);
        m_sortColumn = -1; m_sortDirection = SortDirection::None;   // a manual order replaces any sort
        m_reorderFrom = from; m_reorderTo = to;
        EnsureVisible(to);
        SyncAccess();
        if (m_hWnd) Invalidate(FALSE);
        Notify(TableReordered);
        return true;
    }

    // ----------------------------------------------------------------- editing
    HWND DataTable::EditorWindow() const { return m_editing && m_editor ? m_editor->GetSafeHwnd() : nullptr; }

    bool DataTable::BeginEdit(int row, int column)
    {
        if (!m_hWnd || row < 0 || row >= GetRowCount() || column < 0 || column >= static_cast<int>(m_columns.size()) || !m_columns[column].editable || !IsWindowEnabled()) return false;
        if (m_editing) FinishEdit(true);
        EnsureVisible(row);
        if (!m_editor)
        {
            m_editor = std::make_unique<TableCellEditor>();
            m_editor->owner = this;
            if (!m_editor->Create(WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, CRect(0, 0, 10, 10), this, EditorId)) { m_editor.reset(); return false; }
        }
        m_editorPendingRelease = false;
        m_editor->SetAppearance(GetAppearance());
        m_editor->SetFont(&m_font);
        TableCell cell;
        FetchCell(row, column, cell);
        m_editOriginal = cell.text;
        m_editRow = row; m_editColumn = column;
        m_editing = true;
        m_editor->SetWindowText(cell.text);
        AnnotateAccessibleName(m_editor->GetSafeHwnd(), m_columns[column].title);
        RepositionEditor();
        m_editor->ShowWindow(SW_SHOW);
        m_editor->SetFocus();
        m_editor->SetSel(0, -1);
        Invalidate(FALSE);
        return true;
    }
    void DataTable::RepositionEditor()
    {
        if (!m_editing || !m_editor || !m_editor->GetSafeHwnd()) return;
        const CRect cell = CellRect(m_editRow, m_editColumn);
        CRect client;
        GetClientRect(client);
        if (cell.bottom <= HeaderPx() || cell.top >= client.bottom) { FinishEdit(true); return; }
        CClientDC dc(m_editor.get());
        auto* previous = dc.SelectObject(&m_font);
        TEXTMETRICW metrics{};
        dc.GetTextMetrics(&metrics);
        dc.SelectObject(previous);
        // TextField insets its text by 12 x 8 dip inside the window; the editor sits 4 dip inside the cell so the
        // text lines up with the cell text, which starts at the 16 dip cell padding.
        const int height = (std::min)(cell.Height(), static_cast<int>(metrics.tmHeight) + 2 * Px(8) + 2);
        const int left = cell.left + Px(4), width = (std::max)(Px(48), cell.Width() - Px(8));
        m_editor->SetWindowPos(&CWnd::wndTop, left, cell.top + (cell.Height() - height) / 2, width, height, SWP_NOACTIVATE);
    }
    void DataTable::FinishEdit(bool commit)
    {
        if (!m_editing || !m_editor) return;
        m_editing = false;
        CString text;
        m_editor->GetWindowText(text);
        const bool hadFocus = ::GetFocus() == m_editor->GetSafeHwnd();
        m_editor->ShowWindow(SW_HIDE);
        if (hadFocus) SetFocus();
        ReleaseEditorLater();
        const int row = m_editRow, column = m_editColumn;
        m_editRow = m_editColumn = -1;
        if (m_hWnd) Invalidate(FALSE);
        if (!commit || text == m_editOriginal) return;
        if (!m_virtual && row < static_cast<int>(m_rows.size()))
        {
            auto& cells = m_rows[row].cells;
            if (column >= static_cast<int>(cells.size())) cells.resize(column + 1);
            cells[column].text = text;
        }
        m_editedRow = row; m_editedColumn = column; m_editedText = text;
        SyncAccess();
        Notify(TableEditCommitted);
    }
    bool DataTable::CommitEdit() { const bool editing = m_editing; FinishEdit(true); return editing; }
    void DataTable::CancelEdit() { FinishEdit(false); }
    void DataTable::EditorLostFocus() { if (m_editing) FinishEdit(true); }
    // The editor is destroyed outside its own message handler: it has just handled the key that ended the edit.
    void DataTable::ReleaseEditorLater()
    {
        if (!m_editor) return;
        m_editorPendingRelease = true;
        if (m_hWnd) PostMessage(ReleaseEditorMessage);
    }
    void DataTable::ReleaseEditor()
    {
        if (m_editor && m_editorPendingRelease && !m_editing)
        {
            m_editor->DestroyWindow();
            m_editor.reset();
        }
        m_editorPendingRelease = false;
    }

    // ----------------------------------------------------------- accessibility
    void DataTable::SyncAccess()
    {
        if (!m_table || !m_hWnd) return;
        TableData data;
        data.name = m_name.GetString();
        data.enabled = IsWindowEnabled() != FALSE;
        data.multi = m_multi; data.listMode = m_listMode; data.checkable = m_checkable;
        data.rowCount = GetRowCount();
        data.columnCount = static_cast<int>(m_columns.size());
        for (const auto& column : m_columns) data.headers.push_back(column.title.GetString());
        data.selected.assign(m_selected.begin(), m_selected.end());
        data.firstRealized = FirstVisibleRow();
        for (int row = data.firstRealized; row <= LastVisibleRow(); ++row)
        {
            std::vector<std::wstring> texts;
            for (int column = 0; column < data.columnCount; ++column)
            {
                TableCell cell;
                FetchCell(row, column, cell);
                texts.push_back(cell.text.GetString());
            }
            data.realizedText.push_back(std::move(texts));
            data.realizedCheck.push_back(!m_virtual && row < static_cast<int>(m_rows.size()) ? (m_rows[row].check == CheckState::Checked ? 1 : m_rows[row].check == CheckState::Indeterminate ? 2 : 0) : 0);
        }
        std::lock_guard<std::mutex> lock(m_table->mutex);
        m_table->data = std::move(data);
    }
    void DataTable::ApplyAction(TableActionId action, int row)
    {
        if (row < 0 || row >= GetRowCount()) return;
        auto next = m_selected;
        switch (action)
        {
        case TableActionId::Select: Select({ row }, row, row, true); EnsureVisible(row); return;
        case TableActionId::AddToSelection: if (!m_multi) return; next.insert(row); Select(std::move(next), row, row, true); return;
        case TableActionId::RemoveFromSelection: next.erase(row); Select(std::move(next), m_focus, m_anchor, true); return;
        case TableActionId::Toggle: ToggleChecked(row); return;
        }
    }

    HRESULT DataTable::TableAutomation(IRawElementProviderSimple** provider) const
    {
        if (!provider) return E_POINTER;
        *provider = nullptr;
        if (!m_table) return E_UNEXPECTED;
        *provider = new TableProvider(m_table);
        return S_OK;
    }

    // ------------------------------------------------------------------ paint
    TablePaintModel DataTable::BuildModel() const
    {
        TablePaintModel model;
        model.columns = m_columns;
        model.rowCount = GetRowCount();
        model.viewportWidth = Dip(ViewportPx());
        model.scrollY = static_cast<float>(m_scrollY) * TableMetrics::RowHeight / RowPx();
        model.scrollX = Dip(m_scrollX);
        model.showHeader = !m_listMode;
        model.checkable = m_checkable;
        model.fillLast = m_fillLast;
        model.sortColumn = m_sortColumn; model.sortDirection = m_sortDirection;
        model.hoverHeader = m_hoverHeader; model.resizeHot = m_resizeHot;
        if (m_editing) { model.editRow = m_editRow; model.editColumn = m_editColumn; }
        if (m_dragging) model.dropGap = m_dropGap;
        model.emptyText = m_emptyText;
        for (int row = FirstVisibleRow(); row <= LastVisibleRow(); ++row)
        {
            TablePaintRow item;
            item.index = row;
            item.selected = m_selected.count(row) != 0;
            item.focused = row == m_focus;
            item.hover = row == m_hoverRow;
            if (!m_virtual && row < static_cast<int>(m_rows.size())) item.check = m_rows[row].check;
            for (int column = 0; column < static_cast<int>(m_columns.size()); ++column)
            {
                TableCell cell;
                FetchCell(row, column, cell);
                item.cells.push_back(std::move(cell));
            }
            model.rows.push_back(std::move(item));
        }
        return model;
    }
    void DataTable::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        DrawDataTable(target, palette, BuildModel(), state);
    }

    // ----------------------------------------------------------------- pointer
    void DataTable::PointerPressed(CPoint point)
    {
        m_pressPoint = point;
        m_pressRow = m_pressHeader = m_pressCheck = m_resizeColumn = m_dragRow = -1;
        m_dragging = false;
        if (m_editing) FinishEdit(true);
        const Hit hit = HitTest(point);
        if (hit.zone == Zone::Divider) { m_resizeColumn = hit.column; m_resizeStartWidth = m_columns[hit.column].width; return; }
        if (hit.zone == Zone::Header) { m_pressHeader = hit.column; return; }
        if (hit.zone == Zone::Check) { m_pressCheck = hit.row; m_focus = hit.row; return; }
        if (hit.zone != Zone::Body) return;
        const int row = hit.row;
        const bool shift = Down(VK_SHIFT), ctrl = Down(VK_CONTROL);
        if (m_multi && shift)
        {
            const int anchor = m_anchor >= 0 ? m_anchor : row;
            std::set<int> range;
            for (int i = (std::min)(anchor, row); i <= (std::max)(anchor, row); ++i) range.insert(range.end(), i);
            Select(std::move(range), row, anchor, true);
        }
        else if (m_multi && ctrl)
        {
            auto next = m_selected;
            if (!next.erase(row)) next.insert(row);
            Select(std::move(next), row, row, true);
        }
        else Select({ row }, row, row, true);
        m_pressRow = row;
        if (m_reorderable) m_dragRow = row;
        const ULONGLONG now = GetTickCount64();
        const bool twice = row == m_lastClickRow && now - m_lastClickTime <= GetDoubleClickTime()
            && std::abs(point.x - m_lastClickPoint.x) <= GetSystemMetrics(SM_CXDOUBLECLK) / 2 && std::abs(point.y - m_lastClickPoint.y) <= GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        if (twice)
        {
            m_lastClickRow = -1; m_lastClickTime = 0;
            m_activated = row;
            Notify(TableActivated);
        }
        else { m_lastClickRow = row; m_lastClickTime = now; m_lastClickPoint = point; }
    }
    void DataTable::PointerMoved(CPoint point)
    {
        if (m_resizeColumn >= 0)
        {
            const int width = (std::max)(static_cast<int>(TableMetrics::MinimumColumn), m_resizeStartWidth + static_cast<int>(std::lround(Dip(point.x - m_pressPoint.x))));
            if (width != m_columns[m_resizeColumn].width)
            {
                m_columns[m_resizeColumn].width = width;
                UpdateScroll();
                Invalidate(FALSE);
            }
            return;
        }
        if (m_dragRow >= 0 && m_pressed)
        {
            if (!m_dragging && std::abs(point.y - m_pressPoint.y) >= Px(4)) m_dragging = true;
            if (m_dragging)
            {
                const int y = point.y - HeaderPx() + m_scrollY + RowPx() / 2;
                m_dropGap = (std::max)(0, (std::min)(GetRowCount(), y / RowPx()));
                Invalidate(FALSE);
            }
            return;
        }
        const Hit hit = HitTest(point);
        const bool body = hit.zone == Zone::Body || hit.zone == Zone::Check;
        m_hoverRow = body ? hit.row : -1;
        m_hoverHeader = hit.zone == Zone::Header || hit.zone == Zone::Divider ? hit.column : -1;
        m_resizeHot = hit.zone == Zone::Divider ? hit.column : -1;
    }
    void DataTable::PointerReleased(CPoint point, bool inside)
    {
        if (m_resizeColumn >= 0)
        {
            const int column = m_resizeColumn;
            m_resizeColumn = -1;
            if (m_columns[column].width != m_resizeStartWidth) { m_resizedColumn = column; Notify(TableColumnResized); }
            return;
        }
        if (m_dragging)
        {
            const int from = m_dragRow, gap = m_dropGap;
            m_dragging = false; m_dragRow = -1; m_dropGap = -1;
            if (gap >= 0 && from >= 0) { const int to = gap > from ? gap - 1 : gap; if (to != from) MoveRow(from, to); }
            return;
        }
        m_dragRow = -1;
        if (m_pressHeader >= 0)
        {
            const Hit hit = HitTest(point);
            const int pressed = m_pressHeader;
            m_pressHeader = -1;
            if (inside && hit.zone == Zone::Header && hit.column == pressed) ToggleSort(pressed);
            return;
        }
        if (m_pressCheck >= 0)
        {
            const Hit hit = HitTest(point);
            const int pressed = m_pressCheck;
            m_pressCheck = -1;
            if (inside && hit.zone == Zone::Check && hit.row == pressed) ToggleChecked(pressed);
        }
    }
    void DataTable::PointerLeft()
    {
        if (m_resizeColumn >= 0) return;
        m_hoverRow = m_hoverHeader = m_resizeHot = -1;
    }
    void DataTable::PointerCanceled()
    {
        m_resizeColumn = -1; m_dragRow = -1; m_dropGap = -1; m_dragging = false;
        m_pressHeader = m_pressCheck = -1;
    }
    bool DataTable::MouseWheel(int delta)
    {
        if (!m_railShown) return false;
        m_wheel += delta;
        while (std::abs(m_wheel) >= WHEEL_DELTA)
        {
            const int direction = m_wheel > 0 ? 1 : -1;
            SetScroll(m_scrollY - direction * 3 * RowPx(), false);
            m_wheel -= direction * WHEEL_DELTA;
        }
        return true;
    }

    // ---------------------------------------------------------------- keyboard
    bool DataTable::KeyPressed(UINT key, bool alt)
    {
        if (m_editing) return false;
        const bool shift = Down(VK_SHIFT), ctrl = Down(VK_CONTROL);
        const int count = GetRowCount();
        const int page = (std::max)(1, BodyPx() / RowPx() - 1);
        if (alt)
        {
            if (!m_reorderable || (key != VK_UP && key != VK_DOWN)) return false;
            if (m_focus >= 0) MoveRow(m_focus, m_focus + (key == VK_UP ? -1 : 1));
            return true;
        }
        auto scrollX = [&](int delta)
        {
            const auto layout = CurrentLayout();
            const int most = (std::max)(0, Px(layout.total) - ViewportPx());
            const int next = (std::max)(0, (std::min)(most, m_scrollX + delta));
            if (next == m_scrollX) return;
            m_scrollX = next;
            Invalidate(FALSE);
        };
        switch (key)
        {
        case VK_UP: NavigateTo(m_focus < 0 ? 0 : m_focus - 1, shift, ctrl); return true;
        case VK_DOWN: NavigateTo(m_focus < 0 ? 0 : m_focus + 1, shift, ctrl); return true;
        case VK_PRIOR: NavigateTo(m_focus < 0 ? 0 : m_focus - page, shift, ctrl); return true;
        case VK_NEXT: NavigateTo(m_focus < 0 ? 0 : m_focus + page, shift, ctrl); return true;
        case VK_HOME: NavigateTo(0, shift, ctrl); return true;
        case VK_END: NavigateTo(count - 1, shift, ctrl); return true;
        case VK_LEFT: scrollX(-Px(40)); return true;
        case VK_RIGHT: scrollX(Px(40)); return true;
        case VK_SPACE:
            if (m_focus < 0) return true;
            if (m_checkable) ToggleChecked(m_focus);
            else if (m_multi && ctrl)
            {
                auto next = m_selected;
                if (!next.erase(m_focus)) next.insert(m_focus);
                Select(std::move(next), m_focus, m_focus, true);
            }
            return true;
        case VK_RETURN:
            if (m_focus >= 0) { m_activated = m_focus; Notify(TableActivated); }
            return true;
        case VK_F2:
            for (int c = 0; c < static_cast<int>(m_columns.size()) && m_focus >= 0; ++c)
                if (m_columns[c].editable) { BeginEdit(m_focus, c); break; }
            return true;
        case 'A':
            if (!ctrl) return false;
            if (m_multi) { SelectAll(); Notify(TableSelectionChanged); }
            return true;
        default: return false;
        }
    }
    bool DataTable::CharTyped(UINT ch)
    {
        if (m_editing || ch < 32 || ch == 127 || Down(VK_CONTROL)) return false;
        const ULONGLONG now = GetTickCount64();
        if (now - m_typedAt > TypeaheadMilliseconds) m_typed.Empty();
        m_typedAt = now;
        if (ch == L' ' && m_typed.IsEmpty()) return true;
        Typeahead(static_cast<wchar_t>(ch));
        return true;
    }
    // Selects the next row whose first column starts with the characters typed within the last second.
    void DataTable::Typeahead(wchar_t ch)
    {
        m_typed += ch;
        const int count = (std::min)(GetRowCount(), TypeaheadScanLimit);
        if (!count) return;
        const int start = m_typed.GetLength() == 1 ? m_focus + 1 : (std::max)(m_focus, 0);
        for (int i = 0; i < count; ++i)
        {
            const int row = (start + i) % count;
            const CString text = GetCellText(row, 0);
            if (text.GetLength() >= m_typed.GetLength() && _wcsnicmp(text, m_typed, m_typed.GetLength()) == 0)
            {
                NavigateTo(row, false, false);
                return;
            }
        }
    }

    // ------------------------------------------------------------------ messages
    void DataTable::Context(LPARAM l)
    {
        CPoint screen(GET_X_LPARAM(l), GET_Y_LPARAM(l));
        int row;
        if (l == -1)
        {
            row = m_focus;
            CPoint client(Px(24), row >= 0 ? RowRect(row).CenterPoint().y : HeaderPx());
            ClientToScreen(&client);
            screen = client;
        }
        else
        {
            CPoint client = screen;
            ScreenToClient(&client);
            row = HitTest(client).row;
        }
        m_context = row;
        if (GetParent()) GetParent()->SendMessage(WM_CONTEXTMENU, reinterpret_cast<WPARAM>(m_hWnd), MAKELPARAM(static_cast<short>(screen.x), static_cast<short>(screen.y)));
    }

    LRESULT DataTable::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        switch (message)
        {
        case WM_GETOBJECT:
            if (static_cast<LONG>(l) == UiaRootObjectId && m_table)
            {
                auto* provider = new TableProvider(m_table);
                const LRESULT result = UiaReturnRawElementProvider(m_hWnd, w, l, provider);
                provider->Release();
                return result;
            }
            break;
        case WM_DESTROY:
            if (m_table) { std::lock_guard<std::mutex> lock(m_table->mutex); m_table->window = nullptr; }
            break;
        case TableActionMessage:
            if (IsWindowEnabled()) ApplyAction(static_cast<TableActionId>(w), static_cast<int>(l));
            return 0;
        case ReleaseEditorMessage:
            ReleaseEditor();
            return 0;
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
            if (m_editor && reinterpret_cast<HWND>(l) == m_editor->GetSafeHwnd())
                return reinterpret_cast<LRESULT>(m_editor->Color(CDC::FromHandle(reinterpret_cast<HDC>(w))));
            break;
        case WM_RBUTTONDOWN:
            if (IsWindowEnabled())
            {
                SetFocus();
                const Hit hit = HitTest(CPoint(GET_X_LPARAM(l), GET_Y_LPARAM(l)));
                if (hit.row >= 0 && !m_selected.count(hit.row)) Select({ hit.row }, hit.row, hit.row, true);
            }
            break;
        case WM_CONTEXTMENU:
            if (IsWindowEnabled()) Context(l);
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(l) == HTCLIENT)
            {
                CPoint cursor;
                GetCursorPos(&cursor);
                ScreenToClient(&cursor);
                if (m_resizeColumn >= 0 || HitTest(cursor).zone == Zone::Divider) { SetCursor(LoadCursor(nullptr, IDC_SIZEWE)); return TRUE; }
            }
            break;
        case WM_MOUSEHWHEEL:
        {
            const int step = GET_WHEEL_DELTA_WPARAM(w) > 0 ? Px(40) : -Px(40);
            const auto layout = CurrentLayout();
            const int next = (std::max)(0, (std::min)((std::max)(0, Px(layout.total) - ViewportPx()), m_scrollX + step));
            if (next != m_scrollX) { if (m_editing) FinishEdit(true); m_scrollX = next; Invalidate(FALSE); }
            return 0;
        }
        default: break;
        }
        const LRESULT result = InputWindow::WindowProc(message, w, l);
        if (message == WM_SIZE) Relayout();
        if (message == WM_ENABLE)
        {
            if (m_rail.GetSafeHwnd()) m_rail.EnableWindow(w != 0);
            SyncAccess();
        }
        return result;
    }
}
