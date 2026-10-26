#pragma once
#include "InputControls.h"
#include "ScrollRail.h"
#include "TableVisuals.h"
#include "DataTable.Accessibility.h"
#include <functional>
#include <memory>
#include <set>

// Self-drawn Material 3 data table. One real child HWND painted with Direct2D (no SysListView32, no
// SysHeader32, no system scrollbar): a header row with sort and resizable columns, rows of text cells with
// an optional leading icon and color swatch, single or multiple selection, a virtual row source for large
// data, an in-place text editor, an empty state, a custom ScrollRail, DPI and light/dark support, and UI
// Automation Table, Grid and Selection patterns.
//
// Notifications reach the parent as WM_COMMAND with the control id and one of the codes below. Programmatic
// setters never notify. Right-click and Shift+F10 send WM_CONTEXTMENU to the parent (lParam = screen point,
// or -1 for the keyboard) after ContextRow() has been set.
namespace NativeMaterial
{
    constexpr UINT TableSelectionChanged = 0x4101;  // the selection set changed by user action
    constexpr UINT TableActivated = 0x4102;         // double click or Enter; ActivatedRow()
    constexpr UINT TableEditCommitted = 0x4103;     // EditedRow(), EditedColumn(), EditedText()
    constexpr UINT TableSortChanged = 0x4104;       // SortColumn(), SortOrder()
    constexpr UINT TableCheckChanged = 0x4105;      // CheckedRow() toggled by the user
    constexpr UINT TableReordered = 0x4106;         // ReorderedFrom(), ReorderedTo()
    constexpr UINT TableColumnResized = 0x4107;     // ResizedColumn()

    // Fills one cell of a virtual row. Called only for cells that are visible (or queried explicitly).
    using TableCellSource = std::function<void(int row, int column, TableCell& cell)>;

    struct TableRow
    {
        std::vector<TableCell> cells;
        LPARAM data{};
        CheckState check{ CheckState::Unchecked };
        int id{};                                   // stable identity across sorting and reordering
    };

    class TableCellEditor;

    class DataTable : public InputWindow
    {
    public:
        DataTable();
        ~DataTable() override;
        BOOL Create(const RECT& bounds, CWnd* parent, UINT id);
        void SetAccessibleName(const CString& name);

        // ---- Columns
        void SetColumns(const std::vector<TableColumn>& columns);
        int GetColumnCount() const { return static_cast<int>(m_columns.size()); }
        const TableColumn& Column(int index) const { return m_columns[index]; }
        void SetColumnWidth(int column, int widthDip);   // clamps to the minimum; does not notify
        void SetFillLastColumn(bool fill);

        // ---- Rows. Built-in rows are stored in the table; a virtual source holds no row data at all.
        int AddRow(const std::vector<TableCell>& cells, LPARAM data = 0);
        void SetRows(std::vector<TableRow> rows);
        bool RemoveRow(int row);
        void ClearRows();
        void SetVirtualRows(int count, TableCellSource source);
        void SetRowCount(int count);                      // virtual source only
        void Refresh();                                   // virtual source data changed
        bool IsVirtual() const { return m_virtual; }
        int GetRowCount() const { return m_virtual ? m_virtualCount : static_cast<int>(m_rows.size()); }
        CString GetCellText(int row, int column) const;
        bool SetCellText(int row, int column, const CString& text);   // built-in rows only
        LPARAM GetRowData(int row) const;
        int RowIdentity(int row) const;                   // built-in rows only, -1 otherwise
        void SetEmptyText(const CString& text);

        // ---- Selection and focus
        void SetMultiSelect(bool multi);
        bool IsMultiSelect() const { return m_multi; }
        int GetCurrentRow() const { return m_focus; }     // the keyboard focus row
        int GetSelectedRow() const { return m_selected.empty() ? -1 : *m_selected.begin(); }
        std::vector<int> GetSelectedRows() const { return std::vector<int>(m_selected.begin(), m_selected.end()); }
        bool IsRowSelected(int row) const { return m_selected.count(row) != 0; }
        void SelectRow(int row);                          // selects only this row and scrolls to it
        void SetSelectedRows(const std::vector<int>& rows);
        void SelectAll();
        void ClearSelection();
        void EnsureVisible(int row);
        int ActivatedRow() const { return m_activated; }
        int ContextRow() const { return m_context; }

        // ---- Sorting. Built-in rows are sorted by the table; a virtual source is told through TableSortChanged.
        void SetSort(int column, SortDirection direction); // programmatic; does not notify
        int SortColumn() const { return m_sortColumn; }
        SortDirection SortOrder() const { return m_sortDirection; }
        void SetAutoSort(bool automatic) { m_autoSort = automatic; }

        // ---- Check boxes (built-in rows) and reordering (built-in rows)
        void SetCheckable(bool checkable);
        bool IsCheckable() const { return m_checkable; }
        bool GetChecked(int row) const;
        void SetChecked(int row, bool checked);           // does not notify
        bool ToggleChecked(int row);                      // notifies like a click
        int CheckedRow() const { return m_checkedRow; }
        void SetReorderable(bool reorderable) { m_reorderable = reorderable; }
        bool IsReorderable() const { return m_reorderable; }
        bool MoveRow(int from, int to);                   // notifies TableReordered
        int ReorderedFrom() const { return m_reorderFrom; }
        int ReorderedTo() const { return m_reorderTo; }

        // ---- In-place editing of a text cell
        bool BeginEdit(int row, int column);              // the column must be editable
        bool CommitEdit();
        void CancelEdit();
        bool IsEditing() const { return m_editing; }
        HWND EditorWindow() const;
        int EditedRow() const { return m_editedRow; }
        int EditedColumn() const { return m_editedColumn; }
        CString EditedText() const { return m_editedText; }
        int ResizedColumn() const { return m_resizedColumn; }

        // ---- Scrolling and geometry
        int FirstVisibleRow() const;
        int LastVisibleRow() const;
        int ScrollOffset() const { return m_scrollY; }    // physical pixels
        void ScrollTo(int pixels);
        bool ScrollbarVisible() const { return m_railShown; }
        ScrollRail& Rail() { return m_rail; }
        CRect RowRect(int row) const;                     // client pixels, ignoring the clip
        CRect CellRect(int row, int column) const;
        CRect HeaderRect(int column) const;
        CRect CheckRect(int row) const;
        // Number of cell fetches made from the row source or storage since the last reset (virtualization diagnostic).
        int FetchCount() const { return m_fetches; }
        void ResetFetchCount() { m_fetches = 0; }
        // Keyboard modifier source; defaults to the thread key state. Automation and the verification replace it.
        void SetModifierSource(std::function<bool(int virtualKey)> source) { m_modifier = std::move(source); }
        TablePaintModel BuildModel() const;
        // A UI Automation provider for the table root (the caller releases it). Used by the verification to read
        // the Table, Grid and Selection patterns without a client round trip through the message loop.
        HRESULT TableAutomation(IRawElementProviderSimple** provider) const;
    protected:
        BOOL CreateTable(InputKind kind, const RECT& bounds, CWnd* parent, UINT id);
        void SetListMode(bool list);
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override { return CSize(240, m_listMode ? 96 : 144); }
        void FillAccess(InputData&) const override {}
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        void PointerCanceled() override;
        bool KeyPressed(UINT, bool) override;
        bool CharTyped(UINT) override;
        bool MouseWheel(int) override;
        bool WantsKey(UINT key) const override { return key == VK_RETURN || key == VK_ESCAPE || key == VK_F2; }
        UINT DialogCode() const override { return DLGC_WANTARROWS | DLGC_WANTCHARS; }
        void DpiChanged() override;
        void AppearanceChanged() override;
        bool IsFocused() const override;
        void PerformAction(InputActionId, int) override {}
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        friend class TableCellEditor;
        enum class Zone { None, Header, Divider, Body, Check };
        struct Hit { Zone zone{ Zone::None }; int row{ -1 }, column{ -1 }; };

        Hit HitTest(CPoint) const;
        bool Down(int virtualKey) const;
        int RowPx() const;
        int HeaderPx() const;
        int BodyPx() const;
        int ViewportPx() const;
        TableLayout CurrentLayout() const;
        void FetchCell(int row, int column, TableCell& cell) const;
        void Relayout();
        void UpdateScroll();
        void SetScroll(int pixels, bool fromRail);
        void SyncAccess();
        void Select(std::set<int> rows, int focus, int anchor, bool notify);
        void NavigateTo(int row, bool shift, bool ctrl);
        void SortRows();
        void ToggleSort(int column);
        void FinishEdit(bool commit);
        void RepositionEditor();
        void ReleaseEditor();
        void ReleaseEditorLater();
        void EditorLostFocus();
        void ApplyAction(TableActionId, int);
        int RemapRow(int row, int from, int to) const;
        void Context(LPARAM);
        void Typeahead(wchar_t);

        std::vector<TableColumn> m_columns;
        std::vector<TableRow> m_rows;
        TableCellSource m_source;
        bool m_virtual{}, m_listMode{}, m_multi{}, m_checkable{}, m_reorderable{}, m_autoSort{ true }, m_fillLast{ true };
        int m_virtualCount{}, m_nextId{ 1 };
        std::set<int> m_selected;
        int m_focus{ -1 }, m_anchor{ -1 }, m_activated{ -1 }, m_context{ -1 }, m_checkedRow{ -1 };
        int m_scrollY{}, m_scrollX{};
        int m_sortColumn{ -1 };
        SortDirection m_sortDirection{ SortDirection::None };
        int m_hoverRow{ -1 }, m_hoverHeader{ -1 }, m_resizeHot{ -1 }, m_resizedColumn{ -1 };
        // pointer interaction
        int m_pressRow{ -1 }, m_pressHeader{ -1 }, m_pressCheck{ -1 }, m_resizeColumn{ -1 }, m_resizeStartWidth{}, m_dragRow{ -1 }, m_dropGap{ -1 };
        bool m_dragging{};
        CPoint m_pressPoint;
        ULONGLONG m_lastClickTime{};
        int m_lastClickRow{ -1 };
        CPoint m_lastClickPoint;
        int m_reorderFrom{ -1 }, m_reorderTo{ -1 };
        // editing
        std::unique_ptr<TableCellEditor> m_editor;
        bool m_editing{}, m_editorPendingRelease{};
        int m_editRow{ -1 }, m_editColumn{ -1 }, m_editedRow{ -1 }, m_editedColumn{ -1 };
        CString m_editOriginal, m_editedText;
        // misc
        CString m_emptyText, m_name, m_typed;
        ULONGLONG m_typedAt{};
        int m_wheel{};
        mutable int m_fetches{};
        std::function<bool(int)> m_modifier;
        CFont m_font;
        ScrollRail m_rail;
        bool m_railShown{};
        std::shared_ptr<TableAccess> m_table;
    };
}
