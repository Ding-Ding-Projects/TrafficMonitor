#pragma once
#include <UIAutomation.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <cmath>

// UI Automation providers for the Material 3 input components. A provider only
// reads a synchronized snapshot and posts requested changes to the control's
// own window, so it never touches a window from a foreign thread and survives
// outstanding calls after the control is destroyed (the window is revoked).
namespace NativeMaterial
{
    enum class InputKind { Switch, Checkbox, Radio, RadioGroup, Segmented, Stepper, Dropdown, TextField, Button, Slider, Table, List, Tabs };
    enum class InputActionId : WPARAM { Toggle, Select, SetValue, Expand, Collapse };
    constexpr UINT InputAction = WM_APP + 0x540;

    struct InputData
    {
        InputKind kind{ InputKind::Switch };
        bool enabled{ true }, expanded{}, error{};
        int toggle{};            // 0 off, 1 on, 2 indeterminate
        int value{}, minimum{}, maximum{}, step{ 1 }, page{};
        int selected{ -1 };      // selected item index, or -1
        std::wstring name, text, help;
        std::vector<std::wstring> items;
    };
    struct InputAccess
    {
        std::mutex mutex;
        HWND window{};
        InputData data;
    };

    class InputProvider final : public IRawElementProviderSimple, public IToggleProvider, public ISelectionItemProvider,
        public ISelectionProvider, public IRangeValueProvider, public IExpandCollapseProvider, public IValueProvider, public IInvokeProvider
    {
    public:
        explicit InputProvider(std::shared_ptr<InputAccess> state, int item = -1) : m_state(std::move(state)), m_item(item)
        {
            if (item >= 0) { std::lock_guard<std::mutex> lock(m_state->mutex); if (item < static_cast<int>(m_state->data.items.size())) m_itemName = m_state->data.items[item]; }
        }
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            const auto kind = Kind();
            const bool root = m_item < 0;
            const bool list = kind == InputKind::RadioGroup || kind == InputKind::Segmented || kind == InputKind::Dropdown || kind == InputKind::Tabs;
            if (id == __uuidof(IUnknown) || id == __uuidof(IRawElementProviderSimple)) *out = static_cast<IRawElementProviderSimple*>(this);
            else if (id == __uuidof(IToggleProvider) && root && (kind == InputKind::Switch || kind == InputKind::Checkbox)) *out = static_cast<IToggleProvider*>(this);
            else if (id == __uuidof(ISelectionItemProvider) && ((root && kind == InputKind::Radio) || (!root && list))) *out = static_cast<ISelectionItemProvider*>(this);
            else if (id == __uuidof(ISelectionProvider) && root && list) *out = static_cast<ISelectionProvider*>(this);
            else if (id == __uuidof(IRangeValueProvider) && root && (kind == InputKind::Stepper || kind == InputKind::Slider)) *out = static_cast<IRangeValueProvider*>(this);
            else if (id == __uuidof(IExpandCollapseProvider) && root && kind == InputKind::Dropdown) *out = static_cast<IExpandCollapseProvider*>(this);
            else if (id == __uuidof(IValueProvider) && root && kind == InputKind::Dropdown) *out = static_cast<IValueProvider*>(this);
            else if (id == __uuidof(IInvokeProvider) && root && kind == InputKind::Button) *out = static_cast<IInvokeProvider*>(this);
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
            REFIID iid = id == UIA_TogglePatternId ? __uuidof(IToggleProvider)
                : id == UIA_SelectionItemPatternId ? __uuidof(ISelectionItemProvider)
                : id == UIA_SelectionPatternId ? __uuidof(ISelectionProvider)
                : id == UIA_RangeValuePatternId ? __uuidof(IRangeValueProvider)
                : id == UIA_ExpandCollapsePatternId ? __uuidof(IExpandCollapseProvider)
                : id == UIA_ValuePatternId ? __uuidof(IValueProvider)
                : id == UIA_InvokePatternId ? __uuidof(IInvokeProvider) : GUID_NULL;
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
            const bool root = m_item < 0;
            if (id == UIA_ControlTypePropertyId)
            {
                out->vt = VT_I4;
                out->lVal = !root ? (d.kind == InputKind::Dropdown ? UIA_ListItemControlTypeId : d.kind == InputKind::Tabs ? UIA_TabItemControlTypeId : UIA_RadioButtonControlTypeId)
                    : d.kind == InputKind::Switch ? UIA_ButtonControlTypeId
                    : d.kind == InputKind::Checkbox ? UIA_CheckBoxControlTypeId
                    : d.kind == InputKind::Radio ? UIA_RadioButtonControlTypeId
                    : d.kind == InputKind::Stepper ? UIA_SpinnerControlTypeId
                    : d.kind == InputKind::Slider ? UIA_SliderControlTypeId
                    : d.kind == InputKind::Dropdown ? UIA_ComboBoxControlTypeId
                    : d.kind == InputKind::Button ? UIA_ButtonControlTypeId
                    : d.kind == InputKind::Tabs ? UIA_TabControlTypeId : UIA_GroupControlTypeId;
            }
            else if (id == UIA_LocalizedControlTypePropertyId && root && (d.kind == InputKind::Switch || d.kind == InputKind::Segmented))
                return Text(out, d.kind == InputKind::Switch ? L"switch" : L"segmented button");
            else if (id == UIA_NamePropertyId) return Text(out, !root ? m_itemName : d.name);
            else if (id == UIA_HelpTextPropertyId && root && !d.help.empty()) return Text(out, d.help);
            else if (id == UIA_IsEnabledPropertyId) { out->vt = VT_BOOL; out->boolVal = d.enabled ? VARIANT_TRUE : VARIANT_FALSE; }
            else if (id == UIA_IsKeyboardFocusablePropertyId) { out->vt = VT_BOOL; out->boolVal = root ? VARIANT_TRUE : VARIANT_FALSE; }
            else if (id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId) { out->vt = VT_BOOL; out->boolVal = VARIANT_TRUE; }
            else if (id == UIA_IsRequiredForFormPropertyId && root) { out->vt = VT_BOOL; out->boolVal = VARIANT_FALSE; }
            else if (id == UIA_ToggleToggleStatePropertyId && root && (d.kind == InputKind::Switch || d.kind == InputKind::Checkbox))
            { out->vt = VT_I4; out->lVal = d.toggle == 1 ? ToggleState_On : d.toggle == 2 ? ToggleState_Indeterminate : ToggleState_Off; }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            return m_item >= 0 ? S_OK : UiaHostProviderFromHwnd(m_state->window, out);
        }
        // Invoke (the clear button)
        HRESULT STDMETHODCALLTYPE Invoke() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(InputActionId::Toggle, 0); }
        // Toggle
        HRESULT STDMETHODCALLTYPE Toggle() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(InputActionId::Toggle, 0); }
        HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* out) override
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = m_state->data.toggle == 1 ? ToggleState_On : m_state->data.toggle == 2 ? ToggleState_Indeterminate : ToggleState_Off;
            return S_OK;
        }
        // SelectionItem (a standalone radio is its own item)
        HRESULT STDMETHODCALLTYPE Select() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(InputActionId::Select, m_item < 0 ? 0 : m_item); }
        HRESULT STDMETHODCALLTYPE AddToSelection() override { return Select(); }
        HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { return UIA_E_INVALIDOPERATION; }
        HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* out) override
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = m_item < 0 ? m_state->data.toggle == 1 : m_item == m_state->data.selected;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = new InputProvider(m_state);
            return S_OK;
        }
        // Selection
        HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const int selected = m_state->data.selected;
            const bool has = selected >= 0 && selected < static_cast<int>(m_state->data.items.size());
            *out = SafeArrayCreateVector(VT_UNKNOWN, 0, has ? 1 : 0);
            if (!*out) return E_OUTOFMEMORY;
            if (has)
            {
                auto* provider = new InputProvider(m_state, selected, true);
                LONG index = 0;
                IUnknown* unknown = static_cast<IRawElementProviderSimple*>(provider);
                const HRESULT hr = SafeArrayPutElement(*out, &index, unknown);
                provider->Release();
                if (FAILED(hr)) { SafeArrayDestroy(*out); *out = nullptr; return hr; }
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* out) override { if (!out) return E_POINTER; *out = FALSE; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* out) override
        { if (!out) return E_POINTER; *out = Kind() != InputKind::Dropdown; return S_OK; }
        // RangeValue
        HRESULT STDMETHODCALLTYPE SetValue(double value) override
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const auto& d = m_state->data;
            if (!std::isfinite(value) || value != std::floor(value) || value < d.minimum || value > d.maximum) return E_INVALIDARG;
            return Post(InputActionId::SetValue, static_cast<int>(value));
        }
        HRESULT STDMETHODCALLTYPE get_Value(double* out) override { return Number(out, 0); }
        HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* out) override
        { if (!out) return E_POINTER; std::lock_guard<std::mutex> lock(m_state->mutex); if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE; *out = FALSE; return S_OK; }
        HRESULT STDMETHODCALLTYPE get_Maximum(double* out) override { return Number(out, 2); }
        HRESULT STDMETHODCALLTYPE get_Minimum(double* out) override { return Number(out, 1); }
        HRESULT STDMETHODCALLTYPE get_LargeChange(double* out) override { return Number(out, 4); }
        HRESULT STDMETHODCALLTYPE get_SmallChange(double* out) override { return Number(out, 3); }
        // Value (text)
        HRESULT STDMETHODCALLTYPE SetValue(LPCWSTR value) override
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            if (m_state->data.kind != InputKind::Dropdown || !value) return UIA_E_INVALIDOPERATION;
            const auto& items = m_state->data.items;
            for (size_t i = 0; i < items.size(); ++i) if (items[i] == value) return Post(InputActionId::Select, static_cast<int>(i));
            return E_INVALIDARG;
        }
        HRESULT STDMETHODCALLTYPE get_Value(BSTR* out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = SysAllocString(m_state->data.text.c_str());
            return *out ? S_OK : E_OUTOFMEMORY;
        }
        // IsReadOnly(BOOL*) is shared with the RangeValue pattern above.
        // ExpandCollapse
        HRESULT STDMETHODCALLTYPE Expand() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(InputActionId::Expand, 0); }
        HRESULT STDMETHODCALLTYPE Collapse() override { std::lock_guard<std::mutex> lock(m_state->mutex); return Post(InputActionId::Collapse, 0); }
        HRESULT STDMETHODCALLTYPE get_ExpandCollapseState(ExpandCollapseState* out) override
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = m_state->data.expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed;
            return S_OK;
        }
    private:
        InputProvider(std::shared_ptr<InputAccess> state, int item, bool) : m_state(std::move(state)), m_item(item)
        {
            if (item >= 0 && item < static_cast<int>(m_state->data.items.size())) m_itemName = m_state->data.items[item];
        }
        InputKind Kind() const { std::lock_guard<std::mutex> lock(m_state->mutex); return m_state->data.kind; }
        bool Available() const
        {
            if (!m_state->window) return false;
            return m_item < 0 || (m_item < static_cast<int>(m_state->data.items.size()) && m_itemName == m_state->data.items[m_item]);
        }
        HRESULT Post(InputActionId action, int value)
        {
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            if (!m_state->data.enabled) return UIA_E_ELEMENTNOTENABLED;
            return PostMessageW(m_state->window, InputAction, static_cast<WPARAM>(action), value) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
        HRESULT Number(double* out, int field)
        {
            if (!out) return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const auto& d = m_state->data;
            *out = field == 0 ? d.value : field == 1 ? d.minimum : field == 2 ? d.maximum : field == 3 ? d.step : d.page > 0 ? d.page : (std::max)(d.step, d.step * 10);
            return S_OK;
        }
        static HRESULT Text(VARIANT* out, const std::wstring& text)
        {
            out->vt = VT_BSTR;
            out->bstrVal = SysAllocString(text.c_str());
            return out->bstrVal ? S_OK : E_OUTOFMEMORY;
        }
        std::atomic<ULONG> m_refs{ 1 };
        std::shared_ptr<InputAccess> m_state;
        int m_item;
        std::wstring m_itemName;
    };
}
