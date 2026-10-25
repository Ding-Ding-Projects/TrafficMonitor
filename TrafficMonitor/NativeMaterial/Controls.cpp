#include "../stdafx.h"
#include "Controls.h"
#include <UIAutomation.h>
#include <atomic>
#include <mutex>
#include "Controls.Accessibility.h"
#pragma comment(lib, "uiautomationcore.lib")

namespace NativeMaterial
{
    namespace
    {
        COLORREF Color(D2D1_COLOR_F c){return RGB(BYTE(c.r*255),BYTE(c.g*255),BYTE(c.b*255));}
    }
    BOOL Choice::Create(DWORD style,const RECT& bounds,CWnd* parent,UINT id)
    {
        m_access=std::make_shared<ValueAccess>();
        const BOOL ok=CComboBox::Create(style|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS,bounds,parent,id);
        if(ok){const int height=MulDiv(36,GetDpiForWindow(m_hWnd),96);SetItemHeight(-1,height);SetItemHeight(0,height);}
        return ok;
    }
    void Choice::MeasureItem(LPMEASUREITEMSTRUCT item)
    {item->itemHeight=MulDiv(36,GetDpiForWindow(m_hWnd),96);}
    void Choice::DrawItem(LPDRAWITEMSTRUCT item)
    {
        if(!item->hDC)return;
        CDC dc;dc.Attach(item->hDC);CRect rect(item->rcItem);
        const auto colors=Palette::Resolve(m_appearance);
        const bool selected=(item->itemState&ODS_SELECTED)!=0;
        dc.FillSolidRect(rect,Color(selected?colors.primary:colors.container));
        dc.SetBkMode(TRANSPARENT);dc.SetTextColor(Color(!IsWindowEnabled()?colors.secondaryText:selected?colors.onPrimary:colors.onSurface));
        CString value;if(item->itemID!=UINT(-1))GetLBText(item->itemID,value);
        CFont* old=GetFont()?dc.SelectObject(GetFont()):nullptr;
        rect.DeflateRect(MulDiv(12,GetDpiForWindow(m_hWnd),96),0);
        dc.DrawText(value,rect,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
        if(item->itemState&ODS_FOCUS)dc.DrawFocusRect(rect);
        if(old)dc.SelectObject(old);dc.Detach();
    }
    BEGIN_MESSAGE_MAP(RangeValue,CSliderCtrl)
        ON_NOTIFY_REFLECT(NM_CUSTOMDRAW,&RangeValue::OnCustomDraw)
    END_MESSAGE_MAP()
    BOOL RangeValue::Create(const RECT& bounds,CWnd* parent,UINT id)
    {
        m_access=std::make_shared<ValueAccess>();m_access->range=true;
        const BOOL ok=CSliderCtrl::Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,bounds,parent,id);
        if(ok){SetRange(1,5);SetLineSize(1);SetPageSize(1);SetPos(5);}
        return ok;
    }
    LRESULT Choice::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_GETOBJECT&&l==UiaRootObjectId&&m_access)return ReturnValueProvider(m_hWnd,w,l,m_access);
        if(message==WM_NCDESTROY&&m_access){std::lock_guard<std::mutex> lock(m_access->mutex);m_access->window=nullptr;}
        if(message==ValueAction)
        {
            if(!IsWindowEnabled())return 0;
            if(w==1)ShowDropDown(l!=0);
            else if(l>=0&&l<GetCount())
            {SetCurSel(static_cast<int>(l));GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),CBN_SELCHANGE),reinterpret_cast<LPARAM>(m_hWnd));}
            return 0;
        }
        const auto result=CComboBox::WindowProc(message,w,l);
        if(m_access&&GetSafeHwnd()&&(message==CB_SETCURSEL||message==CB_ADDSTRING||message==CB_RESETCONTENT
            ||message==WM_KEYDOWN||message==WM_KEYUP||message==WM_LBUTTONUP||message==CB_SHOWDROPDOWN||message==WM_ENABLE||message==WM_SETTEXT))
        {
            std::vector<std::wstring> choices;CString text,name;
            for(int i=0;i<GetCount();++i){GetLBText(i,text);choices.emplace_back(text.GetString());}
            if(auto* label=GetWindow(GW_HWNDPREV))label->GetWindowText(name);
            const int selected=GetCurSel();const bool expanded=GetDroppedState()!=FALSE;
            bool changed=false;
            {std::lock_guard<std::mutex> lock(m_access->mutex);changed=m_access->value!=selected||m_access->expanded!=expanded;
                m_access->window=m_hWnd;m_access->choices=std::move(choices);
                m_access->name=name.GetString();m_access->value=selected;m_access->expanded=expanded;m_access->enabled=IsWindowEnabled()!=FALSE;}
            if(changed)::NotifyWinEvent(EVENT_OBJECT_VALUECHANGE,m_hWnd,OBJID_CLIENT,CHILDID_SELF);
        }
        return result;
    }
    LRESULT RangeValue::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_GETOBJECT&&l==UiaRootObjectId&&m_access)return ReturnValueProvider(m_hWnd,w,l,m_access);
        if(message==WM_NCDESTROY&&m_access){std::lock_guard<std::mutex> lock(m_access->mutex);m_access->window=nullptr;}
        if(message==ValueAction)
        {
            if(IsWindowEnabled()&&l>=GetRangeMin()&&l<=GetRangeMax())
            {SetPos(static_cast<int>(l));GetParent()->SendMessage(WM_HSCROLL,MAKEWPARAM(TB_THUMBPOSITION,static_cast<UINT>(l)),reinterpret_cast<LPARAM>(m_hWnd));}
            return 0;
        }
        const auto result=CSliderCtrl::WindowProc(message,w,l);
        if(m_access&&GetSafeHwnd()&&(message==TBM_SETPOS||message==TBM_SETRANGE||message==WM_KEYDOWN
            ||message==WM_KEYUP||message==WM_LBUTTONUP||message==WM_MOUSEMOVE||message==WM_ENABLE||message==WM_SETTEXT))
        {
            CString name;GetWindowText(name);const int value=GetPos(),minimum=GetRangeMin(),maximum=GetRangeMax();
            bool changed=false;
            {std::lock_guard<std::mutex> lock(m_access->mutex);changed=m_access->value!=value;
                m_access->window=m_hWnd;m_access->name=name.GetString();
                m_access->value=value;m_access->minimum=minimum;m_access->maximum=maximum;m_access->enabled=IsWindowEnabled()!=FALSE;}
            if(changed)::NotifyWinEvent(EVENT_OBJECT_VALUECHANGE,m_hWnd,OBJID_CLIENT,CHILDID_SELF);
        }
        return result;
    }
    void RangeValue::OnCustomDraw(NMHDR* header,LRESULT* result)
    {
        auto* draw=reinterpret_cast<NMCUSTOMDRAW*>(header);
        *result=CDRF_DODEFAULT;
        if(draw->dwDrawStage!=CDDS_PREPAINT)return;
        CDC dc;dc.Attach(draw->hdc);CRect bounds;GetClientRect(bounds);
        const auto colors=Palette::Resolve(m_appearance);
        dc.FillSolidRect(bounds,Color(colors.surface));
        CRect channel,thumb;GetChannelRect(channel);GetThumbRect(thumb);
        const int dpi=GetDpiForWindow(m_hWnd),radius=MulDiv(10,dpi,96);
        const int trackY=bounds.bottom-MulDiv(12,dpi,96);
        channel.top=trackY-MulDiv(2,dpi,96);channel.bottom=channel.top+MulDiv(4,dpi,96);
        dc.FillSolidRect(channel,Color(colors.secondaryContainer));
        CRect active=channel;active.right=thumb.CenterPoint().x;
        dc.FillSolidRect(active,Color(IsWindowEnabled()?colors.primary:colors.outline));
        // Draw the existing discrete range without changing native key steps,
        // thumb geometry or provider values. Bound arbitrary external ranges.
        const int minimum=GetRangeMin(),maximum=GetRangeMax();
        if(maximum>minimum&&static_cast<long long>(maximum)-minimum<=100)
        {
            const int tick=(std::max)(1,MulDiv(1,dpi,96));
            for(int offset=0;offset<=maximum-minimum;++offset)
            {
                const int value=minimum+offset;
                const int x=channel.left+MulDiv(offset,channel.Width(),maximum-minimum);
                const auto ink=IsWindowEnabled()?(value<=GetPos()?colors.onPrimary:colors.onSecondaryContainer):colors.onSurfaceVariant;
                dc.FillSolidRect(x-tick,trackY-tick,tick*2,tick*2,Color(ink));
            }
        }
        const auto center=CPoint(thumb.CenterPoint().x,trackY);
        CBrush brush(Color(IsWindowEnabled()?colors.primary:colors.outline));
        auto oldBrush=dc.SelectObject(&brush);auto oldPen=dc.SelectStockObject(NULL_PEN);
        dc.Ellipse(center.x-radius,center.y-radius,center.x+radius,center.y+radius);
        dc.SelectObject(oldBrush);dc.SelectObject(oldPen);
        CString value;value.Format(L"%d",GetPos());dc.SetBkMode(TRANSPARENT);dc.SetTextColor(Color(colors.onSurface));
        const int textWidth=MulDiv(28,dpi,96);
        const int textLeft=(std::max<LONG>)(0,(std::min<LONG>)(bounds.right-textWidth,center.x-textWidth/2));
        CRect number(textLeft,0,textLeft+textWidth,MulDiv(20,dpi,96));
        dc.DrawText(value,number,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        if(GetFocus()==this){bounds.DeflateRect(1,1);dc.DrawFocusRect(bounds);}
        dc.Detach();*result=CDRF_SKIPDEFAULT;
    }
    BEGIN_MESSAGE_MAP(ReadOnlyText,CEdit)
        ON_WM_CTLCOLOR_REFLECT()
    END_MESSAGE_MAP()
    BOOL ReadOnlyText::Create(const RECT& bounds,CWnd* parent,UINT id)
    {return CEdit::Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,bounds,parent,id);}
    void ReadOnlyText::SetAppearance(Appearance value)
    {m_appearance=value;m_background.DeleteObject();m_background.CreateSolidBrush(Color(Palette::Resolve(value).container));Invalidate();}
    HBRUSH ReadOnlyText::CtlColor(CDC* dc,UINT)
    {const auto colors=Palette::Resolve(m_appearance);dc->SetTextColor(Color(colors.onSurface));dc->SetBkColor(Color(colors.container));return static_cast<HBRUSH>(m_background.GetSafeHandle());}
    LRESULT ReadOnlyText::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        const auto result=CEdit::WindowProc(message,w,l);
        if(message==WM_GETDLGCODE)
        {
            const auto* key=reinterpret_cast<const MSG*>(l);
            const bool tab=w==VK_TAB||(key&&key->wParam==VK_TAB&&
                (key->message==WM_KEYDOWN||key->message==WM_KEYUP||key->message==WM_CHAR));
            if(tab)return result&~(DLGC_WANTALLKEYS|DLGC_WANTTAB);
        }
        return result;
    }

    struct ButtonAccess
    {
        std::mutex mutex;
        HWND window{};
        std::wstring label;
        std::wstring accessibleName, description;
        bool disclosure{}, expanded{};
        bool enabled{ true };
    };

    // Each real child HWND participates in the system HWND fragment tree.
    // Custom virtual tables require their own fragment providers separately.
    constexpr UINT DisclosureAction=WM_APP+0x524;
    class ButtonProvider final : public IRawElementProviderSimple, public IInvokeProvider, public IExpandCollapseProvider
    {
    public:
        explicit ButtonProvider(std::shared_ptr<ButtonAccess> state) : m_state(std::move(state)) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override
        {
            if (!value) return E_POINTER;
            *value = nullptr;
            if (iid == __uuidof(IUnknown) || iid == __uuidof(IRawElementProviderSimple))
                *value = static_cast<IRawElementProviderSimple*>(this);
            else if (iid == __uuidof(IInvokeProvider)) *value = static_cast<IInvokeProvider*>(this);
            else if (iid == __uuidof(IExpandCollapseProvider))
            {
                std::lock_guard<std::mutex> lock(m_state->mutex);
                if(!m_state->disclosure)return E_NOINTERFACE;
                *value=static_cast<IExpandCollapseProvider*>(this);
            }
            else return E_NOINTERFACE;
            AddRef(); return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++m_references; }
        ULONG STDMETHODCALLTYPE Release() override
        {
            const auto remaining = --m_references;
            if (!remaining) delete this;
            return remaining;
        }
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* value) override
        {
            if (!value) return E_POINTER;
            *value = ProviderOptions_ServerSideProvider; return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id, IUnknown** value) override
        {
            if (!value) return E_POINTER;
            *value = nullptr;
            if (id == UIA_InvokePatternId) return QueryInterface(__uuidof(IInvokeProvider), reinterpret_cast<void**>(value));
            if (id == UIA_ExpandCollapsePatternId)
            {
                const auto result=QueryInterface(__uuidof(IExpandCollapseProvider),reinterpret_cast<void**>(value));
                return result==E_NOINTERFACE?S_OK:result;
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* value) override
        {
            if (!value) return E_POINTER;
            VariantInit(value);
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            if (id == UIA_ControlTypePropertyId) { value->vt = VT_I4; value->lVal = UIA_ButtonControlTypeId; }
            else if (id == UIA_NamePropertyId)
            {
                value->vt = VT_BSTR; value->bstrVal = SysAllocString((m_state->accessibleName.empty()?m_state->label:m_state->accessibleName).c_str());
                if (!value->bstrVal) return E_OUTOFMEMORY;
            }
            else if(id==UIA_HelpTextPropertyId&&m_state->disclosure)
            {value->vt=VT_BSTR;value->bstrVal=SysAllocString(m_state->description.c_str());if(!value->bstrVal)return E_OUTOFMEMORY;}
            else if(id==UIA_ExpandCollapseExpandCollapseStatePropertyId&&m_state->disclosure)
            {value->vt=VT_I4;value->lVal=m_state->expanded?ExpandCollapseState_Expanded:ExpandCollapseState_Collapsed;}
            else if(id==UIA_IsExpandCollapsePatternAvailablePropertyId)
            {value->vt=VT_BOOL;value->boolVal=m_state->disclosure?VARIANT_TRUE:VARIANT_FALSE;}
            else if (id == UIA_IsEnabledPropertyId || id == UIA_IsKeyboardFocusablePropertyId
                || id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId)
            {
                value->vt = VT_BOOL;
                value->boolVal = id == UIA_IsEnabledPropertyId && !m_state->enabled ? VARIANT_FALSE : VARIANT_TRUE;
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** value) override
        {
            if (!value) return E_POINTER;
            *value = nullptr;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            return UiaHostProviderFromHwnd(m_state->window, value);
        }
        HRESULT STDMETHODCALLTYPE Invoke() override
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if (!m_state->window) return UIA_E_ELEMENTNOTAVAILABLE;
            if (!m_state->enabled) return UIA_E_ELEMENTNOTENABLED;
            return PostMessageW(m_state->window, BM_CLICK, 0, 0) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
        HRESULT STDMETHODCALLTYPE Expand() override{return SetExpanded(true);}
        HRESULT STDMETHODCALLTYPE Collapse() override{return SetExpanded(false);}
        HRESULT STDMETHODCALLTYPE get_ExpandCollapseState(ExpandCollapseState* value) override
        {
            if(!value)return E_POINTER;
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if(!m_state->window)return UIA_E_ELEMENTNOTAVAILABLE;
            *value=m_state->expanded?ExpandCollapseState_Expanded:ExpandCollapseState_Collapsed;
            return S_OK;
        }
    private:
        HRESULT SetExpanded(bool expanded)
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            if(!m_state->window)return UIA_E_ELEMENTNOTAVAILABLE;
            if(!m_state->enabled)return UIA_E_ELEMENTNOTENABLED;
            return PostMessageW(m_state->window,DisclosureAction,expanded,0)?S_OK:HRESULT_FROM_WIN32(GetLastError());
        }
        std::atomic<ULONG> m_references{ 1 };
        std::shared_ptr<ButtonAccess> m_state;
    };

    BEGIN_MESSAGE_MAP(Button, CWnd)
        ON_WM_PAINT()
        ON_WM_ERASEBKGND()
        ON_WM_LBUTTONDOWN()
        ON_WM_LBUTTONUP()
        ON_WM_MOUSEMOVE()
        ON_WM_CAPTURECHANGED()
        ON_MESSAGE(WM_MOUSELEAVE, &Button::OnMouseLeave)
        ON_WM_KEYDOWN()
        ON_WM_KEYUP()
        ON_WM_SETFOCUS()
        ON_WM_KILLFOCUS()
        ON_WM_ENABLE()
        ON_WM_DESTROY()
        ON_WM_GETDLGCODE()
        ON_MESSAGE(WM_GETOBJECT, &Button::OnGetObject)
        ON_MESSAGE(BM_CLICK, &Button::OnClick)
        ON_MESSAGE(DisclosureAction, &Button::OnDisclosure)
        ON_MESSAGE(WM_PRINTCLIENT, &Button::OnPrintClient)
        ON_MESSAGE(WM_PRINT, &Button::OnPrintClient)
        ON_MESSAGE(WM_APP + 0x521, &Button::OnRenderStatus)
    END_MESSAGE_MAP()

    BOOL Button::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_access = std::make_shared<ButtonAccess>();
        m_access->label = label.GetString();
        const auto className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_ARROW));
        if (!CWnd::CreateEx(0, className, label, WS_CHILD | WS_VISIBLE | WS_TABSTOP, bounds, parent, id)) return FALSE;
        m_access->window = m_hWnd;
        return TRUE;
    }
    void Button::SetAppearance(Appearance appearance) { m_appearance = appearance; Invalidate(); }
    void Button::SetLabel(const CString& label)
    {
        SetWindowText(label);
        { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->label = label.GetString(); }
        Invalidate();
    }
    void Button::SetDisclosure(bool expanded,const CString& name,const CString& description)
    {
        bool changed=false,previous=false;
        {
            std::lock_guard<std::mutex> lock(m_access->mutex);
            previous=m_access->expanded;changed=m_access->disclosure&&previous!=expanded;
            m_access->disclosure=true;m_access->expanded=expanded;
            m_access->accessibleName=name.GetString();m_access->description=description.GetString();
        }
        if(changed)
        {
            auto* provider=new ButtonProvider(m_access);
            VARIANT before{},after{};before.vt=after.vt=VT_I4;
            before.lVal=previous?ExpandCollapseState_Expanded:ExpandCollapseState_Collapsed;
            after.lVal=expanded?ExpandCollapseState_Expanded:ExpandCollapseState_Collapsed;
            UiaRaiseAutomationPropertyChangedEvent(provider,UIA_ExpandCollapseExpandCollapseStatePropertyId,before,after);
            provider->Release();
            ::NotifyWinEvent(EVENT_OBJECT_STATECHANGE,m_hWnd,OBJID_CLIENT,CHILDID_SELF);
        }
    }
    LRESULT Button::OnDisclosure(WPARAM expanded,LPARAM)
    {
        bool change=false;
        {std::lock_guard<std::mutex> lock(m_access->mutex);change=m_access->disclosure&&m_access->expanded!=(expanded!=0);}
        if(change)OnClick(0,0);
        return 0;
    }
    BOOL Button::OnEraseBkgnd(CDC*) { return TRUE; }
    void Button::OnPaint()
    {
        CPaintDC paint(this);
        // Paint through the actual native paint DC. This also honors redirected
        // WM_PAINT requests used by native window printing on hidden desktops.
        m_drawResult=DrawDC(paint.GetSafeHdc());
    }
    void Button::Draw(ID2D1RenderTarget* target)
    {
        CString label;GetWindowText(label);
        DrawButton(target,Palette::Resolve(m_appearance),label,m_variant,IsWindowEnabled()!=FALSE,m_pressed,m_hover,GetFocus()==this);
    }
    LRESULT Button::OnPrintClient(WPARAM dc, LPARAM flags)
    {
        if(!dc||((flags&PRF_CHECKVISIBLE)&&!IsWindowVisible()))return 0;
        m_printResult=DrawDC(reinterpret_cast<HDC>(dc));
        return 0;
    }
    HRESULT Button::DrawDC(HDC dc)
    {
        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
        HRESULT result=D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,factory.GetAddressOf());
        const float dpi=static_cast<float>(GetDpiForWindow(m_hWnd));
        auto properties=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE),dpi,dpi);
        if(SUCCEEDED(result))result=factory->CreateDCRenderTarget(&properties,target.GetAddressOf());
        m_attachResult=result;
        CRect bounds;GetClientRect(bounds);
        if(SUCCEEDED(result))result=target->BindDC(dc,bounds);
        if(SUCCEEDED(result))
        {
            target->BeginDraw();
            const auto surface=m_customSurface
                ?D2D1::ColorF(GetRValue(m_surfaceColor)/255.f,GetGValue(m_surfaceColor)/255.f,GetBValue(m_surfaceColor)/255.f)
                :Palette::Resolve(m_appearance).surface;
            target->Clear(surface);
            Draw(target.Get());result=target->EndDraw();
        }
        return result;
    }
    LRESULT Button::OnRenderStatus(WPARAM field, LPARAM)
    {return field==0?m_attachResult:field==1?m_drawResult:m_printResult;}
    void Button::OnLButtonDown(UINT, CPoint) { if (IsWindowEnabled()) { SetFocus(); SetCapture(); m_pressed = true; Invalidate(); } }
    void Button::OnMouseMove(UINT flags,CPoint point)
    {
        CRect bounds;GetClientRect(bounds);m_hover=bounds.PtInRect(point)!=FALSE;
        TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,m_hWnd,0};TrackMouseEvent(&track);Invalidate(FALSE);
        CWnd::OnMouseMove(flags,point);
    }
    LRESULT Button::OnMouseLeave(WPARAM,LPARAM){m_hover=false;Invalidate(FALSE);return 0;}
    void Button::OnCaptureChanged(CWnd* next){m_pressed=false;Invalidate(FALSE);CWnd::OnCaptureChanged(next);}
    void Button::OnLButtonUp(UINT, CPoint point)
    {
        const bool pressed = m_pressed; m_pressed = false;
        if (GetCapture() == this) ReleaseCapture();
        Invalidate(); CRect bounds; GetClientRect(bounds);
        if (pressed && bounds.PtInRect(point)) OnClick(0, 0);
    }
    UINT Button::OnGetDlgCode()
    {
        // A disclosure consumes its activation keys instead of letting the
        // dialog manager route Enter to an unrelated default action.
        const auto* message=GetCurrentMessage();
        std::lock_guard<std::mutex> lock(m_access->mutex);
        if(m_access->disclosure&&message&&(message->wParam==VK_RETURN||message->wParam==VK_SPACE))
            return DLGC_BUTTON|DLGC_WANTMESSAGE;
        return DLGC_BUTTON;
    }
    void Button::OnKeyDown(UINT key, UINT repeat, UINT flags)
    {
        if ((key == VK_SPACE || key == VK_RETURN) && !(flags & 0x4000)) { m_pressed = true; Invalidate(); }
        else CWnd::OnKeyDown(key, repeat, flags);
    }
    void Button::OnKeyUp(UINT key, UINT repeat, UINT flags)
    {
        if ((key == VK_SPACE || key == VK_RETURN) && m_pressed) { m_pressed = false; Invalidate(); OnClick(0, 0); }
        else CWnd::OnKeyUp(key, repeat, flags);
    }
    void Button::OnSetFocus(CWnd* old) { CWnd::OnSetFocus(old); Invalidate(); ::NotifyWinEvent(EVENT_OBJECT_FOCUS, m_hWnd, OBJID_CLIENT, CHILDID_SELF); }
    void Button::OnKillFocus(CWnd* next) { m_pressed = false; CWnd::OnKillFocus(next); Invalidate(); }
    void Button::OnEnable(BOOL enabled)
    {
        CWnd::OnEnable(enabled);
        { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->enabled = enabled != FALSE; }
        m_pressed = false; Invalidate();
    }
    void Button::OnDestroy()
    {
        { std::lock_guard<std::mutex> lock(m_access->mutex); m_access->window = nullptr; }
        m_canvas.Detach(); CWnd::OnDestroy();
    }
    LRESULT Button::OnGetObject(WPARAM wparam, LPARAM lparam)
    {
        if (lparam != UiaRootObjectId) return Default();
        auto provider = new ButtonProvider(m_access);
        const LRESULT result = UiaReturnRawElementProvider(m_hWnd, wparam, lparam, provider);
        provider->Release(); return result;
    }
    LRESULT Button::OnClick(WPARAM, LPARAM)
    {
        if (IsWindowEnabled() && GetParent())
            GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
        return 0;
    }
}
