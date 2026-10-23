#include "stdafx.h"
#include "SearchableMenu.h"
#include "../TrafficMonitor.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <imm.h>
#include <oleacc.h>
#include <memory>
#include <cwctype>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "msimg32.lib")

namespace NativeMenus
{
namespace
{
constexpr UINT EditId = 51001, ListId = 51002, BackId = 51003;
struct Item
{
    UINT id{}, type{}, state{}, position{};
    HMENU submenu{};
    HBITMAP bitmap{};
    std::wstring text, label, shortcut;
    bool Separator() const { return (type & MFT_SEPARATOR) != 0; }
    bool Enabled() const { return !Separator() && !(state & (MFS_DISABLED | MFS_GRAYED)); }
};
struct Level
{
    HMENU menu{};
    UINT parentPosition{};
    std::wstring query;
    int selected{};
    std::vector<Item> items;
};

std::wstring VisibleText(const std::wstring& input)
{
    std::wstring output;
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] == L'&')
        {
            if (i + 1 < input.size() && input[i + 1] == L'&') { output += L'&'; ++i; }
        }
        else output += input[i];
    }
    return output;
}

bool ReadItem(HMENU menu, UINT position, Item& item)
{
    MENUITEMINFOW info{ sizeof(info) };
    info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_BITMAP | MIIM_STRING;
    if (!GetMenuItemInfoW(menu, position, TRUE, &info)) return false;
    std::vector<wchar_t> text(static_cast<size_t>(info.cch) + 1);
    info.dwTypeData = text.data();
    info.cch = static_cast<UINT>(text.size());
    if (!GetMenuItemInfoW(menu, position, TRUE, &info)) return false;
    item.id = info.wID; item.type = info.fType; item.state = info.fState;
    item.position = position; item.submenu = info.hSubMenu; item.bitmap = info.hbmpItem;
    item.text.assign(text.data());
    const auto tab = item.text.find(L'\t');
    item.label = VisibleText(item.text.substr(0, tab));
    item.shortcut = tab == std::wstring::npos ? L"" : item.text.substr(tab + 1);
    return true;
}

bool HasOpaqueItems(HMENU menu, int depth = 0)
{
    if (depth > 24) return true;
    const int count = GetMenuItemCount(menu);
    for (int i = 0; i < count; ++i)
    {
        Item item;
        if (!ReadItem(menu, i, item)) return true;
        if ((item.type & MFT_OWNERDRAW) && item.label.empty()) return true;
        if (item.submenu && HasOpaqueItems(item.submenu, depth + 1)) return true;
    }
    return false;
}

struct AccessibleState;
class Popup;
struct AccessibleState { Popup* popup{}; HWND list{}; };

// The Windows UI Automation legacy-accessibility bridge consumes this provider.
// Native list geometry, focus, selection and navigation remain delegated to oleacc.
class MenuAccessible final : public IAccessible
{
public:
    MenuAccessible(IAccessible* native, std::shared_ptr<AccessibleState> state) : m_native(native), m_state(std::move(state)) {}
    ~MenuAccessible() { m_native->Release(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override
    {
        if (!result) return E_POINTER;
        *result = nullptr;
        if (iid == IID_IUnknown || iid == IID_IDispatch || iid == IID_IAccessible)
        { *result = static_cast<IAccessible*>(this); AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refs); }
    ULONG STDMETHODCALLTYPE Release() override { ULONG n = InterlockedDecrement(&m_refs); if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* n) override { return m_native->GetTypeInfoCount(n); }
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT n, LCID l, ITypeInfo** p) override { return m_native->GetTypeInfo(n, l, p); }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID r, LPOLESTR* n, UINT c, LCID l, DISPID* d) override { return m_native->GetIDsOfNames(r,n,c,l,d); }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID d, REFIID r, LCID l, WORD f, DISPPARAMS* p, VARIANT* v, EXCEPINFO* e, UINT* a) override { return m_native->Invoke(d,r,l,f,p,v,e,a); }
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** p) override { return m_native->get_accParent(p); }
    HRESULT STDMETHODCALLTYPE get_accChildCount(long* p) override { return m_native->get_accChildCount(p); }
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT v, IDispatch** p) override { return m_native->get_accChild(v,p); }
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT v, BSTR* p) override { return m_native->get_accName(v,p); }
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT v, BSTR* p) override { return m_native->get_accValue(v,p); }
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT v, BSTR* p) override { return m_native->get_accDescription(v,p); }
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT v, VARIANT* p) override;
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT v, VARIANT* p) override;
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT v, BSTR* p) override { return m_native->get_accHelp(v,p); }
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR* p, VARIANT v, long* n) override { return m_native->get_accHelpTopic(p,v,n); }
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT v, BSTR* p) override;
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT* p) override { return m_native->get_accFocus(p); }
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT* p) override { return m_native->get_accSelection(p); }
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT v, BSTR* p) override;
    HRESULT STDMETHODCALLTYPE accSelect(long f, VARIANT v) override { return m_native->accSelect(f,v); }
    HRESULT STDMETHODCALLTYPE accLocation(long* x,long* y,long* w,long* h,VARIANT v) override { return m_native->accLocation(x,y,w,h,v); }
    HRESULT STDMETHODCALLTYPE accNavigate(long d, VARIANT v, VARIANT* p) override { return m_native->accNavigate(d,v,p); }
    HRESULT STDMETHODCALLTYPE accHitTest(long x,long y,VARIANT* p) override { return m_native->accHitTest(x,y,p); }
    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT v) override;
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT v, BSTR p) override { return m_native->put_accName(v,p); }
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT v, BSTR p) override { return m_native->put_accValue(v,p); }
private:
    LONG m_refs{ 1 };
    IAccessible* m_native;
    std::shared_ptr<AccessibleState> m_state;
};

class Popup
{
public:
    HWND window{}, owner{}, edit{}, list{}, back{}, previousFocus{};
    bool done{}, composing{}, rebuilding{}, restoreFocus{};
    UINT command{}, dpi{96};
    HFONT font{}, boldFont{};
    HBRUSH background{};
    COLORREF surface{}, ink{}, muted{}, selected{}, accent{};
    std::vector<Level> levels;
    std::vector<size_t> visible;
    std::shared_ptr<AccessibleState> accessibility{ std::make_shared<AccessibleState>() };
    MenuAccessible* accessible{};
    std::wstring searchText, backText, emptyText, invokeText, expandText;
    POINT anchor{};
    int rowHeight{}, width{}, height{};
    HMENU selectedMenu{};
    Item selectedIdentity;

    ~Popup()
    {
        accessibility->popup = nullptr; accessibility->list = nullptr;
        if (IsWindow(window)) DestroyWindow(window);
        if (accessible) accessible->Release();
        if (font) DeleteObject(font);
        if (boldFont) DeleteObject(boldFont);
        if (background) DeleteObject(background);
    }
    int Px(int n) const { return MulDiv(n, dpi, 96); }
    const Item* Row(int row) const
    {
        if (levels.empty() || row < 0 || static_cast<size_t>(row) >= visible.size()) return nullptr;
        return &levels.back().items[visible[row]];
    }
    void Colors()
    {
        HIGHCONTRASTW hc{ sizeof(hc) }; SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0);
        DWORD light = 1, bytes = sizeof(light);
        RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &bytes);
        if (hc.dwFlags & HCF_HIGHCONTRASTON)
        { surface = GetSysColor(COLOR_WINDOW); ink = GetSysColor(COLOR_WINDOWTEXT); muted = GetSysColor(COLOR_GRAYTEXT); selected = GetSysColor(COLOR_HIGHLIGHT); accent = GetSysColor(COLOR_HIGHLIGHTTEXT); }
        else if (light)
        { surface = RGB(247,249,255); ink = RGB(25,28,32); muted = RGB(89,95,106); selected = RGB(217,228,250); accent = RGB(38,86,158); }
        else
        { surface = RGB(31,35,43); ink = RGB(229,233,242); muted = RGB(166,176,192); selected = RGB(54,72,102); accent = RGB(173,201,255); }
        if (background) DeleteObject(background);
        background = CreateSolidBrush(surface);
    }
    void Fonts()
    {
        if (font) DeleteObject(font);
        if (boldFont) DeleteObject(boldFont);
        font = CreateFontW(-Px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        boldFont = CreateFontW(-Px(14),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        for (HWND control : { edit, list, back }) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
    void Layout()
    {
        MONITORINFO mi{ sizeof(mi) }; GetMonitorInfoW(MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST), &mi);
        width = (std::min)(Px(440), static_cast<int>(mi.rcWork.right-mi.rcWork.left));
        rowHeight = Px(48);
        height = (std::min)(Px(560), static_cast<int>(mi.rcWork.bottom-mi.rcWork.top));
        const int x = (std::max)(static_cast<int>(mi.rcWork.left), (std::min)(static_cast<int>(anchor.x), static_cast<int>(mi.rcWork.right)-width));
        const int y = (std::max)(static_cast<int>(mi.rcWork.top), (std::min)(static_cast<int>(anchor.y), static_cast<int>(mi.rcWork.bottom)-height));
        SetWindowPos(window, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE);
        const int padding = Px(12), backWidth = levels.size() > 1 ? Px(64) : 0;
        ShowWindow(back, backWidth ? SW_SHOW : SW_HIDE);
        MoveWindow(back,padding,padding,backWidth,Px(44),TRUE);
        MoveWindow(edit,padding+backWidth,padding,(std::max)(1,width-padding*2-backWidth),Px(44),TRUE);
        MoveWindow(list,padding,Px(68),(std::max)(1,width-padding*2),(std::max)(1,height-Px(80)),TRUE);
        MeasureRows();
    }
    void MeasureRows()
    {
        if (!list || !font) return;
        RECT client{}; GetClientRect(list,&client);
        HDC dc=GetDC(list); auto old=SelectObject(dc,font);
        for(size_t row=0;row<visible.size();++row)
        {
            const Item* item=Row(static_cast<int>(row));
            RECT text{0,0,(std::max)(Px(24),static_cast<int>(client.right)-Px(item->shortcut.empty()?92:200)),0};
            DrawTextW(dc,item->label.c_str(),-1,&text,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
            const int measured=item->Separator()?Px(12):(std::max)(Px(48),static_cast<int>(text.bottom)+Px(12));
            SendMessageW(list,LB_SETITEMHEIGHT,row,measured);
        }
        SelectObject(dc,old); ReleaseDC(list,dc);
    }
    void ReadLevel()
    {
        auto& level = levels.back(); level.items.clear();
        const int count = GetMenuItemCount(level.menu);
        for (int i = 0; i < count; ++i) { Item item; if (ReadItem(level.menu,i,item)) level.items.push_back(std::move(item)); }
    }
    void Filter()
    {
        if (rebuilding || levels.empty()) return;
        int length = GetWindowTextLengthW(edit);
        std::vector<wchar_t> query(length+1); GetWindowTextW(edit,query.data(),length+1);
        levels.back().query = query.data();
        std::wstring needle = query.data();
        std::transform(needle.begin(),needle.end(),needle.begin(),towlower);
        SendMessageW(list,WM_SETREDRAW,FALSE,0); SendMessageW(list,LB_RESETCONTENT,0,0); visible.clear();
        const auto& items = levels.back().items;
        for (size_t i=0;i<items.size();++i)
        {
            std::wstring haystack = items[i].label + L" " + items[i].shortcut;
            std::transform(haystack.begin(),haystack.end(),haystack.begin(),towlower);
            if (!needle.empty() && (items[i].Separator() || haystack.find(needle)==std::wstring::npos)) continue;
            visible.push_back(i);
            SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(items[i].label.c_str()));
        }
        SendMessageW(list,LB_SETCURSEL,visible.empty() ? -1 : 0,0);
        MeasureRows();
        SendMessageW(list,WM_SETREDRAW,TRUE,0); InvalidateRect(list,nullptr,TRUE);
        NotifyWinEvent(EVENT_OBJECT_REORDER,list,OBJID_CLIENT,CHILDID_SELF);
    }
    void Open(HMENU menu, UINT position)
    {
        if (!levels.empty()) levels.back().selected = static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0));
        SendMessageW(owner,WM_INITMENUPOPUP,reinterpret_cast<WPARAM>(menu),MAKELPARAM(position,FALSE));
        Level level; level.menu=menu; level.parentPosition=position; levels.push_back(std::move(level));
        ReadLevel(); rebuilding=true; SetWindowTextW(edit,L""); rebuilding=false; Filter(); Layout(); SetFocus(edit);
    }
    void Back()
    {
        if (levels.size()<2) { done=true; return; }
        SendMessageW(owner,WM_UNINITMENUPOPUP,reinterpret_cast<WPARAM>(levels.back().menu),0);
        levels.pop_back();
        SendMessageW(owner,WM_INITMENUPOPUP,reinterpret_cast<WPARAM>(levels.back().menu),MAKELPARAM(levels.back().parentPosition,FALSE));
        ReadLevel(); rebuilding=true; SetWindowTextW(edit,levels.back().query.c_str()); rebuilding=false;
        const int selection=levels.back().selected; Filter(); SendMessageW(list,LB_SETCURSEL,selection,0); Layout(); SetFocus(list);
    }
    void Activate(int row)
    {
        const Item* selectedItem=Row(row); if (!selectedItem || !selectedItem->Enabled()) return;
        const Item old=*selectedItem; HMENU menu=levels.back().menu;
        SendMessageW(owner,WM_INITMENUPOPUP,reinterpret_cast<WPARAM>(menu),MAKELPARAM(levels.back().parentPosition,FALSE));
        Item fresh;
        if (!ReadItem(menu,old.position,fresh) || fresh.id!=old.id || fresh.text!=old.text || fresh.submenu!=old.submenu || !fresh.Enabled())
        { ReadLevel(); Filter(); return; }
        if (fresh.submenu) { Open(fresh.submenu,fresh.position); return; }
        selectedMenu=menu; selectedIdentity=fresh; command=fresh.id; done=true;
    }
    void SelectNext(int delta)
    {
        if (visible.empty()) return;
        int row=static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0));
        for (size_t n=0;n<visible.size();++n)
        {
            row=(row+delta+static_cast<int>(visible.size()))%static_cast<int>(visible.size());
            if (Row(row)->Enabled()) { SendMessageW(list,LB_SETCURSEL,row,0); NotifyWinEvent(EVENT_OBJECT_FOCUS,list,OBJID_CLIENT,row+1); break; }
        }
    }
    bool Key(MSG& message)
    {
        if (message.message!=WM_KEYDOWN || composing) return false;
        const bool inEdit=message.hwnd==edit, inList=message.hwnd==list;
        if (message.wParam==VK_ESCAPE)
        {
            if (GetWindowTextLengthW(edit)) SetWindowTextW(edit,L"");
            else if (levels.size()>1) Back(); else done=true;
            return true;
        }
        if (inEdit && message.wParam==VK_DOWN) { SetFocus(list); if (SendMessageW(list,LB_GETCURSEL,0,0)<0) SelectNext(1); return true; }
        if (inList && (message.wParam==VK_UP || message.wParam==VK_DOWN)) { SelectNext(message.wParam==VK_DOWN ? 1 : -1); return true; }
        if (inList && message.wParam==VK_LEFT) { if(levels.size()>1) Back(); else SetFocus(edit); return true; }
        if ((inEdit || inList) && message.wParam==VK_RETURN) { Activate(static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0))); return true; }
        if (inList && message.wParam==VK_RIGHT) { const int r=static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0)); if(Row(r) && Row(r)->submenu) Activate(r); return true; }
        if (message.wParam==VK_TAB)
        {
            HWND next = inEdit ? list : (inList && levels.size()>1 ? back : edit);
            if (GetKeyState(VK_SHIFT)<0) next = inEdit ? (levels.size()>1 ? back : list) : (inList ? edit : list);
            SetFocus(next); return true;
        }
        return false;
    }
    void Draw(DRAWITEMSTRUCT& draw)
    {
        const Item* item=Row(static_cast<int>(draw.itemID));
        const bool active=(draw.itemState & ODS_SELECTED)!=0;
        HBRUSH fill=CreateSolidBrush(active ? selected : surface); FillRect(draw.hDC,&draw.rcItem,fill); DeleteObject(fill);
        if (!item) return;
        SetBkMode(draw.hDC,TRANSPARENT);
        if(item->Separator())
        {
            HPEN pen=CreatePen(PS_SOLID,1,muted); auto old=SelectObject(draw.hDC,pen);
            MoveToEx(draw.hDC,draw.rcItem.left+Px(12),(draw.rcItem.top+draw.rcItem.bottom)/2,nullptr);
            LineTo(draw.hDC,draw.rcItem.right-Px(12),(draw.rcItem.top+draw.rcItem.bottom)/2); SelectObject(draw.hDC,old); DeleteObject(pen); return;
        }
        SetTextColor(draw.hDC,item->Enabled() ? ink : muted);
        auto oldFont=SelectObject(draw.hDC,(item->state&MFS_DEFAULT) ? boldFont : font);
        RECT text=draw.rcItem; text.left+=Px(64); text.right-=Px(28); text.top+=Px(4); text.bottom-=Px(4);
        if(!item->shortcut.empty())
        {
            RECT shortcut=text; shortcut.left=(std::max)(shortcut.left,shortcut.right-Px(100));
            DrawTextW(draw.hDC,item->shortcut.c_str(),-1,&shortcut,DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
            text.right=shortcut.left-Px(8);
        }
        DrawTextW(draw.hDC,item->label.c_str(),-1,&text,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
        RECT marker=draw.rcItem; marker.left+=Px(8); marker.right=marker.left+Px(22);
        if(item->state&MFS_CHECKED) DrawTextW(draw.hDC,(item->type&MFT_RADIOCHECK) ? L"●" : L"✓",-1,&marker,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        if(item->bitmap && reinterpret_cast<INT_PTR>(item->bitmap)>12)
        {
            BITMAP bitmap{};
            if(GetObjectW(item->bitmap,sizeof(bitmap),&bitmap))
            {
                HDC source=CreateCompatibleDC(draw.hDC); auto oldBitmap=SelectObject(source,item->bitmap);
                BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
                AlphaBlend(draw.hDC,draw.rcItem.left+Px(34),draw.rcItem.top+(draw.rcItem.bottom-draw.rcItem.top-Px(20))/2,Px(20),Px(20),source,0,0,bitmap.bmWidth,bitmap.bmHeight,blend);
                SelectObject(source,oldBitmap); DeleteDC(source);
            }
        }
        if(item->submenu) { marker=draw.rcItem; marker.left=marker.right-Px(24); DrawTextW(draw.hDC,L"›",-1,&marker,DT_CENTER|DT_VCENTER|DT_SINGLELINE); }
        if((draw.itemState&ODS_FOCUS)!=0) { RECT focus=draw.rcItem; InflateRect(&focus,-2,-2); DrawFocusRect(draw.hDC,&focus); }
        SelectObject(draw.hDC,oldFont);
    }
    static LRESULT CALLBACK ChildProc(HWND hwnd,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data)
    {
        auto* self=reinterpret_cast<Popup*>(data);
        if(hwnd==self->edit)
        {
            if(message==WM_IME_STARTCOMPOSITION) self->composing=true;
            if(message==WM_IME_ENDCOMPOSITION) self->composing=false;
        }
        if(hwnd==self->list && message==WM_GETOBJECT && static_cast<LONG>(l)==OBJID_CLIENT && self->accessible)
            return LresultFromObject(IID_IAccessible,w,self->accessible);
        if(hwnd==self->list && message==WM_PAINT && self->visible.empty())
        {
            LRESULT result=DefSubclassProc(hwnd,message,w,l);
            HDC dc=GetDC(hwnd); RECT rect{}; GetClientRect(hwnd,&rect); InflateRect(&rect,-self->Px(12),-self->Px(12));
            auto old=SelectObject(dc,self->font); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,self->muted);
            DrawTextW(dc,self->emptyText.c_str(),-1,&rect,DT_WORDBREAK|DT_NOPREFIX);
            SelectObject(dc,old); ReleaseDC(hwnd,dc); return result;
        }
        if(hwnd==self->list && message==WM_LBUTTONUP)
        {
            DWORD hit=static_cast<DWORD>(SendMessageW(hwnd,LB_ITEMFROMPOINT,0,l));
            LRESULT result=DefSubclassProc(hwnd,message,w,l);
            if(HIWORD(hit)==0) self->Activate(LOWORD(hit));
            return result;
        }
        if(hwnd==self->list && message==WM_CHAR && w>=32)
        { SetFocus(self->edit); SendMessageW(self->edit,message,w,l); return 0; }
        if(message==WM_NCDESTROY) RemoveWindowSubclass(hwnd,ChildProc,1);
        return DefSubclassProc(hwnd,message,w,l);
    }
    static LRESULT CALLBACK WindowProc(HWND hwnd,UINT message,WPARAM w,LPARAM l)
    {
        Popup* self=reinterpret_cast<Popup*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
        if(message==WM_NCCREATE) { self=static_cast<Popup*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams); self->window=hwnd; SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self)); }
        if(!self) return DefWindowProcW(hwnd,message,w,l);
        switch(message)
        {
        case WM_COMMAND:
            if(LOWORD(w)==EditId && HIWORD(w)==EN_CHANGE) self->Filter();
            if(LOWORD(w)==BackId && HIWORD(w)==BN_CLICKED) self->Back();
            return 0;
        case WM_DRAWITEM: self->Draw(*reinterpret_cast<DRAWITEMSTRUCT*>(l)); return TRUE;
        case WM_MEASUREITEM: reinterpret_cast<MEASUREITEMSTRUCT*>(l)->itemHeight=self->rowHeight; return TRUE;
        case WM_CTLCOLORLISTBOX: case WM_CTLCOLOREDIT: case WM_CTLCOLORSTATIC:
            SetTextColor(reinterpret_cast<HDC>(w),self->ink); SetBkColor(reinterpret_cast<HDC>(w),self->surface); return reinterpret_cast<LRESULT>(self->background);
        case WM_ERASEBKGND:
            { RECT rect; GetClientRect(hwnd,&rect); FillRect(reinterpret_cast<HDC>(w),&rect,self->background); return TRUE; }
        case WM_ACTIVATE: if(LOWORD(w)==WA_INACTIVE) self->done=true; return 0;
        case WM_CLOSE: self->done=true; return 0;
        case WM_DPICHANGED: self->dpi=HIWORD(w); self->Fonts(); self->Layout(); return 0;
        case WM_SETTINGCHANGE: case WM_THEMECHANGED: self->Colors(); InvalidateRect(hwnd,nullptr,TRUE); return 0;
        case WM_NCDESTROY: self->accessibility->popup=nullptr; self->accessibility->list=nullptr; self->window=nullptr; break;
        }
        return DefWindowProcW(hwnd,message,w,l);
    }
    bool Create()
    {
        LANGID language=theApp.m_general_data.language.language_id;
        if(!language) language=GetUserDefaultUILanguage();
        const bool chinese=PRIMARYLANGID(language)==LANG_CHINESE;
        searchText=chinese ? L"搜尋選單" : L"Search menu";
        backText=chinese ? L"返回" : L"Back";
        emptyText=chinese ? L"沒有符合的指令" : L"No matching commands";
        invokeText=chinese ? L"執行" : L"Invoke";
        expandText=chinese ? L"開啟子選單" : L"Open submenu";
        WNDCLASSEXW cls{sizeof(cls)}; cls.lpfnWndProc=WindowProc; cls.hInstance=AfxGetInstanceHandle(); cls.lpszClassName=L"TrafficMonitor.NativeSearchMenu"; cls.hCursor=LoadCursorW(nullptr,IDC_ARROW); cls.style=CS_DROPSHADOW;
        if(!RegisterClassExW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;
        Colors();
        window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,cls.lpszClassName,L"TrafficMonitor",WS_POPUP|WS_BORDER,anchor.x,anchor.y,1,1,owner,nullptr,cls.hInstance,this);
        if(!window) return false;
        using GetDpi=UINT(WINAPI*)(HWND); auto getDpi=reinterpret_cast<GetDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow"));
        if(getDpi) dpi=getDpi(window);
        rowHeight=Px(48);
        back=CreateWindowExW(0,L"BUTTON",backText.c_str(),WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,0,0,1,1,window,reinterpret_cast<HMENU>(BackId),cls.hInstance,nullptr);
        edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,0,0,1,1,window,reinterpret_cast<HMENU>(EditId),cls.hInstance,nullptr);
        list=CreateWindowExW(0,L"LISTBOX",searchText.c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY|LBS_OWNERDRAWVARIABLE|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT,0,0,1,1,window,reinterpret_cast<HMENU>(ListId),cls.hInstance,nullptr);
        if(!edit || !list || !back) return false;
        SendMessageW(edit,EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(searchText.c_str()));
        SendMessageW(edit,EM_SETLIMITTEXT,512,0);
        IAccPropServices* properties{};
        if(SUCCEEDED(CoCreateInstance(CLSID_AccPropServices,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&properties))))
        { properties->SetHwndPropStr(edit,OBJID_CLIENT,CHILDID_SELF,PROPID_ACC_NAME,searchText.c_str()); properties->Release(); }
        accessibility->popup=this; accessibility->list=list;
        IAccessible* native{};
        if(SUCCEEDED(CreateStdAccessibleObject(list,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&native)))) accessible=new MenuAccessible(native,accessibility);
        SetWindowSubclass(edit,ChildProc,1,reinterpret_cast<DWORD_PTR>(this)); SetWindowSubclass(list,ChildProc,1,reinterpret_cast<DWORD_PTR>(this));
        Fonts();
        const DWORD rounded=2; DwmSetWindowAttribute(window,33,&rounded,sizeof(rounded));
        return true;
    }
};

HRESULT MenuAccessible::get_accRole(VARIANT child, VARIANT* value)
{
    if(!value) return E_POINTER;
    if(!m_state->popup) return CO_E_OBJNOTCONNECTED;
    const Item* item=child.vt==VT_I4 ? m_state->popup->Row(child.lVal-1) : nullptr;
    if(!item) return m_native->get_accRole(child,value);
    VariantInit(value); value->vt=VT_I4; value->lVal=item->Separator() ? ROLE_SYSTEM_SEPARATOR : ROLE_SYSTEM_MENUITEM; return S_OK;
}
HRESULT MenuAccessible::get_accState(VARIANT child, VARIANT* value)
{
    if(!value) return E_POINTER;
    if(!m_state->popup) return CO_E_OBJNOTCONNECTED;
    HRESULT result=m_native->get_accState(child,value);
    const Item* item=child.vt==VT_I4 ? m_state->popup->Row(child.lVal-1) : nullptr;
    if(SUCCEEDED(result) && item && value->vt==VT_I4)
    {
        if(!item->Enabled()) value->lVal|=STATE_SYSTEM_UNAVAILABLE;
        if(item->state&MFS_CHECKED) value->lVal|=STATE_SYSTEM_CHECKED;
        if(item->state&MFS_DEFAULT) value->lVal|=STATE_SYSTEM_DEFAULT;
        if(item->submenu) value->lVal|=STATE_SYSTEM_HASPOPUP|STATE_SYSTEM_COLLAPSED;
        if(item->Separator()) value->lVal&=~(STATE_SYSTEM_SELECTABLE|STATE_SYSTEM_FOCUSABLE);
    }
    return result;
}
HRESULT MenuAccessible::get_accKeyboardShortcut(VARIANT child,BSTR* value)
{
    if(!value) return E_POINTER; *value=nullptr;
    if(!m_state->popup) return CO_E_OBJNOTCONNECTED;
    const Item* item=child.vt==VT_I4 ? m_state->popup->Row(child.lVal-1) : nullptr;
    if(!item || item->shortcut.empty()) return S_FALSE;
    *value=SysAllocString(item->shortcut.c_str()); return *value ? S_OK : E_OUTOFMEMORY;
}
HRESULT MenuAccessible::get_accDefaultAction(VARIANT child,BSTR* value)
{
    if(!value) return E_POINTER; *value=nullptr;
    if(!m_state->popup) return CO_E_OBJNOTCONNECTED;
    const Item* item=child.vt==VT_I4 ? m_state->popup->Row(child.lVal-1) : nullptr;
    if(!item || !item->Enabled()) return S_FALSE;
    *value=SysAllocString((item->submenu ? m_state->popup->expandText : m_state->popup->invokeText).c_str()); return *value ? S_OK : E_OUTOFMEMORY;
}
HRESULT MenuAccessible::accDoDefaultAction(VARIANT child)
{
    if(!m_state->popup) return CO_E_OBJNOTCONNECTED;
    if(child.vt!=VT_I4) return E_INVALIDARG;
    const Item* item=m_state->popup->Row(child.lVal-1);
    if(!item || !item->Enabled()) return E_INVALIDARG;
    // Queue onto the owner UI thread rather than entering application code from COM.
    SendMessageW(m_state->list,LB_SETCURSEL,child.lVal-1,0);
    PostMessageW(m_state->list,WM_KEYDOWN,VK_RETURN,0); return S_OK;
}
}

UINT Track(CMenu* menu,UINT flags,int x,int y,CWnd* ownerWindow)
{
    if(!menu || !IsMenu(menu->GetSafeHmenu()) || !ownerWindow || !IsWindow(ownerWindow->GetSafeHwnd())) return 0;
    HWND owner=ownerWindow->GetSafeHwnd(); HMENU handle=menu->GetSafeHmenu();
    // Owner-drawn plugins without retrievable labels retain their native renderer.
    if(HasOpaqueItems(handle)) return static_cast<UINT>(menu->TrackPopupMenu(flags,x,y,ownerWindow));
    Popup popup; popup.owner=owner; popup.anchor={x,y}; popup.previousFocus=GetFocus();
    SendMessageW(owner,WM_ENTERMENULOOP,TRUE,0); SendMessageW(owner,WM_INITMENU,reinterpret_cast<WPARAM>(handle),0);
    const bool created=popup.Create();
    if(created)
    {
        popup.Open(handle,0); ShowWindow(popup.window,SW_SHOW); SetForegroundWindow(popup.window); SetFocus(popup.edit);
        MSG message{};
        while(!popup.done && IsWindow(owner) && IsWindow(popup.window))
        {
            const BOOL read=GetMessageW(&message,nullptr,0,0);
            if(read<=0) { if(read==0) PostQuitMessage(static_cast<int>(message.wParam)); break; }
            const bool inside=message.hwnd==popup.window || IsChild(popup.window,message.hwnd);
            if(!inside && (message.message==WM_LBUTTONDOWN || message.message==WM_RBUTTONDOWN || message.message==WM_NCLBUTTONDOWN)) popup.done=true;
            if(inside && popup.Key(message)) continue;
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        popup.restoreFocus=GetForegroundWindow()==popup.window;
        while(!popup.levels.empty())
        { SendMessageW(owner,WM_UNINITMENUPOPUP,reinterpret_cast<WPARAM>(popup.levels.back().menu),0); popup.levels.pop_back(); }
        if(IsWindow(popup.window)) DestroyWindow(popup.window);
    }
    if(IsWindow(owner)) SendMessageW(owner,WM_EXITMENULOOP,TRUE,0);
    if(!created && IsWindow(owner)) return static_cast<UINT>(menu->TrackPopupMenu(flags,x,y,ownerWindow));
    if(popup.restoreFocus && IsWindow(owner))
    {
        HWND foreground=GetForegroundWindow(); HWND root=GetAncestor(owner,GA_ROOT);
        if(!foreground || foreground==root || foreground==owner)
        {
            HWND focus=popup.previousFocus;
            if(IsWindow(focus) && (focus==owner || IsChild(root,focus))) SetFocus(focus);
        }
    }
    // Exit handlers may change plugin state. Never execute an item removed or
    // replaced while the popup was unwinding its notification lifecycle.
    Item finalItem;
    if(popup.command && (!IsMenu(popup.selectedMenu) || !ReadItem(popup.selectedMenu,popup.selectedIdentity.position,finalItem)
        || finalItem.id!=popup.selectedIdentity.id || finalItem.text!=popup.selectedIdentity.text
        || finalItem.submenu || !finalItem.Enabled())) popup.command=0;
    if(popup.command && !(flags&(TPM_RETURNCMD|TPM_NONOTIFY)) && IsWindow(owner)) PostMessageW(owner,WM_COMMAND,popup.command,0);
    return (flags&TPM_RETURNCMD) ? popup.command : static_cast<UINT>(popup.command!=0);
}
}
