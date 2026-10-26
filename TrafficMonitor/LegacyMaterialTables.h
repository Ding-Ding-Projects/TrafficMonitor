#pragma once
#include <memory>
#include "NativeMaterial/DataTable.h"
#include "NativeMaterial/MaterialList.h"

// Helpers of the legacy dialogs that replace a stock list control (list view, list box, check list box)
// by a Material data table or list. The helpers only use the public API of CWnd and of the Material
// components, so a dialog keeps the replacement as an ordinary member and applies the appearance itself
// from CBaseDialog::OnMaterialAppearanceChanged.
namespace LegacyMaterial
{
    // Replaces the stock control `id` of `dialog` by a new Material table (NativeMaterial::DataTable or
    // NativeMaterial::MaterialList). The new window takes the rectangle, the control ID, the z-order slot
    // (so the tab order is kept) and the enabled state of the stock control, which is destroyed.
    // Returns null when the stock control does not exist or the table cannot be created.
    template <class Table>
    std::unique_ptr<Table> ReplaceWithTable(CWnd& dialog, UINT id, NativeMaterial::Appearance appearance, COLORREF surface, const CString& accessible_name)
    {
        CWnd* stock = dialog.GetDlgItem(id);
        if (stock == nullptr)
            return nullptr;
        CRect rect;
        stock->GetWindowRect(rect);
        dialog.ScreenToClient(rect);
        const HWND previous = ::GetWindow(stock->GetSafeHwnd(), GW_HWNDPREV);
        const bool enabled = (stock->GetStyle() & WS_DISABLED) == 0;
        stock->DestroyWindow();

        auto table = std::make_unique<Table>();
        if (!table->Create(rect, &dialog, id))
            return nullptr;
        table->SetAccessibleName(accessible_name);
        table->SetWindowPos(previous != nullptr ? CWnd::FromHandle(previous) : &CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        table->SetAppearance(appearance);
        table->SetSurfaceColor(surface);
        table->EnableWindow(enabled);
        return table;
    }

    // Column of `row` that holds the screen point (the last column when the point is right of all columns).
    inline int ColumnAtScreenPoint(const NativeMaterial::DataTable& table, int row, CPoint screen)
    {
        const int count = table.GetColumnCount();
        if (row < 0 || count <= 0)
            return -1;
        CPoint client = screen;
        ::ScreenToClient(table.GetSafeHwnd(), &client);
        for (int column = 0; column < count; ++column)
        {
            const CRect cell = table.CellRect(row, column);
            if (client.x >= cell.left && client.x < cell.right)
                return column;
        }
        return count - 1;
    }
}
