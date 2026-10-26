#include "../stdafx.h"
#include "MaterialList.h"

namespace NativeMaterial
{
    BOOL MaterialList::Create(const RECT& bounds, CWnd* parent, UINT id)
    {
        if (!CreateTable(InputKind::List, bounds, parent, id)) return FALSE;
        TableColumn column;
        column.width = 1;                   // the single column always fills the viewport
        SetListMode(true);
        SetColumns({ column });
        return TRUE;
    }

    int MaterialList::AddItem(const CString& text, HICON icon, LPARAM data, bool checked)
    {
        TableCell cell;
        cell.text = text;
        cell.icon = icon;
        const int row = AddRow({ cell }, data);
        if (checked && row >= 0) SetChecked(row, true);
        return row;
    }

    void MaterialList::SetItems(const std::vector<CString>& items)
    {
        std::vector<TableRow> rows;
        for (const auto& item : items)
        {
            TableRow row;
            TableCell cell;
            cell.text = item;
            row.cells.push_back(cell);
            rows.push_back(std::move(row));
        }
        SetRows(std::move(rows));
    }
}
