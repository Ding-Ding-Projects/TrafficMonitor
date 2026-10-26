#pragma once
#include "DataTable.h"

// Self-drawn Material 3 list: a single column of 48 dip items without a header, built on the data table
// so it shares its painting, scrolling, keyboard model and UI Automation (a List with ListItem children).
//
//   Single-select list (skin chooser): SetCurSel / GetCurSel; selection changes arrive as TableSelectionChanged
//   (CBN_SELCHANGE / LBN_SELCHANGE equivalent) and Enter or a double click as TableActivated.
//   Checkable list with reorder (connection selection, item order): SetCheckable, SetReorderable. A click on the
//   check box or Space toggles an item (TableCheckChanged, CheckedRow()); Alt+Up / Alt+Down or dragging an item
//   moves it (TableReordered, ReorderedFrom(), ReorderedTo()).
namespace NativeMaterial
{
    class MaterialList final : public DataTable
    {
    public:
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        int AddItem(const CString& text, HICON icon = nullptr, LPARAM data = 0, bool checked = false);
        void SetItems(const std::vector<CString>& items);
        void ResetContent() { ClearRows(); }
        bool DeleteItem(int index) { return RemoveRow(index); }
        int GetCount() const { return GetRowCount(); }
        CString GetItemText(int index) const { return GetCellText(index, 0); }
        void SetItemText(int index, const CString& text) { SetCellText(index, 0, text); }
        LPARAM GetItemData(int index) const { return GetRowData(index); }
        void SetCurSel(int index) { SelectRow(index); }                  // does not notify
        int GetCurSel() const { return GetSelectedRow(); }
        bool GetItemChecked(int index) const { return GetChecked(index); }
        void SetItemChecked(int index, bool checked) { SetChecked(index, checked); }   // does not notify
        bool MoveItem(int from, int to) { return MoveRow(from, to); }
        bool MoveSelected(int delta) { const int row = GetCurrentRow(); return row >= 0 && MoveRow(row, row + delta); }
    };
}
