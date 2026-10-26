#pragma once
#include "InputVisuals.h"
#include <afxwin.h>
#include <vector>

// Direct2D drawing and geometry for the Material 3 data table and list. Like InputVisuals, every
// coordinate is in 96-dpi device-independent pixels and the same function paints the real window and
// the offscreen verification renders.
namespace NativeMaterial
{
    namespace TableMetrics
    {
        constexpr float RowHeight = 48.f;       // also the minimum interactive target
        constexpr float HeaderHeight = 48.f;
        constexpr float CellPadding = 16.f;
        constexpr float CheckColumn = 48.f;     // leading checkbox column of a checkable table or list
        constexpr float IconSize = 20.f, SwatchSize = 16.f, ContentGap = 8.f;
        constexpr float ResizeGrip = 6.f;       // half-width of the draggable column divider
        constexpr float RailWidth = 12.f;
        constexpr float MinimumColumn = 40.f;
    }

    enum class ColumnAlign { Left, Center, Right };
    enum class SortDirection { None, Ascending, Descending };

    struct TableColumn
    {
        CString title;
        int width{ 160 };                       // DIP
        ColumnAlign align{ ColumnAlign::Left };
        bool sortable{}, resizable{ true }, editable{};
    };

    // One cell: text plus an optional leading icon (not owned) and an optional color swatch.
    // A cell with hasBar draws a proportional bar (barFraction 0..1 of the free cell width, barColor) instead of its
    // text; the text stays the accessible and typeahead value, so it should describe the bar (for example the total).
    struct TableCell
    {
        CString text;
        HICON icon{};
        bool hasSwatch{};
        COLORREF swatch{};
        bool hasBar{};
        float barFraction{};
        COLORREF barColor{};
    };

    struct TableLayout
    {
        std::vector<float> x, width;            // per column, from the left edge of the content
        float checkWidth{}, total{};
    };
    // The last column grows to fill the viewport when fillLast is set and the columns are narrower.
    TableLayout LayoutColumns(const std::vector<TableColumn>& columns, float viewportWidth, bool checkable, bool fillLast);

    struct TablePaintRow
    {
        int index{};
        std::vector<TableCell> cells;
        bool selected{}, focused{}, hover{};
        CheckState check{ CheckState::Unchecked };
    };

    struct TablePaintModel
    {
        std::vector<TableColumn> columns;
        std::vector<TablePaintRow> rows;        // only the rows that are visible
        int rowCount{};
        float viewportWidth{};                  // DIP, excluding the scroll rail
        float scrollY{}, scrollX{};             // DIP
        bool showHeader{ true }, checkable{}, fillLast{ true };
        int sortColumn{ -1 };
        SortDirection sortDirection{ SortDirection::None };
        int hoverHeader{ -1 }, resizeHot{ -1 };
        int editRow{ -1 }, editColumn{ -1 };    // the cell covered by the in-place editor
        int dropGap{ -1 };                      // insertion line between rows while reordering
        CString emptyText;
    };

    void DrawDataTable(ID2D1RenderTarget*, const Palette&, const TablePaintModel&, const InputState&);
}
