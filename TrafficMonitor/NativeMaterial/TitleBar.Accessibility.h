#pragma once
#include <UIAutomation.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace NativeMaterial
{
    // Synchronized snapshot read by UI Automation threads. The UI thread refreshes it;
    // Invoke only posts a system command to the owning window, so no provider call
    // mutates window state on a foreign thread. Destruction revokes both handles first.
    struct TitleBarAccess
    {
        std::mutex mutex;
        HWND host{}, bar{};
        std::wstring title;
        bool present[3]{};
        std::wstring names[3];
        RECT cells[3]{};
    };

    inline const wchar_t* TitleBarAutomationId(int index) { return index == 0 ? L"TitleBar.Minimize" : index == 1 ? L"TitleBar.Maximize" : L"TitleBar.Close"; }

    class TitleBarProvider final : public IRawElementProviderSimple, public IRawElementProviderFragment,
        public IRawElementProviderFragmentRoot, public IInvokeProvider
    {
    public:
        // item < 0 is the title bar root, otherwise the caption button index 0..2.
        TitleBarProvider(std::shared_ptr<TitleBarAccess> state, int item = -1) : m_state(std::move(state)), m_item(item) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            if (id == __uuidof(IUnknown) || id == __uuidof(IRawElementProviderSimple)) *out = static_cast<IRawElementProviderSimple*>(this);
            else if (id == __uuidof(IRawElementProviderFragment)) *out = static_cast<IRawElementProviderFragment*>(this);
            else if (m_item < 0 && id == __uuidof(IRawElementProviderFragmentRoot)) *out = static_cast<IRawElementProviderFragmentRoot*>(this);
            else if (m_item >= 0 && id == __uuidof(IInvokeProvider)) *out = static_cast<IInvokeProvider*>(this);
            else return E_NOINTERFACE;
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
        ULONG STDMETHODCALLTYPE Release() override { auto n = --m_refs; if (!n) delete this; return n; }

        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* out) override
        {
            if (!out) return E_POINTER;
            *out = ProviderOptions_ServerSideProvider;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id, IUnknown** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            if (m_item >= 0 && id == UIA_InvokePatternId) return QueryInterface(__uuidof(IInvokeProvider), reinterpret_cast<void**>(out));
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* out) override
        {
            if (!out) return E_POINTER;
            VariantInit(out);
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            if (id == UIA_ControlTypePropertyId) { out->vt = VT_I4; out->lVal = m_item < 0 ? UIA_TitleBarControlTypeId : UIA_ButtonControlTypeId; }
            else if (id == UIA_NamePropertyId)
            {
                out->vt = VT_BSTR;
                out->bstrVal = SysAllocString((m_item < 0 ? m_state->title : m_state->names[m_item]).c_str());
                if (!out->bstrVal) return E_OUTOFMEMORY;
            }
            else if (id == UIA_AutomationIdPropertyId)
            {
                out->vt = VT_BSTR;
                out->bstrVal = SysAllocString(m_item < 0 ? L"TitleBar" : TitleBarAutomationId(m_item));
                if (!out->bstrVal) return E_OUTOFMEMORY;
            }
            else if (id == UIA_IsEnabledPropertyId || id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId)
            { out->vt = VT_BOOL; out->boolVal = VARIANT_TRUE; }
            else if (id == UIA_IsKeyboardFocusablePropertyId || id == UIA_HasKeyboardFocusPropertyId)
            { out->vt = VT_BOOL; out->boolVal = VARIANT_FALSE; }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            return m_item < 0 ? UiaHostProviderFromHwnd(m_state->bar, out) : S_OK;
        }

        HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            int target = -1;
            bool root = false;
            if (m_item < 0)
            {
                if (direction == NavigateDirection_FirstChild) target = Step(-1, 1);
                else if (direction == NavigateDirection_LastChild) target = Step(3, -1);
            }
            else if (direction == NavigateDirection_Parent) root = true;
            else if (direction == NavigateDirection_NextSibling) target = Step(m_item, 1);
            else if (direction == NavigateDirection_PreviousSibling) target = Step(m_item, -1);
            if (root) *out = new TitleBarProvider(m_state, -1);
            else if (target >= 0) *out = new TitleBarProvider(m_state, target);
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            if (m_item < 0) return S_OK;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            const int values[3] = { UiaAppendRuntimeId, static_cast<int>(reinterpret_cast<INT_PTR>(m_state->bar) & 0x7fffffff), m_item + 1 };
            *out = SafeArrayCreateVector(VT_I4, 0, 3);
            if (!*out) return E_OUTOFMEMORY;
            for (LONG i = 0; i < 3; ++i) SafeArrayPutElement(*out, &i, const_cast<int*>(&values[i]));
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* out) override
        {
            if (!out) return E_POINTER;
            *out = {};
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            RECT origin{};
            if (!GetWindowRect(m_state->bar, &origin)) return UIA_E_ELEMENTNOTAVAILABLE;
            RECT r = origin;
            if (m_item >= 0) { r = m_state->cells[m_item]; OffsetRect(&r, origin.left, origin.top); }
            *out = { double(r.left), double(r.top), double(r.right - r.left), double(r.bottom - r.top) };
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** out) override
        { if (!out) return E_POINTER; *out = nullptr; return S_OK; }
        HRESULT STDMETHODCALLTYPE SetFocus() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            *out = new TitleBarProvider(m_state, -1);
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** out) override
        {
            if (!out) return E_POINTER;
            *out = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!Available()) return UIA_E_ELEMENTNOTAVAILABLE;
            RECT origin{};
            if (!GetWindowRect(m_state->bar, &origin)) return S_OK;
            const POINT p{ static_cast<LONG>(x) - origin.left, static_cast<LONG>(y) - origin.top };
            for (int i = 0; i < 3; ++i)
                if (m_state->present[i] && PtInRect(&m_state->cells[i], p)) { *out = new TitleBarProvider(m_state, i); return S_OK; }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** out)
        { if (!out) return E_POINTER; *out = nullptr; return S_OK; }

        HRESULT STDMETHODCALLTYPE Invoke() override
        {
            HWND host{};
            WPARAM command = SC_CLOSE;
            {
                std::lock_guard<std::mutex> lock(m_state->mutex);
                if (!Available() || m_item < 0 || !m_state->present[m_item]) return UIA_E_ELEMENTNOTAVAILABLE;
                host = m_state->host;
            }
            if (m_item == 0) command = SC_MINIMIZE;
            else if (m_item == 1) command = IsZoomed(host) ? SC_RESTORE : SC_MAXIMIZE;
            return PostMessageW(host, WM_SYSCOMMAND, command, 0) ? S_OK : UIA_E_ELEMENTNOTAVAILABLE;
        }

    private:
        bool Available() const { return m_state->host && m_state->bar && IsWindow(m_state->bar); }
        int Step(int from, int delta) const
        {
            for (int i = from + delta; i >= 0 && i < 3; i += delta)
                if (m_state->present[i]) return i;
            return -1;
        }
        std::shared_ptr<TitleBarAccess> m_state;
        int m_item;
        std::atomic<ULONG> m_refs{ 1 };
    };
}
