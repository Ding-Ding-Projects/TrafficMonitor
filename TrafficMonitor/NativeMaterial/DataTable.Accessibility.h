#pragma once
#include <UIAutomation.h>
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// UI Automation providers for the Material 3 data table and list. Like the input providers, a provider
// reads a synchronized snapshot (only the rows that are currently realized carry text) and posts requested
// changes to the control's own window, so it never touches the window from a foreign thread and survives
// outstanding calls after the control is destroyed.
//
// Table root: Table, Grid and Selection patterns. Cell: GridItem and TableItem. Header: a header item.
// Row: SelectionItem (and Toggle when the rows are checkable). List root: Selection only.
namespace NativeMaterial
{
    constexpr UINT TableActionMessage = WM_APP + 0x544;
    enum class TableActionId : WPARAM { Select, AddToSelection, RemoveFromSelection, Toggle };
    enum class TableRole { Root, Row, Cell, Header };

    struct TableData
    {
        std::wstring name;
        bool enabled{ true }, multi{}, listMode{}, checkable{};
        int rowCount{}, columnCount{}, firstRealized{};
        std::vector<std::wstring> headers;
        std::vector<int> selected;                           // ascending
        std::vector<std::vector<std::wstring>> realizedText; // rows firstRealized...
        std::vector<int> realizedCheck;                      // 0 unchecked, 1 checked, 2 mixed
    };
    struct TableAccess
    {
        std::mutex mutex;
        HWND window{};
        TableData data;
    };

    class TableProvider final : public IRawElementProviderSimple, public IGridProvider, public ITableProvider, public ISelectionProvider,
        public IGridItemProvider, public ITableItemProvider, public ISelectionItemProvider, public IToggleProvider
    {
    public:
        explicit TableProvider(std::shared_ptr<TableAccess> state, TableRole role = TableRole::Root, int row = -1, int column = -1)
            : m_state(std::move(state)), m_role(role), m_row(row), m_column(column) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            bool list, checkable;
            { std::lock_guard<std::mutex> lock(m_state->mutex); list = m_state->data.listMode; checkable = m_state->data.checkable; }
            const bool root = m_role == TableRole::Root;
            if (id == __uuidof(IUnknown) || id == __uuidof(IRawElementProviderSimple)) *out = static_cast<IRawElementProviderSimple*>(this);
            else if (id == __uuidof(IGridProvider) && root && !list) *out = static_cast<IGridProvider*>(this);
            else if (id == __uuidof(ITableProvider) && root && !list) *out = static_cast<ITableProvider*>(this);
            else if (id == __uuidof(ISelectionProvider) && root) *out = static_cast<ISelectionProvider*>(this);
            else if (id == __uuidof(IGridItemProvider) && m_role == TableRole::Cell) *out = static_cast<IGridItemProvider*>(this);
            else if (id == __uuidof(ITableItemProvider) && m_role == TableRole::Cell) *out = static_cast<ITableItemProvider*>(this);
            else if (id == __uuidof(ISelectionItemProvider) && m_role == TableRole::Row) *out = static_cast<ISelectionItemProvider*>(this);
            else if (id == __uuidof(IToggleProvider) && m_role == TableRole::Row && checkable) *out = static_cast<IToggleProvider*>(this);
            else return E_NOINTERFACE;
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
        ULONG STDMETHODCALLTYPE Release() override { const auto n = --m_refs; if (!n) delete this; return n; }
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* out) override
        { if (!out) return E_POINTER; *out = ProviderOptions_ServerSideProvider; return S_OK; }
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id, IUnknown** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            REFIID iid = id == UIA_GridPatternId ? __uuidof(IGridProvider)
                : id == UIA_TablePatternId ? __uuidof(ITableProvider)
                : id == UIA_SelectionPatternId ? __uuidof(ISelectionProvider)
                : id == UIA_GridItemPatternId ? __uuidof(IGridItemProvider)
                : id == UIA_TableItemPatternId ? __uuidof(ITableItemProvider)
                : id == UIA_SelectionItemPatternId ? __uuidof(ISelectionItemProvider)
                : id == UIA_TogglePatternId ? __uuidof(IToggleProvider) : GUID_NULL;
            if (iid == GUID_NULL) return S_OK;
            const auto result = QueryInterface(iid, reinterpret_cast<void**>(out));
            return result == E_NOINTERFACE ? S_OK : result;
        }
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* out) override
        {
            if (!out) return E_POINTER;
            VariantInit(out);
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const auto& d = m_state->data;
            if (id == UIA_ControlTypePropertyId)
            {
                out->vt = VT_I4;
                out->lVal = m_role == TableRole::Root ? (d.listMode ? UIA_ListControlTypeId : UIA_TableControlTypeId)
                    : m_role == TableRole::Row ? (d.listMode ? UIA_ListItemControlTypeId : UIA_DataItemControlTypeId)
                    : m_role == TableRole::Cell ? UIA_DataItemControlTypeId : UIA_HeaderItemControlTypeId;
            }
            else if (id == UIA_NamePropertyId)
            {
                if (m_role == TableRole::Root) return Text(out, d.name);
                if (m_role == TableRole::Header) return Text(out, m_column < static_cast<int>(d.headers.size()) ? d.headers[m_column] : std::wstring());
                return Text(out, CellText(d, m_row, m_role == TableRole::Row ? 0 : m_column));
            }
            else if (id == UIA_IsEnabledPropertyId) { out->vt = VT_BOOL; out->boolVal = d.enabled ? VARIANT_TRUE : VARIANT_FALSE; }
            else if (id == UIA_IsKeyboardFocusablePropertyId) { out->vt = VT_BOOL; out->boolVal = m_role == TableRole::Root ? VARIANT_TRUE : VARIANT_FALSE; }
            else if (id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId) { out->vt = VT_BOOL; out->boolVal = VARIANT_TRUE; }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            return m_role == TableRole::Root ? UiaHostProviderFromHwnd(m_state->window, out) : S_OK;
        }

        // ---- Grid
        HRESULT STDMETHODCALLTYPE GetItem(int row, int column, IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            if (row < 0 || column < 0 || row >= m_state->data.rowCount || column >= m_state->data.columnCount) return E_INVALIDARG;
            *out = new TableProvider(m_state, TableRole::Cell, row, column);
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_RowCount(int* out) override { return Count(out, true); }
        HRESULT STDMETHODCALLTYPE get_ColumnCount(int* out) override { return Count(out, false); }

        // ---- Table
        HRESULT STDMETHODCALLTYPE GetRowHeaders(SAFEARRAY** out) override { return Headers(out, false); }
        HRESULT STDMETHODCALLTYPE GetColumnHeaders(SAFEARRAY** out) override { return Headers(out, true); }
        HRESULT STDMETHODCALLTYPE get_RowOrColumnMajor(RowOrColumnMajor* out) override
        { if (!out) return E_POINTER; *out = RowOrColumnMajor_RowMajor; return S_OK; }

        // ---- GridItem
        HRESULT STDMETHODCALLTYPE get_Row(int* out) override { return Position(out, m_row); }
        HRESULT STDMETHODCALLTYPE get_Column(int* out) override { return Position(out, m_column); }
        HRESULT STDMETHODCALLTYPE get_RowSpan(int* out) override { return Position(out, 1); }
        HRESULT STDMETHODCALLTYPE get_ColumnSpan(int* out) override { return Position(out, 1); }
        HRESULT STDMETHODCALLTYPE get_ContainingGrid(IRawElementProviderSimple** out) override { return Container(out); }

        // ---- TableItem
        HRESULT STDMETHODCALLTYPE GetRowHeaderItems(SAFEARRAY** out) override
        {
            if (!out) return E_POINTER;
            *out = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
            return *out ? S_OK : E_OUTOFMEMORY;
        }
        HRESULT STDMETHODCALLTYPE GetColumnHeaderItems(SAFEARRAY** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
            if (!*out) return E_OUTOFMEMORY;
            return Put(*out, 0, new TableProvider(m_state, TableRole::Header, -1, m_column));
        }

        // ---- Selection
        HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const auto& selected = m_state->data.selected;
            *out = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(selected.size()));
            if (!*out) return E_OUTOFMEMORY;
            for (size_t i = 0; i < selected.size(); ++i)
            {
                const HRESULT hr = Put(*out, static_cast<LONG>(i), new TableProvider(m_state, TableRole::Row, selected[i], -1));
                if (FAILED(hr)) { SafeArrayDestroy(*out); *out = nullptr; return hr; }
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* out) override
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = m_state->data.multi;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* out) override { if (!out) return E_POINTER; *out = FALSE; return S_OK; }

        // ---- SelectionItem (a row)
        HRESULT STDMETHODCALLTYPE Select() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(TableActionId::Select); }
        HRESULT STDMETHODCALLTYPE AddToSelection() override
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (Available() && !m_state->data.multi) return UIA_E_INVALIDOPERATION;
            return Post(TableActionId::AddToSelection);
        }
        HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(TableActionId::RemoveFromSelection); }
        HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* out) override
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const auto& s = m_state->data.selected;
            *out = std::binary_search(s.begin(), s.end(), m_row);
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** out) override { return Container(out); }

        // ---- Toggle (a checkable row)
        HRESULT STDMETHODCALLTYPE Toggle() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(TableActionId::Toggle); }
        HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* out) override
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const auto& d = m_state->data;
            const int index = m_row - d.firstRealized;
            const int value = index >= 0 && index < static_cast<int>(d.realizedCheck.size()) ? d.realizedCheck[index] : 0;
            *out = value == 1 ? ToggleState_On : value == 2 ? ToggleState_Indeterminate : ToggleState_Off;
            return S_OK;
        }

    private:
        bool Available() const
        {
            if (!m_state->window) return false;
            const auto& d = m_state->data;
            if (m_role == TableRole::Row) return m_row >= 0 && m_row < d.rowCount;
            if (m_role == TableRole::Cell) return m_row >= 0 && m_row < d.rowCount && m_column >= 0 && m_column < d.columnCount;
            if (m_role == TableRole::Header) return m_column >= 0 && m_column < d.columnCount;
            return true;
        }
        static std::wstring CellText(const TableData& d, int row, int column)
        {
            const int index = row - d.firstRealized;
            if (index < 0 || index >= static_cast<int>(d.realizedText.size())) return std::wstring();
            const auto& cells = d.realizedText[index];
            return column >= 0 && column < static_cast<int>(cells.size()) ? cells[column] : std::wstring();
        }
        static HRESULT Text(VARIANT* out, const std::wstring& text)
        {
            out->vt = VT_BSTR;
            out->bstrVal = SysAllocString(text.c_str());
            return out->bstrVal ? S_OK : E_OUTOFMEMORY;
        }
        // Stores the provider (a new reference) in a safe array, which takes its own reference.
        static HRESULT Put(SAFEARRAY* array, LONG index, TableProvider* provider)
        {
            IUnknown* unknown = static_cast<IRawElementProviderSimple*>(provider);
            const HRESULT hr = SafeArrayPutElement(array, &index, unknown);
            provider->Release();
            return hr;
        }
        HRESULT Count(int* out, bool rows)
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = rows ? m_state->data.rowCount : m_state->data.columnCount;
            return S_OK;
        }
        HRESULT Position(int* out, int value)
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = value;
            return S_OK;
        }
        HRESULT Container(IRawElementProviderSimple** out)
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = new TableProvider(m_state);
            return S_OK;
        }
        HRESULT Headers(SAFEARRAY** out, bool columns)
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const int count = columns ? m_state->data.columnCount : 0;
            *out = SafeArrayCreateVector(VT_UNKNOWN, 0, count);
            if (!*out) return E_OUTOFMEMORY;
            for (int i = 0; i < count; ++i)
            {
                const HRESULT hr = Put(*out, i, new TableProvider(m_state, TableRole::Header, -1, i));
                if (FAILED(hr)) { SafeArrayDestroy(*out); *out = nullptr; return hr; }
            }
            return S_OK;
        }
        HRESULT Post(TableActionId action)
        {
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            if (!m_state->data.enabled) return UIA_E_ELEMENTNOTENABLED;
            return PostMessageW(m_state->window, TableActionMessage, static_cast<WPARAM>(action), m_row) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
        std::atomic<ULONG> m_refs{ 1 };
        std::shared_ptr<TableAccess> m_state;
        TableRole m_role;
        int m_row, m_column;
    };
}
