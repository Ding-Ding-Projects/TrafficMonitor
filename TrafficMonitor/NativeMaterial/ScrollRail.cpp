#include "../stdafx.h"
#include "ScrollRail.h"
#include <UIAutomation.h>
#include <atomic>
#include <mutex>
#include <cmath>
#include <windowsx.h>
namespace NativeMaterial
{
    constexpr UINT SetScrollValue=WM_APP+0x529;
    struct ScrollAccess { std::mutex mutex; HWND window{}; std::wstring name{L"Scroll"}; int maximum{},page{},position{},step{32}; };
    class ScrollProvider final : public IRawElementProviderSimple, public IRangeValueProvider
    {
    public:
        explicit ScrollProvider(std::shared_ptr<ScrollAccess> s):state(std::move(s)){}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override
        {if(!out)return E_POINTER;*out=nullptr;if(id==__uuidof(IUnknown)||id==__uuidof(IRawElementProviderSimple))*out=static_cast<IRawElementProviderSimple*>(this);else if(id==__uuidof(IRangeValueProvider))*out=static_cast<IRangeValueProvider*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
        ULONG STDMETHODCALLTYPE AddRef() override{return ++refs;}
        ULONG STDMETHODCALLTYPE Release() override{auto n=--refs;if(!n)delete this;return n;}
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* out) override{if(!out)return E_POINTER;*out=ProviderOptions_ServerSideProvider;return S_OK;}
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id,IUnknown** out) override
        {if(!out)return E_POINTER;*out=nullptr;return id==UIA_RangeValuePatternId?QueryInterface(__uuidof(IRangeValueProvider),reinterpret_cast<void**>(out)):S_OK;}
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id,VARIANT* out) override
        {
            if(!out)return E_POINTER;VariantInit(out);std::lock_guard<std::mutex> lock(state->mutex);if(!state->window)return UIA_E_ELEMENTNOTAVAILABLE;
            if(id==UIA_ControlTypePropertyId){out->vt=VT_I4;out->lVal=UIA_ScrollBarControlTypeId;}
            else if(id==UIA_OrientationPropertyId){out->vt=VT_I4;out->lVal=OrientationType_Vertical;}
            else if(id==UIA_NamePropertyId){out->vt=VT_BSTR;out->bstrVal=SysAllocString(state->name.c_str());}
            else if(id==UIA_IsEnabledPropertyId||id==UIA_IsKeyboardFocusablePropertyId||id==UIA_IsControlElementPropertyId||id==UIA_IsContentElementPropertyId){out->vt=VT_BOOL;out->boolVal=VARIANT_TRUE;}
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** out) override
        {if(!out)return E_POINTER;std::lock_guard<std::mutex> lock(state->mutex);return state->window?UiaHostProviderFromHwnd(state->window,out):UIA_E_ELEMENTNOTAVAILABLE;}
        HRESULT STDMETHODCALLTYPE SetValue(double value) override
        {std::lock_guard<std::mutex> lock(state->mutex);if(!state->window)return UIA_E_ELEMENTNOTAVAILABLE;if(!std::isfinite(value)||value<0||value>state->maximum)return E_INVALIDARG;return PostMessageW(state->window,SetScrollValue,static_cast<WPARAM>(value),0)?S_OK:HRESULT_FROM_WIN32(GetLastError());}
        HRESULT STDMETHODCALLTYPE get_Value(double* out) override{return Number(out,0);}
        HRESULT STDMETHODCALLTYPE get_Minimum(double* out) override{return Number(out,1);}
        HRESULT STDMETHODCALLTYPE get_Maximum(double* out) override{return Number(out,2);}
        HRESULT STDMETHODCALLTYPE get_LargeChange(double* out) override{return Number(out,3);}
        HRESULT STDMETHODCALLTYPE get_SmallChange(double* out) override{return Number(out,4);}
        HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* out) override{if(!out)return E_POINTER;*out=FALSE;return S_OK;}
    private:
        HRESULT Number(double* out,int part){if(!out)return E_POINTER;std::lock_guard<std::mutex> lock(state->mutex);if(!state->window)return UIA_E_ELEMENTNOTAVAILABLE;*out=part==0?state->position:part==1?0:part==2?state->maximum:part==3?state->page:state->step;return S_OK;}
        std::atomic<ULONG> refs{1};std::shared_ptr<ScrollAccess> state;
    };
    BOOL ScrollRail::Create(CWnd* parent,UINT id)
    {
        m_access=std::make_shared<ScrollAccess>();
        const BOOL ok=CreateEx(0,AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,::LoadCursor(nullptr,IDC_ARROW)),L"Scroll",WS_CHILD|WS_TABSTOP,CRect(0,0,12,100),parent,id);
        if(ok)m_access->window=m_hWnd;return ok;
    }
    void ScrollRail::SetStep(int step){m_step=(std::max)(1,step);if(m_access){std::lock_guard<std::mutex> lock(m_access->mutex);m_access->step=m_step;}}
    void ScrollRail::SetMetrics(int content,int page,int position)
    {
        m_content=(std::max)(0,content);m_page=(std::max)(0,page);m_position=(std::max)(0,(std::min)(position,(std::max)(0,m_content-m_page)));
        {std::lock_guard<std::mutex> lock(m_access->mutex);m_access->maximum=(std::max)(0,m_content-m_page);m_access->page=m_page;m_access->position=m_position;}
        const bool visible=m_content>m_page&&m_page>0;if(!visible&&GetFocus()==this)GetParent()->SetFocus();ShowWindow(visible?SW_SHOW:SW_HIDE);Invalidate(FALSE);
    }
    CRect ScrollRail::Thumb() const
    {
        CRect r;GetClientRect(r);const int height=r.Height(),minimum=MulDiv(32,GetDpiForWindow(m_hWnd),96);
        const int length=(std::min)(height,(std::max)(minimum,m_content?MulDiv(height,m_page,m_content):height));
        const int top=m_content>m_page?MulDiv(height-length,m_position,m_content-m_page):0;
        const int inset=m_hover||m_drag?2:3;return CRect(inset,top,(std::max)(inset+1,r.Width()-inset),top+length);
    }
    void ScrollRail::Paint(HDC dc)
    {
        CDC* d=CDC::FromHandle(dc);CRect r;GetClientRect(r);const auto c=Palette::Resolve(m_appearance);auto rgb=[](D2D1_COLOR_F v){return RGB(BYTE(v.r*255),BYTE(v.g*255),BYTE(v.b*255));};
        d->FillSolidRect(r,rgb(c.container));CRect thumb=Thumb();CBrush brush(rgb(m_drag?c.primary:c.secondaryText));auto* old=d->SelectObject(&brush);auto* pen=d->SelectStockObject(NULL_PEN);d->RoundRect(thumb,CPoint(6,6));d->SelectObject(old);d->SelectObject(pen);if(GetFocus()==this||c.highContrast)d->DrawFocusRect(r);
    }
    void ScrollRail::Move(int position)
    {const int next=(std::max)(0,(std::min)(position,(std::max)(0,m_content-m_page)));if(next!=m_position){m_position=next;{std::lock_guard<std::mutex> lock(m_access->mutex);m_access->position=next;}if(changed)changed(next);::NotifyWinEvent(EVENT_OBJECT_VALUECHANGE,m_hWnd,OBJID_CLIENT,CHILDID_SELF);Invalidate(FALSE);}}
    LRESULT ScrollRail::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_PAINT){CPaintDC dc(this);Paint(dc);return 0;}if(message==WM_PRINTCLIENT){Paint(reinterpret_cast<HDC>(w));return 0;}if(message==WM_ERASEBKGND)return TRUE;
        if(message==WM_SETTEXT&&m_access){std::lock_guard<std::mutex> lock(m_access->mutex);m_access->name=l?reinterpret_cast<const wchar_t*>(l):L"";}
        if(message==WM_GETOBJECT&&l==UiaRootObjectId){auto* p=new ScrollProvider(m_access);auto result=UiaReturnRawElementProvider(m_hWnd,w,l,p);p->Release();return result;}
        if(message==SetScrollValue){Move(static_cast<int>(w));return 0;}
        if(message==WM_LBUTTONDOWN){SetFocus();const CPoint point(GET_X_LPARAM(l),GET_Y_LPARAM(l));auto thumb=Thumb();if(thumb.PtInRect(point)){m_drag=true;m_dragOrigin=point.y;m_dragPosition=m_position;SetCapture();}else Move(m_position+(point.y<thumb.top?-m_page:m_page));Invalidate();return 0;}
        if(message==WM_MOUSEMOVE){m_hover=true;TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,m_hWnd,0};TrackMouseEvent(&track);if(m_drag){CRect r;GetClientRect(r);const int travel=r.Height()-Thumb().Height();if(travel>0)Move(m_dragPosition+MulDiv(GET_Y_LPARAM(l)-m_dragOrigin,m_content-m_page,travel));}Invalidate(FALSE);return 0;}
        if(message==WM_MOUSELEAVE){m_hover=false;Invalidate(FALSE);return 0;}
        if(message==WM_LBUTTONUP){m_drag=false;if(GetCapture()==this)ReleaseCapture();Invalidate();return 0;}
        if(message==WM_CAPTURECHANGED){m_drag=false;Invalidate();}
        if(message==WM_SETFOCUS||message==WM_KILLFOCUS)Invalidate();
        if(message==WM_GETDLGCODE)return DLGC_WANTARROWS;
        if(message==WM_KEYDOWN){switch(w){case VK_UP:Move(m_position-m_step);break;case VK_DOWN:Move(m_position+m_step);break;case VK_PRIOR:Move(m_position-m_page);break;case VK_NEXT:Move(m_position+m_page);break;case VK_HOME:Move(0);break;case VK_END:Move(m_content);break;default:return CWnd::WindowProc(message,w,l);}return 0;}
        if(message==WM_MOUSEWHEEL){m_wheel+=GET_WHEEL_DELTA_WPARAM(w);while(abs(m_wheel)>=WHEEL_DELTA){const int direction=m_wheel>0?1:-1;Move(m_position-direction*m_step*3);m_wheel-=direction*WHEEL_DELTA;}return TRUE;}
        if(message==WM_NCDESTROY&&m_access){std::lock_guard<std::mutex> lock(m_access->mutex);m_access->window=nullptr;}
        return CWnd::WindowProc(message,w,l);
    }
}
