#include "stdafx.h"
#include "SearchableMenu.h"
#include "../NativePreferences/Text.h"
#include "../NativePreferences/Subscription.h"
#include "../NativeRegex/Workbench.h"
#include "../TrafficMonitor.h"
#include "../NativeMaterial/TextField.h"
#include "../NativeMaterial/ActionSounds.h"
#include "../NativeMaterial/Controls.h"
#include "../NativeMaterial/ScrollRail.h"
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
#pragma comment(lib, "uuid.lib")

namespace NativeMenus
{
namespace
{
constexpr UINT EditId = 51001, ListId = 51002, BackId = 51003, RegexId = 51004;
constexpr UINT AccessibleInvokeMessage = WM_APP + 0x431;
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
    std::shared_ptr<NativeRegex::Filter> filter{std::make_shared<NativeRegex::Filter>()};
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
    const wchar_t* en=nullptr;const wchar_t* zh=nullptr;
    switch(item.id)
    {
        case ID_OPTIONS:case ID_OPTIONS2:en=L"Settings";zh=L"設定";break;
        case ID_TRAFFIC_HISTORY:en=L"Traffic history";zh=L"流量記錄";break;
        case ID_PLUGIN_MANAGE:en=L"Manage plugins";zh=L"管理外掛程式";break;
        case ID_ALWAYS_ON_TOP:en=L"Always on top";zh=L"保持最上層";break;
        case ID_LOCK_WINDOW_POS:en=L"Lock window position";zh=L"鎖定視窗位置";break;
        case ID_SHOW_NOTIFY_ICON:en=L"Show notification-area icon";zh=L"顯示通知區域圖示";break;
        case ID_SHOW_TASK_BAR_WND:en=L"Show taskbar monitor";zh=L"顯示工作列監察器";break;
        case ID_SHOW_MAIN_WND:en=L"Show floating monitor";zh=L"顯示浮動監察器";break;
        case ID_CHECK_UPDATE:en=L"Check for updates";zh=L"檢查更新";break;
        default:break;
    }
    if(en)item.label=NativePreferences::Pair(en,zh).GetString();

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
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* n) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->GetTypeInfoCount(n); }
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT n, LCID l, ITypeInfo** p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->GetTypeInfo(n, l, p); }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID r, LPOLESTR* n, UINT c, LCID l, DISPID* d) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->GetIDsOfNames(r,n,c,l,d); }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID d, REFIID r, LCID l, WORD f, DISPPARAMS* p, VARIANT* v, EXCEPINFO* e, UINT* a) override
    {
        if(!m_state->popup) return CO_E_OBJNOTCONNECTED;
        if(r!=IID_NULL) return DISP_E_UNKNOWNINTERFACE;
        ITypeInfo* type{};
        HRESULT result=m_native->GetTypeInfo(0,l,&type);
        if(FAILED(result)) return result;
        result=DispInvoke(static_cast<IAccessible*>(this),type,d,f,p,v,e,a);
        type->Release(); return result;
    }
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accParent(p); }
    HRESULT STDMETHODCALLTYPE get_accChildCount(long* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accChildCount(p); }
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT v, IDispatch** p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accChild(v,p); }
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT v, BSTR* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accName(v,p); }
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT v, BSTR* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accValue(v,p); }
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT v, BSTR* p) override;
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT v, VARIANT* p) override;
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT v, VARIANT* p) override;
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT v, BSTR* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accHelp(v,p); }
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR* p, VARIANT v, long* n) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accHelpTopic(p,v,n); }
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT v, BSTR* p) override;
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accFocus(p); }
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->get_accSelection(p); }
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT v, BSTR* p) override;
    HRESULT STDMETHODCALLTYPE accSelect(long f, VARIANT v) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->accSelect(f,v); }
    HRESULT STDMETHODCALLTYPE accLocation(long* x,long* y,long* w,long* h,VARIANT v) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->accLocation(x,y,w,h,v); }
    HRESULT STDMETHODCALLTYPE accNavigate(long d, VARIANT v, VARIANT* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->accNavigate(d,v,p); }
    HRESULT STDMETHODCALLTYPE accHitTest(long x,long y,VARIANT* p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->accHitTest(x,y,p); }
    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT v) override;
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT v, BSTR p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->put_accName(v,p); }
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT v, BSTR p) override { if(!m_state->popup) return CO_E_OBJNOTCONNECTED; return m_native->put_accValue(v,p); }
private:
    LONG m_refs{ 1 };
    IAccessible* m_native;
    std::shared_ptr<AccessibleState> m_state;
};

class Popup
{
public:
    HWND window{}, owner{}, edit{}, list{}, back{}, empty{}, regex{}, previousFocus{};
    NativePreferences::Subscription preferences;
    NativeMaterial::TextField searchField;
    NativeMaterial::Button backButton, regexButton;
    NativeMaterial::ScrollRail scrollRail;
    bool done{}, composing{}, rebuilding{}, restoreFocus{}, compatibility{}, chinese{}, noResultCueSent{};
    UINT command{}, dpi{96};
    NativeRegex::Editor regexEditor;
    HFONT font{}, boldFont{};
    HBRUSH background{};
    COLORREF surface{}, ink{}, muted{}, selected{}, accent{}, selectedInk{};
    std::vector<Level> levels;
    std::vector<size_t> visible;
    std::shared_ptr<AccessibleState> accessibility{ std::make_shared<AccessibleState>() };
    MenuAccessible* accessible{};
    std::wstring searchText, backText, emptyText, invokeText, expandText, radioText;
    POINT anchor{};
    int rowHeight{}, width{}, height{};
    bool placing{},reflowing{},externallySized{};
    HMENU selectedMenu{};
    Item selectedIdentity;
    struct Invocation { UINT_PTR generation; HMENU menu; Item item; };
    UINT_PTR generation{}, nextInvocation{};
    std::map<UINT_PTR,Invocation> invocations;

    ~Popup()
    {
        preferences.Detach();
        regexEditor.Close();
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
        const bool light = !NativePreferences::Current().dark;
        if (hc.dwFlags & HCF_HIGHCONTRASTON)
        { surface = GetSysColor(COLOR_WINDOW); ink = GetSysColor(COLOR_WINDOWTEXT); muted = GetSysColor(COLOR_GRAYTEXT); selected = GetSysColor(COLOR_HIGHLIGHT); accent = GetSysColor(COLOR_HIGHLIGHTTEXT); }
        else if (light)
        { surface = RGB(245,247,250); ink = RGB(23,33,47); muted = RGB(82,97,116); selected = RGB(220,232,255); accent = RGB(36,95,200); }
        else
        { surface = RGB(16,20,25); ink = RGB(230,237,245); muted = RGB(168,183,200); selected = RGB(40,58,84); accent = RGB(145,184,255); }
        selectedInk=(hc.dwFlags&HCF_HIGHCONTRASTON)?GetSysColor(COLOR_HIGHLIGHTTEXT):ink;
        if (background) DeleteObject(background);
        background = CreateSolidBrush(surface);
        const auto appearance = NativePreferences::EffectiveAppearance();
        if (edit) searchField.SetAppearance(appearance);
        if (back) backButton.SetAppearance(appearance);
        if (regex) regexButton.SetAppearance(appearance);
        if (scrollRail.GetSafeHwnd()) scrollRail.SetAppearance(appearance);
    }
    void Fonts()
    {
        if (font) DeleteObject(font);
        if (boldFont) DeleteObject(boldFont);
        font = CreateFontW(-Px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        boldFont = CreateFontW(-Px(14),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        for (HWND control : { edit, list, back, empty, regex }) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
    void SyncScroll()
    {
        if (!list || !scrollRail.GetSafeHwnd()) return;
        RECT client{};
        GetClientRect(list, &client);
        const int top = static_cast<int>(SendMessageW(list, LB_GETTOPINDEX, 0, 0));
        int page = 0, used = 0;
        for (int row = top; row < static_cast<int>(visible.size()); ++row)
        {
            const int height = static_cast<int>(SendMessageW(list, LB_GETITEMHEIGHT, row, 0));
            if (height <= 0 || used + height > client.bottom) break;
            used += height;
            ++page;
        }
        scrollRail.SetMetrics(static_cast<int>(visible.size()), (std::max)(1, page), top);
    }
    void Layout()
    {
        if(externallySized){ReflowChildren();return;}
        MONITORINFO mi{sizeof(mi)};
        GetMonitorInfoW(MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST), &mi);
        width = (std::min)(Px(420), static_cast<int>(mi.rcWork.right - mi.rcWork.left));
        rowHeight = Px(48);
        const int padding = Px(12);
        const int toolbar = NativePreferences::Current().values.language == NativePreferences::Language::Bilingual ? Px(56) : Px(44);
        const int listTop = padding + toolbar + Px(12);
        const int oldTop=static_cast<int>(SendMessageW(list,LB_GETTOPINDEX,0,0));
        RECT frame{};AdjustWindowRectEx(&frame,static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE)),FALSE,static_cast<DWORD>(GetWindowLongPtrW(window,GWL_EXSTYLE)));
        const int frameWidth=frame.right-frame.left,frameHeight=frame.bottom-frame.top;
        MoveWindow(list, padding, listTop, (std::max)(1, width-frameWidth-padding*2-Px(16)), Px(100), FALSE);
        MeasureRows();
        int contentHeight = 0;
        for (size_t row = 0; row < visible.size(); ++row)
            contentHeight += static_cast<int>(SendMessageW(list, LB_GETITEMHEIGHT, row, 0));
        height = (std::min)((std::min)(Px(480), static_cast<int>(mi.rcWork.bottom - mi.rcWork.top)), listTop + (std::max)(Px(72), contentHeight) + padding+frameHeight);
        const int x = (std::max)(static_cast<int>(mi.rcWork.left), (std::min)(static_cast<int>(anchor.x), static_cast<int>(mi.rcWork.right) - width));
        const int y = (std::max)(static_cast<int>(mi.rcWork.top), (std::min)(static_cast<int>(anchor.y), static_cast<int>(mi.rcWork.bottom) - height));
        placing=true;
        SetWindowPos(window, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE);
        placing=false;
        ReflowChildren();
        if(oldTop>=0)SendMessageW(list,LB_SETTOPINDEX,oldTop,0);
        SyncScroll();
    }
    void ReflowChildren()
    {
        if(reflowing||!list||!edit||!back||!regex||!scrollRail.GetSafeHwnd())return;
        reflowing=true;
        RECT client{};GetClientRect(window,&client);
        const int clientWidth=client.right-client.left,clientHeight=client.bottom-client.top;
        const int padding=Px(12),backWidth=levels.size()>1?Px(72):0;
        const int toolbar=NativePreferences::Current().values.language==NativePreferences::Language::Bilingual?Px(56):Px(44);
        const int listTop=padding+toolbar+Px(12);
        const int oldTop=static_cast<int>(SendMessageW(list,LB_GETTOPINDEX,0,0));
        const int selectedRow=static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0));
        ShowWindow(back, backWidth ? SW_SHOW : SW_HIDE);
        MoveWindow(back, padding, padding, backWidth, toolbar, TRUE);
        MoveWindow(edit, padding + backWidth, padding, (std::max)(1, clientWidth - padding * 2 - backWidth - Px(96)), toolbar, TRUE);
        MoveWindow(regex, clientWidth - padding - Px(88), padding, Px(88), toolbar, TRUE);
        const int pageHeight = (std::max)(1, clientHeight - listTop - padding);
        MoveWindow(list, padding, listTop, (std::max)(1, clientWidth - padding * 2 - Px(16)), pageHeight, TRUE);
        scrollRail.MoveWindow(clientWidth - padding - Px(12), listTop, Px(12), pageHeight);
        MoveWindow(empty, padding + Px(12), listTop + Px(12), (std::max)(1,clientWidth-padding*2-Px(24)), (std::min)(Px(64),(std::max)(1,pageHeight-Px(24))), TRUE);
        MeasureRows();
        if(selectedRow>=0)SendMessageW(list,LB_SETCURSEL,selectedRow,0);
        if(oldTop>=0)SendMessageW(list,LB_SETTOPINDEX,oldTop,0);
        SyncScroll();
        reflowing=false;
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
        ++generation;
        int length = GetWindowTextLengthW(edit);
        std::vector<wchar_t> query(length+1); GetWindowTextW(edit,query.data(),length+1);
        levels.back().query = query.data();
        auto& filter=*levels.back().filter;
        filter.options.query=query.data();std::vector<std::wstring> subjects;
        for(const auto& item:levels.back().items)subjects.push_back(item.label+L" "+item.shortcut);
        filter.Request(subjects);
        std::wstring needle = query.data();
        SendMessageW(list,WM_SETREDRAW,FALSE,0); SendMessageW(list,LB_RESETCONTENT,0,0); visible.clear();
        const auto& items = levels.back().items;
        for (size_t i=0;i<items.size();++i)
        {
            std::wstring haystack = items[i].label + L" " + items[i].shortcut;
            if (!filter.Visible(i) || (!needle.empty() && items[i].Separator())) continue;
            visible.push_back(i);
            SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(items[i].label.c_str()));
        }
        SendMessageW(list,LB_SETCURSEL,visible.empty() ? -1 : 0,0);
        SetWindowTextW(empty,filter.status.empty()?emptyText.c_str():filter.status.c_str());
        ShowWindow(empty,visible.empty()?SW_SHOW:SW_HIDE);
        // Cue only on the transition into an empty result for a typed query, never on every keystroke.
        const bool noResults=visible.empty()&&!needle.empty();if(noResults&&!noResultCueSent)NativeSounds::Notify(NativeSounds::Action::SearchNoResults);noResultCueSent=noResults;
        Layout();
        SendMessageW(list,WM_SETREDRAW,TRUE,0); InvalidateRect(list,nullptr,TRUE);
        NotifyWinEvent(EVENT_OBJECT_REORDER,list,OBJID_CLIENT,CHILDID_SELF);
    }
    void EditRegex()
    {
        if(levels.empty())return;
        auto filter=levels.back().filter;std::vector<std::wstring> subjects;
        for(const auto& item:levels.back().items)subjects.push_back(item.label+L" "+item.shortcut);
        const HMENU levelMenu=levels.back().menu;
        regexEditor.Open(window,edit,filter->options,
            [this,filter,levelMenu]{
                std::vector<std::wstring> current;
                if(!levels.empty() && levels.back().filter==filter && levels.back().menu==levelMenu)
                    for(const auto& item:levels.back().items)current.push_back(item.label+L" "+item.shortcut);
                return current;
            },
            [this,filter,levelMenu](const NativeRegex::Options& options){
                if(done || levels.empty() || levels.back().filter!=filter || levels.back().menu!=levelMenu)return;
                filter->options=options;rebuilding=true;SetWindowTextW(edit,options.query.c_str());rebuilding=false;Filter();
            },chinese);

    }
    void Open(HMENU menu, UINT position)
    {
        regexEditor.Close();
        if (!levels.empty()) levels.back().selected = static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0));
        SendMessageW(owner,WM_INITMENUPOPUP,reinterpret_cast<WPARAM>(menu),MAKELPARAM(position,FALSE));
        Level level; level.menu=menu; level.parentPosition=position; levels.push_back(std::move(level));
        if(HasOpaqueItems(menu)) { compatibility=true; done=true; return; }
        ReadLevel(); rebuilding=true; SetWindowTextW(edit,L""); rebuilding=false; Filter(); Layout(); SetFocus(edit);
    }
    void Back()
    {
        regexEditor.Close();
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
        ActivateIdentity(menu,old);
    }
    void ActivateIdentity(HMENU menu,const Item& old)
    {
        regexEditor.Close();
        if(levels.empty() || levels.back().menu!=menu) return;
        SendMessageW(owner,WM_INITMENUPOPUP,reinterpret_cast<WPARAM>(menu),MAKELPARAM(levels.back().parentPosition,FALSE));
        Item fresh;
        if (!ReadItem(menu,old.position,fresh) || fresh.id!=old.id || fresh.text!=old.text || fresh.submenu!=old.submenu || !fresh.Enabled())
        { ReadLevel(); Filter(); return; }
        if (fresh.submenu) { Open(fresh.submenu,fresh.position); return; }
        selectedMenu=menu; selectedIdentity=fresh; command=fresh.id; done=true;
    }
    HRESULT QueueInvocation(int row)
    {
        const Item* item=Row(row);
        if(!item || !item->Enabled() || done || !IsWindow(window)) return E_INVALIDARG;
        const UINT_PTR id=++nextInvocation;
        invocations.emplace(id,Invocation{generation,levels.back().menu,*item});
        if(!PostMessageW(window,AccessibleInvokeMessage,id,0))
        { const DWORD error=GetLastError(); invocations.erase(id); return HRESULT_FROM_WIN32(error?error:ERROR_INVALID_WINDOW_HANDLE); }
        return S_OK;
    }
    void ExecuteInvocation(UINT_PTR id)
    {
        auto found=invocations.find(id); if(found==invocations.end()) return;
        const Invocation invocation=found->second; invocations.erase(found);
        if(done || levels.empty() || generation!=invocation.generation || levels.back().menu!=invocation.menu) return;
        ActivateIdentity(invocation.menu,invocation.item);
    }
    void SelectNext(int delta)
    {
        if (visible.empty()) return;
        int row=static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0));
        for (size_t n=0;n<visible.size();++n)
        {
            row=(row+delta+static_cast<int>(visible.size()))%static_cast<int>(visible.size());
            if (Row(row)->Enabled()) { SendMessageW(list,LB_SETCURSEL,row,0); NotifyWinEvent(EVENT_OBJECT_FOCUS,list,OBJID_CLIENT,row+1); SyncScroll(); break; }
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
            std::vector<HWND> order{edit, regex, list};
            if (scrollRail.IsWindowVisible()) order.push_back(scrollRail.GetSafeHwnd());
            if (levels.size() > 1) order.push_back(back);
            auto current = std::find(order.begin(), order.end(), message.hwnd);
            int index = current == order.end() ? 0 : static_cast<int>(current - order.begin());
            const int delta = GetKeyState(VK_SHIFT) < 0 ? -1 : 1;
            index = (index + delta + static_cast<int>(order.size())) % static_cast<int>(order.size());
            HWND next = order[index];
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
        SetTextColor(draw.hDC,item->Enabled() ? (active?selectedInk:ink) : muted);
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
        if(hwnd==self->list && message==WM_LBUTTONUP)
        {
            DWORD hit=static_cast<DWORD>(SendMessageW(hwnd,LB_ITEMFROMPOINT,0,l));
            LRESULT result=DefSubclassProc(hwnd,message,w,l);
            if(HIWORD(hit)==0) self->Activate(LOWORD(hit));
            return result;
        }
        if(hwnd==self->list && message==WM_CHAR && w>=32)
        { SetFocus(self->edit); SendMessageW(self->edit,message,w,l); return 0; }
        if (hwnd == self->list && (message == WM_MOUSEWHEEL || message == WM_KEYDOWN))
        {
            const auto result = DefSubclassProc(hwnd, message, w, l);
            self->SyncScroll();
            return result;
        }
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
        case WM_SIZE:
            if(!self->placing&&self->list&&w!=SIZE_MINIMIZED){self->externallySized=true;self->ReflowChildren();}
            return 0;
        case AccessibleInvokeMessage: self->ExecuteInvocation(w);return 0;
        case WM_TIMER:
            if(w==1)
            {
                // The input desktop's foreground can be unavailable while this
                // UI thread still has an active owned editor on another desktop.
                // Unknown activation is not evidence of an outside click.
                HWND active = GetForegroundWindow();
                if (!active)
                    active = GetActiveWindow();
                if (self->regexEditor.IsOpen() && active
                    && active != self->window && !self->regexEditor.Contains(active))
                {
                    self->done = true;
                }
                if(!self->levels.empty() && self->levels.back().filter->Poll())self->Filter();
            }return 0;
        case WM_COMMAND:
            if(LOWORD(w)==EditId && HIWORD(w)==EN_CHANGE){if(!self->rebuilding)self->regexEditor.Close();self->Filter();}
            if(LOWORD(w)==BackId && HIWORD(w)==BN_CLICKED) self->Back();
            if(LOWORD(w)==RegexId && HIWORD(w)==BN_CLICKED) self->EditRegex();
            return 0;
        case WM_DRAWITEM: self->Draw(*reinterpret_cast<DRAWITEMSTRUCT*>(l)); return TRUE;
        case WM_MEASUREITEM: reinterpret_cast<MEASUREITEMSTRUCT*>(l)->itemHeight=self->rowHeight; return TRUE;
        case WM_CTLCOLOREDIT:
            if (reinterpret_cast<HWND>(l) == self->edit) return reinterpret_cast<LRESULT>(self->searchField.Color(CDC::FromHandle(reinterpret_cast<HDC>(w))));
            break;
        case WM_CTLCOLORLISTBOX: case WM_CTLCOLORSTATIC:
            SetTextColor(reinterpret_cast<HDC>(w),self->ink); SetBkColor(reinterpret_cast<HDC>(w),self->surface); return reinterpret_cast<LRESULT>(self->background);
        case WM_ERASEBKGND:
            { RECT rect; GetClientRect(hwnd,&rect); FillRect(reinterpret_cast<HDC>(w),&rect,self->background); return TRUE; }
        case WM_ACTIVATE: if(LOWORD(w)==WA_INACTIVE && !self->regexEditor.Contains(reinterpret_cast<HWND>(l))) self->done=true; return 0;
        case WM_CLOSE: self->done=true; return 0;
        case WM_DPICHANGED: self->dpi=HIWORD(w); self->Fonts(); self->Layout(); return 0;
        case NativePreferences::ChangedMessage: if(!self->preferences.Accepts(w))return 0; self->PreferencesChanged(); InvalidateRect(hwnd,nullptr,TRUE); return 0;
        case WM_SETTINGCHANGE: case WM_THEMECHANGED: NativePreferences::RefreshSystemAppearance(); self->Colors(); InvalidateRect(hwnd,nullptr,TRUE); return 0;
        case WM_NCDESTROY: self->preferences.Detach(); self->accessibility->popup=nullptr; self->accessibility->list=nullptr; self->window=nullptr; break;
        }
        return DefWindowProcW(hwnd,message,w,l);
    }
    void Labels()
    {
        chinese=NativePreferences::Current().values.language==NativePreferences::Language::Cantonese;
        searchText=NativePreferences::Pair(L"Search menu",L"搜尋選單").GetString();
        if (NativePreferences::Current().values.language == NativePreferences::Language::Bilingual) searchText=L"Search menu · 搜尋選單";
        backText=NativePreferences::Pair(L"Back",L"返回").GetString();
        emptyText=NativePreferences::Pair(L"No matching commands",L"沒有符合的指令").GetString();
        invokeText=NativePreferences::Pair(L"Invoke",L"執行").GetString();
        expandText=NativePreferences::Pair(L"Open submenu",L"開啟子選單").GetString();
        radioText=NativePreferences::Pair(L"Radio option",L"單選選項").GetString();
        if(back)SetWindowTextW(back,backText.c_str());if(regex)SetWindowTextW(regex,NativePreferences::Pair(L"Regex",L"正規式"));
        if(edit){SendMessageW(edit,EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(searchText.c_str()));IAccPropServices* properties{};if(SUCCEEDED(CoCreateInstance(CLSID_AccPropServices,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&properties)))){properties->SetHwndPropStr(edit,OBJID_CLIENT,CHILDID_SELF,PROPID_ACC_NAME,searchText.c_str());properties->Release();}}
    }
    void PreferencesChanged()
    {
        UINT selectedId=0;const auto* selected=Row(static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0)));if(selected)selectedId=selected->id;
        Labels();Colors();if(!levels.empty()){ReadLevel();Filter();for(size_t i=0;i<visible.size();++i)if(Row(static_cast<int>(i))->id==selectedId){SendMessageW(list,LB_SETCURSEL,i,0);break;}}Layout();
    }
    bool Create()
    {
        Labels();
        WNDCLASSEXW cls{sizeof(cls)}; cls.lpfnWndProc=WindowProc; cls.hInstance=AfxGetInstanceHandle(); cls.lpszClassName=L"TrafficMonitor.NativeSearchMenu"; cls.hCursor=LoadCursorW(nullptr,IDC_ARROW); cls.style=CS_DROPSHADOW;
        if(!RegisterClassExW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;
        Colors();
        window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,cls.lpszClassName,L"TrafficMonitor",WS_POPUP|WS_BORDER,anchor.x,anchor.y,1,1,owner,nullptr,cls.hInstance,this);
        if(!window) return false;
        preferences.Attach(window);
        using GetDpi=UINT(WINAPI*)(HWND); auto getDpi=reinterpret_cast<GetDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow"));
        if(getDpi) dpi=getDpi(window);
        rowHeight=Px(48);
        CWnd* parent = CWnd::FromHandle(window);
        backButton.Create(backText.c_str(), CRect(0,0,1,1), parent, BackId);
        backButton.SetVariant(NativeMaterial::ButtonVariant::Text);
        back = backButton.GetSafeHwnd();
        searchField.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, CRect(0,0,1,1), parent, EditId);
        edit = searchField.GetSafeHwnd();
        list=CreateWindowExW(0,L"LISTBOX",searchText.c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|LBS_NOTIFY|LBS_OWNERDRAWVARIABLE|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT,0,0,1,1,window,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ListId)),cls.hInstance,nullptr);
        empty=CreateWindowExW(0,L"STATIC",emptyText.c_str(),WS_CHILD|SS_LEFT,0,0,1,1,window,nullptr,cls.hInstance,nullptr);
        regexButton.Create(NativePreferences::Pair(L"Regex",L"正規式"), CRect(0,0,1,1), parent, RegexId);
        regexButton.SetVariant(NativeMaterial::ButtonVariant::Tonal);
        regex = regexButton.GetSafeHwnd();
        scrollRail.Create(parent, 51005);
        scrollRail.SetStep(1);
        scrollRail.changed = [this](int position) {
            SendMessageW(list, LB_SETTOPINDEX, position, 0);
            SyncScroll();
            InvalidateRect(list, nullptr, TRUE);
        };
        Colors();
        if(!edit || !list || !back || !empty || !regex) return false;
        SetTimer(window,1,40,nullptr);
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
HRESULT MenuAccessible::get_accDescription(VARIANT child,BSTR* value)
{
    if(!value) return E_POINTER; *value=nullptr;
    if(!m_state->popup) return CO_E_OBJNOTCONNECTED;
    const Item* item=child.vt==VT_I4 ? m_state->popup->Row(child.lVal-1) : nullptr;
    if(item && (item->type&MFT_RADIOCHECK))
    { *value=SysAllocString(m_state->popup->radioText.c_str()); return *value ? S_OK : E_OUTOFMEMORY; }
    return m_native->get_accDescription(child,value);
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
    // Capture identity now; a later selection change must never retarget this action.
    return m_state->popup->QueueInvocation(child.lVal-1);
}
}

UINT Track(CMenu* menu,UINT flags,int x,int y,CWnd* ownerWindow,NativeRegex::Options* retainedSearch)
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
        popup.Open(handle,0);
        if(retainedSearch&&!popup.levels.empty()){popup.levels.front().filter->options=*retainedSearch;SetWindowTextW(popup.edit,retainedSearch->query.c_str());popup.Filter();}
        if(!popup.done) { ShowWindow(popup.window,SW_SHOW); SetForegroundWindow(popup.window); SetFocus(popup.edit); }
        MSG message{};
        while(!popup.done && IsWindow(owner) && IsWindow(popup.window))
        {
            const BOOL read=GetMessageW(&message,nullptr,0,0);
            if(read<=0) { if(read==0) PostQuitMessage(static_cast<int>(message.wParam)); break; }
            const bool editorMessage=popup.regexEditor.Contains(message.hwnd);
            const bool inside=message.hwnd==popup.window || IsChild(popup.window,message.hwnd);
            if(popup.regexEditor.Translate(&message))continue;
            if(!inside && !editorMessage && (message.message==WM_LBUTTONDOWN || message.message==WM_RBUTTONDOWN || message.message==WM_NCLBUTTONDOWN)) popup.done=true;
            if(inside && popup.Key(message)) continue;
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        popup.restoreFocus=GetForegroundWindow()==popup.window || popup.regexEditor.Contains(GetForegroundWindow());
        popup.regexEditor.Close();
        if(retainedSearch&&!popup.levels.empty())*retainedSearch=popup.levels.front().filter->options;
        while(!popup.levels.empty())
        { SendMessageW(owner,WM_UNINITMENUPOPUP,reinterpret_cast<WPARAM>(popup.levels.back().menu),0); popup.levels.pop_back(); }
        if(IsWindow(popup.window)) DestroyWindow(popup.window);
    }
    if(IsWindow(owner)) SendMessageW(owner,WM_EXITMENULOOP,TRUE,0);
    if((!created || popup.compatibility) && IsWindow(owner)) return static_cast<UINT>(menu->TrackPopupMenu(flags,x,y,ownerWindow));
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
