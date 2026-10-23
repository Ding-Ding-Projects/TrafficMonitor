#pragma once
#include <UIAutomation.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <cmath>

namespace NativeMaterial
{
    constexpr UINT ValueAction = WM_APP + 0x522;
    struct ValueAccess
    {
        std::mutex mutex;
        HWND window{};
        bool range{}, enabled{true}, expanded{};
        std::wstring name;
        std::vector<std::wstring> choices;
        int value{}, minimum{1}, maximum{5};
    };
    // Providers own only synchronized value snapshots. All mutations are queued
    // to the control's UI thread; destruction revokes the shared HWND first.
    class ValueProvider final : public IRawElementProviderSimple,
        public IRangeValueProvider, public IExpandCollapseProvider,
        public ISelectionProvider, public ISelectionItemProvider
    {
    public:
        explicit ValueProvider(std::shared_ptr<ValueAccess> state,int item=-1):m_state(std::move(state)),m_item(item){if(item>=0)m_itemName=m_state->choices[item];}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override
        {
            if(!out)return E_POINTER;*out=nullptr;
            if(id==__uuidof(IUnknown)||id==__uuidof(IRawElementProviderSimple))*out=static_cast<IRawElementProviderSimple*>(this);
            else if(id==__uuidof(IRangeValueProvider)&&m_state->range)*out=static_cast<IRangeValueProvider*>(this);
            else if(!m_state->range&&m_item<0&&id==__uuidof(IExpandCollapseProvider))*out=static_cast<IExpandCollapseProvider*>(this);
            else if(!m_state->range&&m_item<0&&id==__uuidof(ISelectionProvider))*out=static_cast<ISelectionProvider*>(this);
            else if(!m_state->range&&m_item>=0&&id==__uuidof(ISelectionItemProvider))*out=static_cast<ISelectionItemProvider*>(this);
            else return E_NOINTERFACE;AddRef();return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override{return ++m_refs;}
        ULONG STDMETHODCALLTYPE Release() override{auto n=--m_refs;if(!n)delete this;return n;}
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* out) override
        {if(!out)return E_POINTER;*out=ProviderOptions_ServerSideProvider;return S_OK;}
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id,IUnknown** out) override
        {
            if(!out)return E_POINTER;*out=nullptr;
            if(m_state->range&&id==UIA_RangeValuePatternId)return QueryInterface(__uuidof(IRangeValueProvider),reinterpret_cast<void**>(out));
            if(!m_state->range&&m_item<0&&id==UIA_ExpandCollapsePatternId)return QueryInterface(__uuidof(IExpandCollapseProvider),reinterpret_cast<void**>(out));
            if(!m_state->range&&m_item<0&&id==UIA_SelectionPatternId)return QueryInterface(__uuidof(ISelectionProvider),reinterpret_cast<void**>(out));
            if(!m_state->range&&m_item>=0&&id==UIA_SelectionItemPatternId)return QueryInterface(__uuidof(ISelectionItemProvider),reinterpret_cast<void**>(out));
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id,VARIANT* out) override
        {
            if(!out)return E_POINTER;VariantInit(out);std::lock_guard<std::mutex> lock(m_state->mutex);
            if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;
            if(id==UIA_ControlTypePropertyId){out->vt=VT_I4;out->lVal=m_state->range?UIA_SliderControlTypeId:m_item>=0?UIA_ListItemControlTypeId:UIA_ComboBoxControlTypeId;}
            else if(id==UIA_NamePropertyId){out->vt=VT_BSTR;out->bstrVal=SysAllocString((m_item>=0?m_state->choices[m_item]:m_state->name).c_str());if(!out->bstrVal)return E_OUTOFMEMORY;}
            else if(id==UIA_IsEnabledPropertyId||id==UIA_IsKeyboardFocusablePropertyId||id==UIA_IsControlElementPropertyId||id==UIA_IsContentElementPropertyId)
            {out->vt=VT_BOOL;out->boolVal=(id==UIA_IsEnabledPropertyId&&!m_state->enabled)||(id==UIA_IsKeyboardFocusablePropertyId&&m_item>=0)?VARIANT_FALSE:VARIANT_TRUE;}
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** out) override
        {
            if(!out)return E_POINTER;*out=nullptr;std::lock_guard<std::mutex> lock(m_state->mutex);
            if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;
            return m_item>=0?S_OK:UiaHostProviderFromHwnd(m_state->window,out);
        }
        HRESULT STDMETHODCALLTYPE SetValue(double value) override
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;
            if(!std::isfinite(value)||value!=std::floor(value)||value<m_state->minimum||value>m_state->maximum)return E_INVALIDARG;
            return Post(0,static_cast<int>(value));
        }
        HRESULT STDMETHODCALLTYPE get_Value(double* out) override{return Number(out,0);}
        HRESULT STDMETHODCALLTYPE get_Minimum(double* out) override{return Number(out,1);}
        HRESULT STDMETHODCALLTYPE get_Maximum(double* out) override{return Number(out,2);}
        HRESULT STDMETHODCALLTYPE get_LargeChange(double* out) override{return Number(out,3);}
        HRESULT STDMETHODCALLTYPE get_SmallChange(double* out) override{return Number(out,3);}
        HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* out) override
        {if(!out)return E_POINTER;std::lock_guard<std::mutex> lock(m_state->mutex);if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;*out=FALSE;return S_OK;}
        HRESULT STDMETHODCALLTYPE Expand() override{std::lock_guard<std::mutex> lock(m_state->mutex);return Post(1,1);}
        HRESULT STDMETHODCALLTYPE Collapse() override{std::lock_guard<std::mutex> lock(m_state->mutex);return Post(1,0);}
        HRESULT STDMETHODCALLTYPE get_ExpandCollapseState(ExpandCollapseState* out) override
        {if(!out)return E_POINTER;std::lock_guard<std::mutex> lock(m_state->mutex);if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;*out=m_state->expanded?ExpandCollapseState_Expanded:ExpandCollapseState_Collapsed;return S_OK;}
        HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** out) override
        {
            if(!out)return E_POINTER;*out=nullptr;std::lock_guard<std::mutex> lock(m_state->mutex);if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;
            const bool selected=m_state->value>=0&&m_state->value<static_cast<int>(m_state->choices.size());
            *out=SafeArrayCreateVector(VT_UNKNOWN,0,selected?1:0);if(!*out)return E_OUTOFMEMORY;
            if(selected){auto* provider=new ValueProvider(m_state,m_state->value);LONG i=0;IUnknown* value=static_cast<IRawElementProviderSimple*>(provider);HRESULT hr=SafeArrayPutElement(*out,&i,value);provider->Release();if(FAILED(hr)){SafeArrayDestroy(*out);*out=nullptr;return hr;}}
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* out) override{if(!out)return E_POINTER;*out=FALSE;return S_OK;}
        HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* out) override{if(!out)return E_POINTER;*out=FALSE;return S_OK;}
        HRESULT STDMETHODCALLTYPE Select() override{std::lock_guard<std::mutex> lock(m_state->mutex);return Post(0,m_item);}
        HRESULT STDMETHODCALLTYPE AddToSelection() override{return Select();}
        HRESULT STDMETHODCALLTYPE RemoveFromSelection() override{return UIA_E_INVALIDOPERATION;}
        HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* out) override
        {if(!out)return E_POINTER;std::lock_guard<std::mutex> lock(m_state->mutex);if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;*out=m_item==m_state->value;return S_OK;}
        HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** out) override
        {if(!out)return E_POINTER;*out=nullptr;std::lock_guard<std::mutex> lock(m_state->mutex);if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;*out=new ValueProvider(m_state);return S_OK;}
    private:
        bool Available() const{return m_state->window&&(m_item<0||(m_item<static_cast<int>(m_state->choices.size())&&m_itemName==m_state->choices[m_item]));}
        HRESULT Post(WPARAM action,int value)
        {if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;if(!m_state->enabled)return UIA_E_ELEMENTNOTENABLED;return PostMessageW(m_state->window,ValueAction,action,value)?S_OK:HRESULT_FROM_WIN32(GetLastError());}
        HRESULT Number(double* out,int field)
        {if(!out)return E_POINTER;std::lock_guard<std::mutex> lock(m_state->mutex);if(!Available())return UIA_E_ELEMENTNOTAVAILABLE;*out=field==0?m_state->value:field==1?m_state->minimum:field==2?m_state->maximum:1;return S_OK;}
        std::atomic<ULONG> m_refs{1};std::shared_ptr<ValueAccess> m_state;int m_item;std::wstring m_itemName;
    };
    inline LRESULT ReturnValueProvider(HWND window,WPARAM w,LPARAM l,const std::shared_ptr<ValueAccess>& state)
    {auto* provider=new ValueProvider(state);const auto result=UiaReturnRawElementProvider(window,w,l,provider);provider->Release();return result;}
}
