// Hidden-window verification of the Material 3 data table and list. Each component is created in a real,
// never-shown parent and driven with window messages. Nothing here needs a visible desktop.
#include "../stdafx.h"
#include "MaterialList.h"
#include "../NativePreferences/Text.h"
#include "../NativeMenus/SearchableMenu.h"
#include <cstdio>
#include <functional>
#include <set>
#include <wrl/client.h>

// The focused test links only the table sources, so the two application services they reach are stubbed.
namespace NativePreferences { Snapshot Current() { return Snapshot{}; } }
namespace NativeMenus { UINT Track(CMenu*, UINT, int, int, CWnd*, NativeRegex::Options*) { return 0; } }

namespace
{
    using Microsoft::WRL::ComPtr;
    using namespace NativeMaterial;
    CWinApp application;
    int failures{};
    void Check(bool value, const char* name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name); if (!value) ++failures; }

    // Records every WM_COMMAND and WM_CONTEXTMENU a component sends to its parent.
    class Host : public CWnd
    {
    public:
        std::vector<std::pair<UINT, UINT>> log;
        int contextMenus{};
        int Count(UINT id, UINT code) const
        {
            int n = 0;
            for (const auto& entry : log) if (entry.first == id && entry.second == code) ++n;
            return n;
        }
    protected:
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
        {
            if (message == WM_COMMAND) log.emplace_back(LOWORD(w), HIWORD(w));
            if (message == WM_CONTEXTMENU) { ++contextMenus; return 0; }
            return CWnd::WindowProc(message, w, l);
        }
    };

    void Pump()
    {
        MSG message;
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&message); DispatchMessage(&message); }
    }
    LPARAM Point(int x, int y) { return MAKELPARAM(static_cast<short>(x), static_cast<short>(y)); }
    void ClickAt(HWND window, CPoint p)
    {
        SendMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, Point(p.x, p.y));
        SendMessage(window, WM_LBUTTONUP, 0, Point(p.x, p.y));
    }
    void Key(HWND window, UINT key) { SendMessage(window, WM_KEYDOWN, key, 0); SendMessage(window, WM_KEYUP, key, 0); }
    void AltKey(HWND window, UINT key) { SendMessage(window, WM_SYSKEYDOWN, key, 0); SendMessage(window, WM_SYSKEYUP, key, 0); }
    void Chars(HWND window, const wchar_t* text) { for (; *text; ++text) SendMessage(window, WM_CHAR, *text, 0); }
    // Typeahead forgets what was typed after one second.
    DWORD TypeaheadWait() { return 1100; }
    CString ClassOf(HWND window) { wchar_t name[128]{}; GetClassNameW(window, name, 128); return name; }

    // Modifier keys are injected, never read from the real keyboard.
    bool g_shift{}, g_ctrl{};
    bool Modifier(int key) { return (key == VK_SHIFT && g_shift) || (key == VK_CONTROL && g_ctrl); }

    CPoint RowPoint(const DataTable& t, int row) { return t.RowRect(row).CenterPoint(); }
    CPoint CellPoint(const DataTable& t, int row, int column) { return t.CellRect(row, column).CenterPoint(); }
    void ClickRow(DataTable& t, int row) { ClickAt(t.GetSafeHwnd(), RowPoint(t, row)); }

    // A generic system control may not appear anywhere below a component. Edit is only allowed for the in-place editor.
    int StockChildren(HWND root, bool allowEditor, CString* found = nullptr)
    {
        struct Context { int bad{}; bool editor{}; CString name; } context{ 0, allowEditor, CString() };
        EnumChildWindows(root, [](HWND child, LPARAM l) -> BOOL
        {
            auto* c = reinterpret_cast<Context*>(l);
            wchar_t name[128]{};
            GetClassNameW(child, name, 128);
            for (const wchar_t* stock : { L"SysListView32", L"ListBox", L"SysHeader32", L"ScrollBar", L"Button", L"ComboBox", L"ComboLBox", L"msctls_updown32", L"msctls_trackbar32", L"SysTabControl32", L"Static" })
                if (_wcsicmp(name, stock) == 0) { ++c->bad; c->name = name; }
            if (_wcsicmp(name, L"Edit") == 0 && !c->editor) { ++c->bad; c->name = name; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&context));
        if (found) *found = context.name;
        return context.bad;
    }

    // ----------------------------------------------------------- offscreen renders
    struct Picture { std::vector<DWORD> pixels; D2D1_COLOR_F surface{}; };
    Picture Render(UINT dpi, int widthDip, int heightDip, Appearance appearance, const std::function<void(ID2D1RenderTarget*, const Palette&)>& draw)
    {
        const int w = MulDiv(widthDip, dpi, 96), h = MulDiv(heightDip, dpi, 96);
        HDC dc = CreateCompatibleDC(nullptr);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
        void* bits{};
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        auto previous = SelectObject(dc, bitmap);
        ComPtr<ID2D1Factory> factory;
        ComPtr<ID2D1DCRenderTarget> target;
        HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
        const auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), static_cast<float>(dpi), static_cast<float>(dpi));
        if (SUCCEEDED(result)) result = factory->CreateDCRenderTarget(&properties, target.GetAddressOf());
        RECT bounds{ 0, 0, w, h };
        if (SUCCEEDED(result)) result = target->BindDC(dc, &bounds);
        const auto palette = Palette::Resolve(appearance);
        if (SUCCEEDED(result))
        {
            target->BeginDraw();
            target->Clear(palette.surface);
            draw(target.Get(), palette);
            result = target->EndDraw();
        }
        if (FAILED(result)) Check(false, "real Direct2D render succeeds");
        GdiFlush();
        Picture picture;
        picture.surface = palette.surface;
        picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(w) * h);
        target.Reset(); factory.Reset();
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return picture;
    }
    bool NearSurface(DWORD pixel, D2D1_COLOR_F s)
    {
        auto within = [](int a, float b) { return std::abs(a - static_cast<int>(b * 255 + .5f)) <= 2; };
        return within((pixel >> 16) & 255, s.r) && within((pixel >> 8) & 255, s.g) && within(pixel & 255, s.b);
    }
    size_t Ink(const Picture& p) { size_t n = 0; for (DWORD px : p.pixels) if (!NearSurface(px, p.surface)) ++n; return n; }
    size_t Difference(const Picture& a, const Picture& b)
    {
        if (a.pixels.size() != b.pixels.size()) return 1u << 30;
        size_t n = 0;
        for (size_t i = 0; i < a.pixels.size(); ++i) if (a.pixels[i] != b.pixels[i]) ++n;
        return n;
    }
    // Paints a real window through its WM_PRINTCLIENT route into a DIB.
    size_t WindowInk(InputWindow& window)
    {
        CRect client;
        window.GetClientRect(client);
        HDC dc = CreateCompatibleDC(nullptr);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = client.Width(); info.bmiHeader.biHeight = -client.Height();
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
        void* bits{};
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        auto previous = SelectObject(dc, bitmap);
        const HRESULT result = window.PaintTo(dc, true);
        GdiFlush();
        Picture picture;
        picture.surface = Palette::Resolve(window.GetAppearance()).surface;
        picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(client.Width()) * client.Height());
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return SUCCEEDED(result) ? Ink(picture) : 0;
    }

    // ------------------------------------------------------------ automation helpers
    template<class T> ComPtr<T> Pattern(IRawElementProviderSimple* provider, PATTERNID id)
    {
        ComPtr<T> pattern;
        ComPtr<IUnknown> unknown;
        if (provider && SUCCEEDED(provider->GetPatternProvider(id, unknown.GetAddressOf())) && unknown)
            unknown.As(&pattern);
        return pattern;
    }
    CString Name(IRawElementProviderSimple* provider)
    {
        VARIANT v{};
        CString name;
        if (SUCCEEDED(provider->GetPropertyValue(UIA_NamePropertyId, &v)) && v.vt == VT_BSTR) name = v.bstrVal;
        VariantClear(&v);
        return name;
    }
    long ControlType(IRawElementProviderSimple* provider)
    {
        VARIANT v{};
        long type = 0;
        if (SUCCEEDED(provider->GetPropertyValue(UIA_ControlTypePropertyId, &v)) && v.vt == VT_I4) type = v.lVal;
        return type;
    }
    ComPtr<IRawElementProviderSimple> Root(DataTable& table)
    {
        ComPtr<IRawElementProviderSimple> provider;
        table.TableAutomation(provider.GetAddressOf());
        return provider;
    }
    // Element i of a SAFEARRAY of providers.
    ComPtr<IRawElementProviderSimple> ElementOf(SAFEARRAY* array, LONG index)
    {
        ComPtr<IUnknown> unknown;
        ComPtr<IRawElementProviderSimple> provider;
        if (array && SUCCEEDED(SafeArrayGetElement(array, &index, unknown.GetAddressOf())) && unknown) unknown.As(&provider);
        return provider;
    }
    long ArrayLength(SAFEARRAY* array)
    {
        LONG lower = 0, upper = -1;
        if (array) { SafeArrayGetLBound(array, 1, &lower); SafeArrayGetUBound(array, 1, &upper); }
        return upper - lower + 1;
    }

    std::vector<TableColumn> NameSizeColor()
    {
        TableColumn name; name.title = L"Name"; name.width = 200; name.sortable = true; name.editable = true;
        TableColumn size; size.title = L"Size"; size.width = 100; size.sortable = true; size.align = ColumnAlign::Right;
        TableColumn color; color.title = L"Color"; color.width = 120;
        return { name, size, color };
    }
    TableRow MakeRow(const wchar_t* name, const wchar_t* size, COLORREF swatch, LPARAM data)
    {
        TableRow row;
        TableCell a; a.text = name;
        TableCell b; b.text = size;
        TableCell c; c.text = L"swatch"; c.hasSwatch = true; c.swatch = swatch;
        row.cells = { a, b, c };
        row.data = data;
        return row;
    }
}

int wmain()
{
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Host host;
    Check(host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Material table verification", WS_POPUP, 0, 0, 1000, 1000, nullptr, 0) != FALSE, "hidden native parent created");

    // ----------------------------------------------------------------- DataTable basics
    DataTable table;
    Check(table.Create(CRect(10, 10, 530, 490), &host, 201) != FALSE, "data table created");
    table.SetAccessibleName(L"Files");
    table.SetColumns(NameSizeColor());
    table.SetEmptyText(L"No files");
    table.SetModifierSource(Modifier);
    Check(table.GetColumnCount() == 3 && table.GetRowCount() == 0, "table starts with its columns and no rows");
    {
        const auto model = table.BuildModel();
        Check(model.rowCount == 0 && model.emptyText == L"No files" && model.rows.empty(), "empty table reports its empty state text");
    }
    table.SetRows({ MakeRow(L"Delta", L"40", RGB(200, 30, 30), 1), MakeRow(L"alpha", L"5", RGB(30, 160, 60), 2), MakeRow(L"Charlie", L"300", RGB(30, 60, 200), 3),
                    MakeRow(L"Bravo", L"20", RGB(200, 160, 0), 4), MakeRow(L"echo", L"1", RGB(120, 0, 160), 5) });
    host.log.clear();
    Check(table.GetRowCount() == 5 && table.GetCellText(2, 0) == L"Charlie" && table.GetRowData(3) == 4, "rows are stored with text and row data");
    Check(!table.ScrollbarVisible(), "five rows need no scroll rail");

    // ---- pointer selection
    ClickRow(table, 2);
    Check(table.GetSelectedRow() == 2 && table.GetCurrentRow() == 2 && host.Count(201, TableSelectionChanged) == 1, "click selects the row and notifies once");
    ClickRow(table, 3);
    Check(table.GetSelectedRow() == 3 && host.Count(201, TableSelectionChanged) == 2 && !table.IsRowSelected(2), "clicking another row moves the single selection");
    table.SelectRow(1);
    Check(table.GetSelectedRow() == 1 && host.Count(201, TableSelectionChanged) == 2, "programmatic selection does not notify");
    ClickAt(table.GetSafeHwnd(), CPoint(table.RowRect(4).left + 5, table.RowRect(4).bottom + 120));
    Check(table.GetSelectedRow() == 1, "a click below the last row keeps the selection");

    // ---- keyboard
    table.SelectRow(0);
    host.log.clear();
    Key(table.GetSafeHwnd(), VK_DOWN);
    Check(table.GetSelectedRow() == 1 && host.Count(201, TableSelectionChanged) == 1, "Down moves the selection and notifies");
    Key(table.GetSafeHwnd(), VK_END);
    Check(table.GetSelectedRow() == 4, "End selects the last row");
    Key(table.GetSafeHwnd(), VK_DOWN);
    Check(table.GetSelectedRow() == 4, "Down at the last row stays there");
    Key(table.GetSafeHwnd(), VK_HOME);
    Check(table.GetSelectedRow() == 0, "Home selects the first row");
    Key(table.GetSafeHwnd(), VK_UP);
    Check(table.GetSelectedRow() == 0, "Up at the first row stays there");
    Key(table.GetSafeHwnd(), VK_NEXT);
    Check(table.GetSelectedRow() == 4, "PageDown moves by a page and clamps");
    Key(table.GetSafeHwnd(), VK_PRIOR);
    Check(table.GetSelectedRow() == 0, "PageUp moves by a page and clamps");
    host.log.clear();
    Key(table.GetSafeHwnd(), VK_RETURN);
    Check(table.ActivatedRow() == 0 && host.Count(201, TableActivated) == 1, "Enter activates the focused row");

    // ---- double click, then a click after the interval is only a selection
    ClickRow(table, 1);
    host.log.clear();
    ClickRow(table, 1);
    Check(table.ActivatedRow() == 1 && host.Count(201, TableActivated) == 1, "a second click on the same row activates it");
    ClickRow(table, 1);
    Sleep(GetDoubleClickTime() + 60);
    host.log.clear();
    ClickRow(table, 1);
    Check(host.Count(201, TableActivated) == 0, "clicks further apart than the double-click time do not activate");

    // ---- typeahead on the first column
    table.SelectRow(0);
    Chars(table.GetSafeHwnd(), L"c");
    Check(table.GetSelectedRow() == 2, "typing a letter selects the next row that starts with it");
    Sleep(TypeaheadWait());
    Chars(table.GetSafeHwnd(), L"B");
    Check(table.GetSelectedRow() == 3, "typeahead ignores case");
    Chars(table.GetSafeHwnd(), L"x");
    Check(table.GetSelectedRow() == 3, "a non matching prefix keeps the selection");

    // ---- multiple selection with injected modifiers
    table.SetMultiSelect(true);
    table.SelectRow(1);
    host.log.clear();
    g_shift = true;
    Key(table.GetSafeHwnd(), VK_DOWN); Key(table.GetSafeHwnd(), VK_DOWN);
    g_shift = false;
    Check(table.GetSelectedRows() == std::vector<int>({ 1, 2, 3 }), "Shift+Down extends the selection from the anchor");
    g_ctrl = true;
    Key(table.GetSafeHwnd(), VK_UP); Key(table.GetSafeHwnd(), VK_UP);
    Check(table.GetSelectedRows() == std::vector<int>({ 1, 2, 3 }) && table.GetCurrentRow() == 1, "Ctrl+Up moves only the focus");
    Key(table.GetSafeHwnd(), VK_SPACE);
    Check(table.GetSelectedRows() == std::vector<int>({ 2, 3 }), "Ctrl+Space toggles the focused row");
    Key(table.GetSafeHwnd(), 'A');
    g_ctrl = false;
    Check(table.GetSelectedRows() == std::vector<int>({ 0, 1, 2, 3, 4 }), "Ctrl+A selects every row");
    g_ctrl = true;
    ClickRow(table, 2);
    g_ctrl = false;
    Check(!table.IsRowSelected(2) && table.GetSelectedRows().size() == 4, "Ctrl+click toggles a row");
    table.SelectRow(1);
    g_shift = true;
    ClickRow(table, 3);
    g_shift = false;
    Check(table.GetSelectedRows() == std::vector<int>({ 1, 2, 3 }), "Shift+click selects the range from the anchor");
    ClickRow(table, 0);
    Check(table.GetSelectedRows() == std::vector<int>({ 0 }), "a plain click replaces a multiple selection");
    table.SetMultiSelect(false);

    // ---- sorting (built-in rows)
    table.SelectRow(3);                                  // Bravo
    const int bravoId = table.RowIdentity(3);
    host.log.clear();
    ClickAt(table.GetSafeHwnd(), table.HeaderRect(0).CenterPoint());
    Check(table.SortColumn() == 0 && table.SortOrder() == SortDirection::Ascending && host.Count(201, TableSortChanged) == 1, "header click sorts ascending and notifies");
    Check(table.GetCellText(0, 0) == L"alpha" && table.GetCellText(1, 0) == L"Bravo" && table.GetCellText(4, 0) == L"echo", "rows are ordered case-insensitively");
    Check(table.GetSelectedRow() >= 0 && table.RowIdentity(table.GetSelectedRow()) == bravoId, "the selected row follows its data through the sort");
    ClickAt(table.GetSafeHwnd(), table.HeaderRect(0).CenterPoint());
    Check(table.SortOrder() == SortDirection::Descending && table.GetCellText(0, 0) == L"echo" && host.Count(201, TableSortChanged) == 2, "a second header click sorts descending");
    ClickAt(table.GetSafeHwnd(), table.HeaderRect(1).CenterPoint());
    Check(table.SortColumn() == 1 && table.GetCellText(0, 1) == L"1" && table.GetCellText(4, 1) == L"300", "numeric columns sort by number, not by text");
    host.log.clear();
    ClickAt(table.GetSafeHwnd(), table.HeaderRect(2).CenterPoint());
    Check(table.SortColumn() == 1 && host.Count(201, TableSortChanged) == 0, "a header that is not sortable does nothing");
    {
        const auto asc = table.BuildModel();
        Check(asc.sortColumn == 1 && asc.sortDirection == SortDirection::Ascending, "the paint model carries the sort indicator");
    }

    // ---- column resize
    const int widthBefore = table.Column(0).width;
    host.log.clear();
    {
        const CRect header = table.HeaderRect(0);
        SendMessage(table.GetSafeHwnd(), WM_LBUTTONDOWN, MK_LBUTTON, Point(header.right, header.CenterPoint().y));
        SendMessage(table.GetSafeHwnd(), WM_MOUSEMOVE, MK_LBUTTON, Point(header.right + 60, header.CenterPoint().y));
        SendMessage(table.GetSafeHwnd(), WM_LBUTTONUP, 0, Point(header.right + 60, header.CenterPoint().y));
    }
    Check(table.Column(0).width > widthBefore + 20 && host.Count(201, TableColumnResized) == 1 && table.ResizedColumn() == 0 && table.SortColumn() == 1, "dragging the column divider resizes it without sorting");
    {
        const CRect header = table.HeaderRect(0);
        SendMessage(table.GetSafeHwnd(), WM_LBUTTONDOWN, MK_LBUTTON, Point(header.right, header.CenterPoint().y));
        SendMessage(table.GetSafeHwnd(), WM_MOUSEMOVE, MK_LBUTTON, Point(header.left - 400, header.CenterPoint().y));
        SendMessage(table.GetSafeHwnd(), WM_LBUTTONUP, 0, Point(header.left - 400, header.CenterPoint().y));
    }
    Check(table.Column(0).width >= static_cast<int>(TableMetrics::MinimumColumn), "a column cannot be resized below its minimum");
    table.SetColumnWidth(0, 200);

    // ---- in-place edit
    table.SetSort(0, SortDirection::None);
    table.SetRows({ MakeRow(L"Delta", L"40", RGB(200, 30, 30), 1), MakeRow(L"alpha", L"5", RGB(30, 160, 60), 2), MakeRow(L"Charlie", L"300", RGB(30, 60, 200), 3) });
    host.log.clear();
    Check(!table.BeginEdit(0, 1), "a column that is not editable cannot be edited");
    Check(StockChildren(table.GetSafeHwnd(), false) == 0, "no Edit window exists before editing");
    Check(table.BeginEdit(1, 0) && table.IsEditing() && table.EditorWindow() != nullptr, "BeginEdit opens the editor over the cell");
    {
        const HWND editor = table.EditorWindow();
        Check(ClassOf(editor) == L"Edit" && StockChildren(table.GetSafeHwnd(), true) == 0, "the in-place editor is the only Edit and nothing else is a stock control");
        CRect cell = table.CellRect(1, 0), editRect;
        ::GetWindowRect(editor, editRect);
        CPoint corner = editRect.TopLeft();
        ::ScreenToClient(table.GetSafeHwnd(), &corner);
        Check(corner.x >= cell.left && corner.y >= cell.top && corner.y < cell.bottom && editRect.Width() <= cell.Width(), "the editor sits inside the edited cell");
        ::SetWindowTextW(editor, L"omega");
        SendMessage(editor, WM_KEYDOWN, VK_RETURN, 0);
    }
    Check(!table.IsEditing() && table.GetCellText(1, 0) == L"omega" && host.Count(201, TableEditCommitted) == 1
        && table.EditedRow() == 1 && table.EditedColumn() == 0 && table.EditedText() == L"omega", "Enter commits the edit, updates the cell and notifies once");
    Pump();
    Check(StockChildren(table.GetSafeHwnd(), false) == 0, "the editor window is gone after the edit ends");
    host.log.clear();
    Check(table.BeginEdit(2, 0), "a second edit can begin");
    ::SetWindowTextW(table.EditorWindow(), L"discard me");
    SendMessage(table.EditorWindow(), WM_KEYDOWN, VK_ESCAPE, 0);
    Check(!table.IsEditing() && table.GetCellText(2, 0) == L"Charlie" && host.Count(201, TableEditCommitted) == 0, "Escape cancels the edit, keeps the cell and does not notify");
    Pump();
    table.SelectRow(0);
    Key(table.GetSafeHwnd(), VK_F2);
    Check(table.IsEditing() && table.EditorWindow() != nullptr, "F2 starts editing the focused row");
    ::SetWindowTextW(table.EditorWindow(), L"Delta");
    SendMessage(table.EditorWindow(), WM_KEYDOWN, VK_RETURN, 0);
    Check(host.Count(201, TableEditCommitted) == 0, "committing unchanged text does not notify");
    Pump();

    // ---- context menu
    host.contextMenus = 0;
    table.SelectRow(0);
    {
        const CPoint p = RowPoint(table, 2);
        SendMessage(table.GetSafeHwnd(), WM_RBUTTONDOWN, MK_RBUTTON, Point(p.x, p.y));
        CPoint screen = p;
        ClientToScreen(table.GetSafeHwnd(), &screen);
        SendMessage(table.GetSafeHwnd(), WM_CONTEXTMENU, reinterpret_cast<WPARAM>(table.GetSafeHwnd()), Point(screen.x, screen.y));
    }
    Check(host.contextMenus == 1 && table.ContextRow() == 2 && table.GetSelectedRow() == 2, "right click selects the row and sends WM_CONTEXTMENU with the row");
    SendMessage(table.GetSafeHwnd(), WM_CONTEXTMENU, reinterpret_cast<WPARAM>(table.GetSafeHwnd()), static_cast<LPARAM>(-1));
    Check(host.contextMenus == 2 && table.ContextRow() == table.GetCurrentRow(), "Shift+F10 style context menu uses the focused row");

    // ---- disabled
    table.SelectRow(0);
    table.EnableWindow(FALSE);
    host.log.clear();
    ClickRow(table, 2);
    Key(table.GetSafeHwnd(), VK_DOWN);
    Check(table.GetSelectedRow() == 0 && host.log.empty(), "a disabled table ignores the mouse and keyboard");
    table.EnableWindow(TRUE);

    // ----------------------------------------------------------------- virtual rows
    DataTable big;
    Check(big.Create(CRect(10, 500, 530, 980), &host, 202) != FALSE, "virtual table created");
    big.SetColumns(NameSizeColor());
    std::set<int> asked;
    int calls{};
    big.SetVirtualRows(10000, [&](int row, int column, TableCell& cell)
    {
        asked.insert(row); ++calls;
        wchar_t text[32];
        swprintf_s(text, column == 0 ? L"Item %05d" : L"%d", column == 0 ? row : row * 3);
        cell.text = text;
    });
    const int visible = big.LastVisibleRow() - big.FirstVisibleRow() + 1;
    Check(big.GetRowCount() == 10000 && big.IsVirtual(), "virtual table reports ten thousand rows");
    Check(!asked.empty() && static_cast<int>(asked.size()) <= visible + 1 && *asked.rbegin() <= big.LastVisibleRow() + 1, "only the visible rows are requested from the row source");
    big.ResetFetchCount();
    Check(WindowInk(big) > 800, "the virtual table paints visible output");
    Check(big.FetchCount() <= (visible + 1) * 3 && big.FetchCount() >= visible * 3 - 3, "a paint fetches at most the visible cells");
    Check(big.ScrollbarVisible(), "ten thousand rows show the custom scroll rail");
    asked.clear();
    big.ScrollTo(4000 * big.RowRect(1).top - 4000 * big.RowRect(0).top);
    Check(big.FirstVisibleRow() >= 3990 && big.FirstVisibleRow() <= 4010, "scrolling jumps to the requested region");
    Check(!asked.empty() && *asked.begin() >= big.FirstVisibleRow() - 1 && *asked.rbegin() <= big.LastVisibleRow() + 1 && static_cast<int>(asked.size()) <= visible + 3, "scrolling requests only the newly visible rows");
    Check(big.GetCellText(4000, 0) == L"Item 04000", "any row can be read on demand");
    asked.clear();
    SendMessage(big.GetSafeHwnd(), WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), Point(100, 100));
    const int afterWheel = big.ScrollOffset();
    Check(afterWheel > 0 && big.FirstVisibleRow() >= 4000, "the mouse wheel scrolls by rows");
    {
        const int before = big.ScrollOffset();
        big.Rail().SendMessage(WM_KEYDOWN, VK_NEXT, 0);
        Check(big.ScrollOffset() > before, "the custom scroll rail pages the table");
    }
    big.SetMultiSelect(true);
    Key(big.GetSafeHwnd(), VK_HOME);
    Check(big.GetSelectedRow() == 0 && big.FirstVisibleRow() == 0, "Home selects and shows the first virtual row");
    Key(big.GetSafeHwnd(), VK_END);
    Check(big.GetSelectedRow() == 9999 && big.LastVisibleRow() == 9999, "End selects and shows the last virtual row");
    Sleep(TypeaheadWait());
    big.SelectRow(0);
    Chars(big.GetSafeHwnd(), L"Item 05");
    Check(big.GetSelectedRow() == 5000, "typeahead on a virtual table searches the first column");
    big.SetRowCount(5);
    Check(big.GetRowCount() == 5 && !big.ScrollbarVisible() && big.GetSelectedRow() == -1, "shrinking the row count drops the rail and stale selection");
    Check(StockChildren(big.GetSafeHwnd(), false) == 0, "the virtual table has no stock child windows");

    // ----------------------------------------------------------------- automation
    {
        table.SetRows({ MakeRow(L"Delta", L"40", RGB(200, 30, 30), 1), MakeRow(L"alpha", L"5", RGB(30, 160, 60), 2), MakeRow(L"Charlie", L"300", RGB(30, 60, 200), 3) });
        table.SetMultiSelect(true);
        table.SetSelectedRows({ 1 });
        auto root = Root(table);
        Check(root && Name(root.Get()) == L"Files" && ControlType(root.Get()) == UIA_TableControlTypeId, "table root exposes its name and the Table control type");
        auto grid = Pattern<IGridProvider>(root.Get(), UIA_GridPatternId);
        int rows = 0, columns = 0;
        Check(grid && SUCCEEDED(grid->get_RowCount(&rows)) && SUCCEEDED(grid->get_ColumnCount(&columns)) && rows == 3 && columns == 3, "Grid pattern reports the row and column counts");
        auto tablePattern = Pattern<ITableProvider>(root.Get(), UIA_TablePatternId);
        SAFEARRAY* headers{};
        Check(tablePattern && SUCCEEDED(tablePattern->GetColumnHeaders(&headers)) && ArrayLength(headers) == 3, "Table pattern lists the column headers");
        if (headers)
        {
            auto header = ElementOf(headers, 1);
            Check(header && Name(header.Get()) == L"Size" && ControlType(header.Get()) == UIA_HeaderItemControlTypeId, "a column header exposes its title as a header item");
            SafeArrayDestroy(headers);
        }
        ComPtr<IRawElementProviderSimple> cell;
        Check(grid && SUCCEEDED(grid->GetItem(2, 0, cell.GetAddressOf())) && cell && Name(cell.Get()) == L"Charlie" && ControlType(cell.Get()) == UIA_DataItemControlTypeId, "Grid GetItem returns a data cell with its text");
        auto gridItem = Pattern<IGridItemProvider>(cell.Get(), UIA_GridItemPatternId);
        int row = -1, column = -1;
        Check(gridItem && SUCCEEDED(gridItem->get_Row(&row)) && SUCCEEDED(gridItem->get_Column(&column)) && row == 2 && column == 0, "GridItem reports the cell position");
        auto tableItem = Pattern<ITableItemProvider>(cell.Get(), UIA_TableItemPatternId);
        SAFEARRAY* columnHeaders{};
        Check(tableItem && SUCCEEDED(tableItem->GetColumnHeaderItems(&columnHeaders)) && ArrayLength(columnHeaders) == 1, "TableItem names the column header of a cell");
        if (columnHeaders) { auto h = ElementOf(columnHeaders, 0); Check(h && Name(h.Get()) == L"Name", "the cell's column header is its column title"); SafeArrayDestroy(columnHeaders); }
        ComPtr<IRawElementProviderSimple> outOfRange;
        Check(grid && FAILED(grid->GetItem(9, 0, outOfRange.GetAddressOf())), "Grid GetItem rejects a cell outside the table");
        auto selection = Pattern<ISelectionProvider>(root.Get(), UIA_SelectionPatternId);
        SAFEARRAY* selected{};
        BOOL multiple = FALSE;
        Check(selection && SUCCEEDED(selection->GetSelection(&selected)) && ArrayLength(selected) == 1 && SUCCEEDED(selection->get_CanSelectMultiple(&multiple)) && multiple, "Selection pattern lists the selected row and allows several");
        if (selected)
        {
            auto item = ElementOf(selected, 0);
            BOOL isSelected = FALSE;
            auto itemPattern = Pattern<ISelectionItemProvider>(item.Get(), UIA_SelectionItemPatternId);
            Check(item && Name(item.Get()) == L"alpha" && itemPattern && SUCCEEDED(itemPattern->get_IsSelected(&isSelected)) && isSelected, "the selected row is a SelectionItem named by its first cell");
            SafeArrayDestroy(selected);
        }
        // Rows are reachable through the selection; a row item points back to its table.
        SAFEARRAY* again{};
        selection->GetSelection(&again);
        auto firstRow = ElementOf(again, 0);
        if (again) SafeArrayDestroy(again);
        ComPtr<IRawElementProviderSimple> container;
        auto firstItem = Pattern<ISelectionItemProvider>(firstRow.Get(), UIA_SelectionItemPatternId);
        Check(firstItem && SUCCEEDED(firstItem->get_SelectionContainer(container.GetAddressOf())) && container && ControlType(container.Get()) == UIA_TableControlTypeId, "a row item points back to its table");
        Check(!Pattern<IToggleProvider>(root.Get(), UIA_TogglePatternId), "a plain table does not claim the Toggle pattern");
        // Drive the Select action posted by a provider.
        {
            BOOL isSelected = FALSE;
            table.SelectRow(2);
            SAFEARRAY* sel{};
            selection->GetSelection(&sel);
            auto thirdRow = ElementOf(sel, 0);
            if (sel) SafeArrayDestroy(sel);
            table.SelectRow(0);
            host.log.clear();
            auto pattern = Pattern<ISelectionItemProvider>(thirdRow.Get(), UIA_SelectionItemPatternId);
            Check(pattern && SUCCEEDED(pattern->Select()), "SelectionItem Select posts the request");
            Pump();
            Check(table.GetSelectedRow() == 2 && host.Count(201, TableSelectionChanged) == 1 && pattern && SUCCEEDED(pattern->get_IsSelected(&isSelected)) && isSelected, "the posted Select changes the table selection");
            if (pattern) pattern->AddToSelection();
            Pump();
            Check(table.GetSelectedRows() == std::vector<int>({ 2 }), "AddToSelection on a selected row keeps the selection");
        }
        table.SetMultiSelect(false);
    }

    // ----------------------------------------------------------------- MaterialList: single select
    MaterialList list;
    Check(list.Create(CRect(540, 10, 840, 330), &host, 301) != FALSE, "material list created");
    list.SetAccessibleName(L"Skins");
    list.SetItems({ L"Default", L"Dark", L"Light", L"Contrast", L"Mono", L"Neon" });
    host.log.clear();
    Check(list.GetCount() == 6 && list.GetItemText(3) == L"Contrast" && list.GetCurSel() == -1, "list holds its items without a selection");
    ClickAt(list.GetSafeHwnd(), RowPoint(list, 2));
    Check(list.GetCurSel() == 2 && host.Count(301, TableSelectionChanged) == 1, "click selects an item and notifies");
    list.SetCurSel(4);
    Check(list.GetCurSel() == 4 && host.Count(301, TableSelectionChanged) == 1, "SetCurSel does not notify");
    Key(list.GetSafeHwnd(), VK_UP);
    Check(list.GetCurSel() == 3 && host.Count(301, TableSelectionChanged) == 2, "Up selects the previous item");
    Key(list.GetSafeHwnd(), VK_RETURN);
    Check(list.ActivatedRow() == 3 && host.Count(301, TableActivated) == 1, "Enter activates the selected item");
    Check(list.BuildModel().showHeader == false, "a list has no header row");
    {
        auto root = Root(list);
        Check(root && ControlType(root.Get()) == UIA_ListControlTypeId && Name(root.Get()) == L"Skins", "list root exposes the List control type and its name");
        Check(!Pattern<IGridProvider>(root.Get(), UIA_GridPatternId) && Pattern<ISelectionProvider>(root.Get(), UIA_SelectionPatternId), "a list exposes Selection and not Grid");
        SAFEARRAY* sel{};
        auto selection = Pattern<ISelectionProvider>(root.Get(), UIA_SelectionPatternId);
        selection->GetSelection(&sel);
        auto item = ElementOf(sel, 0);
        Check(item && Name(item.Get()) == L"Contrast" && ControlType(item.Get()) == UIA_ListItemControlTypeId && !Pattern<IToggleProvider>(item.Get(), UIA_TogglePatternId), "the selected list item is a named ListItem without Toggle");
        if (sel) SafeArrayDestroy(sel);
    }

    // ----------------------------------------------------------------- MaterialList: checkable with reorder
    MaterialList order;
    Check(order.Create(CRect(540, 340, 840, 640), &host, 302) != FALSE, "checkable list created");
    order.SetAccessibleName(L"Connections");
    order.SetModifierSource(Modifier);
    order.SetCheckable(true);
    order.SetReorderable(true);
    for (const wchar_t* name : { L"Ethernet", L"Wi-Fi", L"VPN", L"Cellular" }) order.AddItem(name);
    order.SetItemChecked(0, true);
    host.log.clear();
    Check(order.GetItemChecked(0) && !order.GetItemChecked(1), "items start with their programmatic check state");
    {
        const CRect box = order.CheckRect(1);
        ClickAt(order.GetSafeHwnd(), box.CenterPoint());
    }
    Check(order.GetItemChecked(1) && host.Count(302, TableCheckChanged) == 1 && order.CheckedRow() == 1 && order.GetCurSel() == -1, "clicking the check box toggles the item and notifies without selecting");
    ClickRow(order, 1);
    Check(order.GetCurSel() == 1 && order.GetItemChecked(1) && host.Count(302, TableCheckChanged) == 1, "clicking the label selects without toggling");
    Key(order.GetSafeHwnd(), VK_SPACE);
    Check(!order.GetItemChecked(1) && host.Count(302, TableCheckChanged) == 2, "Space toggles the focused item");
    order.SetItemChecked(1, true);
    Check(host.Count(302, TableCheckChanged) == 2, "programmatic check does not notify");
    host.log.clear();
    AltKey(order.GetSafeHwnd(), VK_DOWN);
    Check(order.GetItemText(2) == L"Wi-Fi" && order.GetItemText(1) == L"VPN" && order.GetCurSel() == 2 && host.Count(302, TableReordered) == 1
        && order.ReorderedFrom() == 1 && order.ReorderedTo() == 2, "Alt+Down moves the item down, keeps it selected and notifies");
    Check(order.GetItemChecked(2) && !order.GetItemChecked(1), "a moved item keeps its check state");
    AltKey(order.GetSafeHwnd(), VK_UP);
    Check(order.GetItemText(1) == L"Wi-Fi" && order.GetCurSel() == 1 && host.Count(302, TableReordered) == 2, "Alt+Up moves it back");
    order.SelectRow(0);
    Key(order.GetSafeHwnd(), VK_HOME);
    AltKey(order.GetSafeHwnd(), VK_UP);
    Check(order.GetItemText(0) == L"Ethernet" && host.Count(302, TableReordered) == 2, "Alt+Up on the first item changes nothing");
    // drag the first item below the third
    host.log.clear();
    {
        const CPoint from = RowPoint(order, 0);
        const CPoint to(from.x + 120, order.RowRect(2).bottom - 6);
        SendMessage(order.GetSafeHwnd(), WM_LBUTTONDOWN, MK_LBUTTON, Point(from.x + 120, from.y));
        SendMessage(order.GetSafeHwnd(), WM_MOUSEMOVE, MK_LBUTTON, Point(to.x, to.y));
        Check(order.BuildModel().dropGap == 3, "dragging shows the insertion line between the rows");
        SendMessage(order.GetSafeHwnd(), WM_LBUTTONUP, 0, Point(to.x, to.y));
    }
    Check(order.GetItemText(2) == L"Ethernet" && order.GetItemText(0) == L"Wi-Fi" && host.Count(302, TableReordered) == 1 && order.GetCurSel() == 2, "dragging an item reorders the list");
    {
        auto root = Root(order);
        SAFEARRAY* sel{};
        auto selection = Pattern<ISelectionProvider>(root.Get(), UIA_SelectionPatternId);
        selection->GetSelection(&sel);
        auto item = ElementOf(sel, 0);
        auto toggle = Pattern<IToggleProvider>(item.Get(), UIA_TogglePatternId);
        ToggleState state{};
        Check(item && Name(item.Get()) == L"Ethernet" && toggle && SUCCEEDED(toggle->get_ToggleState(&state)) && state == ToggleState_On, "a checkable list item exposes its check state through Toggle");
        host.log.clear();
        Check(toggle && SUCCEEDED(toggle->Toggle()), "Toggle posts the request");
        Pump();
        Check(!order.GetItemChecked(2) && host.Count(302, TableCheckChanged) == 1, "the posted Toggle unchecks the item and notifies");
        if (sel) SafeArrayDestroy(sel);
    }
    Check(StockChildren(order.GetSafeHwnd(), false) == 0 && StockChildren(list.GetSafeHwnd(), false) == 0, "lists have no stock child windows");

    // ----------------------------------------------------------------- real window paint
    for (auto appearance : { Appearance::Light, Appearance::Dark })
    {
        table.SetAppearance(appearance); list.SetAppearance(appearance); order.SetAppearance(appearance); big.SetAppearance(appearance);
        big.SetRowCount(10000);
        const bool allInk = WindowInk(table) > 800 && WindowInk(list) > 400 && WindowInk(order) > 400 && WindowInk(big) > 800;
        Check(allInk, appearance == Appearance::Light ? "every real table and list paints visible light output through WM_PRINTCLIENT" : "every real table and list paints visible dark output through WM_PRINTCLIENT");
    }

    // ---------------------------------------------------- offscreen rendering
    HICON icon = LoadIcon(nullptr, IDI_INFORMATION);
    for (UINT dpi : { 96u, 144u })
    for (auto appearance : { Appearance::Light, Appearance::Dark })
    {
        const char* tag = appearance == Appearance::Light ? "light" : "dark";
        char label[160];
        auto named = [&](const char* what) { std::snprintf(label, sizeof(label), "%s renders non-blank at %u dpi %s", what, dpi, tag); return label; };
        InputState on; InputState focus; focus.focused = true; InputState off; off.enabled = false;
        auto make = [&](bool selected, bool withIcon, bool sorted, bool checkable) {
            TablePaintModel m;
            m.columns = NameSizeColor();
            m.viewportWidth = 480; m.rowCount = 4;
            m.checkable = checkable;
            if (sorted) { m.sortColumn = 0; m.sortDirection = SortDirection::Ascending; }
            const wchar_t* names[] = { L"Alpha", L"Bravo", L"Charlie", L"Delta" };
            for (int i = 0; i < 4; ++i)
            {
                TablePaintRow row;
                row.index = i; row.selected = selected && i == 1; row.focused = i == 1;
                TableCell a; a.text = names[i]; if (withIcon) a.icon = icon;
                TableCell b; b.text = L"12";
                TableCell c; c.text = L"red"; c.hasSwatch = withIcon; c.swatch = RGB(220, 40, 40);
                row.cells = { a, b, c };
                row.check = i == 0 ? CheckState::Checked : CheckState::Unchecked;
                m.rows.push_back(row);
            }
            return m;
        };
        const auto plain = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, make(false, false, false, false), on); });
        const auto chosen = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, make(true, false, false, false), focus); });
        const auto sorted = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, make(false, false, true, false), on); });
        const auto decorated = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, make(false, true, false, false), on); });
        const auto checks = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, make(false, false, false, true), on); });
        const auto disabled = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, make(true, false, false, false), off); });
        auto emptyModel = make(false, false, false, false);
        emptyModel.rows.clear(); emptyModel.rowCount = 0; emptyModel.emptyText = L"No files";
        const auto empty = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, emptyModel, on); });
        auto dropModel = make(false, false, false, false);
        dropModel.dropGap = 2;
        const auto drop = Render(dpi, 480, 260, appearance, [&](auto* t, const auto& p) { DrawDataTable(t, p, dropModel, on); });
        Check(Ink(plain) > 3000, named("data table"));
        Check(Difference(plain, chosen) > 500 && Difference(plain, sorted) > 20 && Difference(plain, checks) > 200 && Difference(plain, disabled) > 500
            && Difference(plain, drop) > 100, "table plain, selected, sorted, checkable, disabled and drop states are visibly different");
        Check(Difference(plain, decorated) > 150, "cell icons and color swatches are drawn");
        Check(Ink(empty) > 800 && Difference(plain, empty) > 3000, named("empty state"));
    }

    host.DestroyWindow();
    std::printf("RESULT failures=%d\n", failures);
    return failures ? 1 : 0;
}
