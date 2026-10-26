#pragma once
#include "InputControls.Accessibility.h"

// UI Automation provider for the Material 3 calendar: the root is a Calendar element with the Grid and
// Selection patterns, every cell is a data item with the GridItem and SelectionItem patterns. Like the other
// providers it only reads a synchronized snapshot and posts requested changes to the control's own window.
namespace NativeMaterial
{
    constexpr int CalendarRows = 6, CalendarColumns = 7;

    struct CalendarCellAccess
    {
        int day{};                  // 0 for a blank cell
        std::wstring name;
    };
    struct CalendarAccess
    {
        std::mutex mutex;
        HWND window{};
        std::wstring name;
        bool enabled{ true };
        int selectedCell{ -1 };     // row * 7 + column, or -1
        CalendarCellAccess cells[CalendarRows][CalendarColumns];
    };

    class CalendarProvider final : public IRawElementProviderSimple, public IGridProvider, public ISelectionProvider,
        public IGridItemProvider, public ISelectionItemProvider
    {
    public:
        explicit CalendarProvider(std::shared_ptr<CalendarAccess> state, int row = -1, int column = -1)
            : m_state(std::move(state)), m_row(row), m_column(column) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            const bool root = m_row < 0;
            if (id == __uuidof(IUnknown) || id == __uuidof(IRawElementProviderSimple)) *out = static_cast<IRawElementProviderSimple*>(this);
            else if (id == __uuidof(IGridProvider) && root) *out = static_cast<IGridProvider*>(this);
            else if (id == __uuidof(ISelectionProvider) && root) *out = static_cast<ISelectionProvider*>(this);
            else if (id == __uuidof(IGridItemProvider) && !root) *out = static_cast<IGridItemProvider*>(this);
            else if (id == __uuidof(ISelectionItemProvider) && !root) *out = static_cast<ISelectionItemProvider*>(this);
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
                : id == UIA_SelectionPatternId ? __uuidof(ISelectionProvider)
                : id == UIA_GridItemPatternId ? __uuidof(IGridItemProvider)
                : id == UIA_SelectionItemPatternId ? __uuidof(ISelectionItemProvider) : GUID_NULL;
            if (iid == GUID_NULL) return S_OK;
            const auto result = QueryInterface(iid, reinterpret_cast<void**>(out));
            return result == E_NOINTERFACE ? S_OK : result;
        }
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* out) override
        {
            if (!out) return E_POINTER;
            VariantInit(out);
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            const bool root = m_row < 0;
            if (id == UIA_ControlTypePropertyId) { out->vt = VT_I4; out->lVal = root ? UIA_CalendarControlTypeId : UIA_DataItemControlTypeId; }
            else if (id == UIA_NamePropertyId)
            {
                const std::wstring& name = root ? m_state->name : m_state->cells[m_row][m_column].name;
                out->vt = VT_BSTR;
                out->bstrVal = SysAllocString(name.c_str());
                return out->bstrVal ? S_OK : E_OUTOFMEMORY;
            }
            else if (id == UIA_IsEnabledPropertyId)
            { out->vt = VT_BOOL; out->boolVal = (m_state->enabled && (root || m_state->cells[m_row][m_column].day > 0)) ? VARIANT_TRUE : VARIANT_FALSE; }
            else if (id == UIA_IsKeyboardFocusablePropertyId) { out->vt = VT_BOOL; out->boolVal = root ? VARIANT_TRUE : VARIANT_FALSE; }
            else if (id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId) { out->vt = VT_BOOL; out->boolVal = VARIANT_TRUE; }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            return m_row >= 0 ? S_OK : UiaHostProviderFromHwnd(m_state->window, out);
        }
        // Grid
        HRESULT STDMETHODCALLTYPE GetItem(int row, int column, IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            if (row < 0 || row >= CalendarRows || column < 0 || column >= CalendarColumns) return E_INVALIDARG;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = new CalendarProvider(m_state, row, column);
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_RowCount(int* out) override { if (!out) return E_POINTER; *out = CalendarRows; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_ColumnCount(int* out) override { if (!out) return E_POINTER; *out = CalendarColumns; return S_OK; }
        // Selection
        HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            const int cell = m_state->selectedCell;
            const bool has = cell >= 0;
            *out = SafeArrayCreateVector(VT_UNKNOWN, 0, has ? 1 : 0);
            if (!*out) return E_OUTOFMEMORY;
            if (has)
            {
                auto* provider = new CalendarProvider(m_state, cell / CalendarColumns, cell % CalendarColumns);
                LONG index = 0;
                IUnknown* unknown = static_cast<IRawElementProviderSimple*>(provider);
                const HRESULT hr = SafeArrayPutElement(*out, &index, unknown);
                provider->Release();
                if (FAILED(hr)) { SafeArrayDestroy(*out); *out = nullptr; return hr; }
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* out) override { if (!out) return E_POINTER; *out = FALSE; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* out) override { if (!out) return E_POINTER; *out = FALSE; return S_OK; }
        // GridItem
        HRESULT STDMETHODCALLTYPE get_Row(int* out) override { if (!out) return E_POINTER; *out = m_row; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_Column(int* out) override { if (!out) return E_POINTER; *out = m_column; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_RowSpan(int* out) override { if (!out) return E_POINTER; *out = 1; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_ColumnSpan(int* out) override { if (!out) return E_POINTER; *out = 1; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_ContainingGrid(IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = new CalendarProvider(m_state);
            return S_OK;
        }
        // SelectionItem
        HRESULT STDMETHODCALLTYPE Select() override
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            if (!m_state->enabled || m_state->cells[m_row][m_column].day <= 0) return UIA_E_ELEMENTNOTENABLED;
            return PostMessageW(m_state->window, InputAction, static_cast<WPARAM>(InputActionId::Select), m_row * CalendarColumns + m_column)
                ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
        HRESULT STDMETHODCALLTYPE AddToSelection() override { return Select(); }
        HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { return UIA_E_INVALIDOPERATION; }
        HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* out) override
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = m_state->selectedCell == m_row * CalendarColumns + m_column;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = new CalendarProvider(m_state);
            return S_OK;
        }
    private:
        std::atomic<ULONG> m_refs{ 1 };
        std::shared_ptr<CalendarAccess> m_state;
        int m_row, m_column;
    };
}
