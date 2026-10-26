// Verification of the Material 3 tab bar and tooltip. Everything runs on a private desktop created for this
// process, so the tooltip popups and the shown test windows never appear on a user's desktop.
#include "../stdafx.h"
#include "Tabs.h"
#include "Tooltip.h"
#include "../NativePreferences/Text.h"
#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <wrl/client.h>
#include <oleacc.h>

namespace
{
    using Microsoft::WRL::ComPtr;
    using namespace NativeMaterial;
    CWinApp application;
    int failures{};
    void Check(bool value, const char* name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name); if (!value) ++failures; }

    // ----------------------------------------------------------- window class audit
    std::set<std::wstring> g_createdClasses;
    LRESULT CALLBACK CreateHook(int code, WPARAM w, LPARAM l)
    {
        if (code == HCBT_CREATEWND)
        {
            wchar_t name[128]{};
            GetClassNameW(reinterpret_cast<HWND>(w), name, 128);
            g_createdClasses.insert(name);
        }
        return CallNextHookEx(nullptr, code, w, l);
    }
    bool CreatedClass(const wchar_t* name)
    {
        for (const auto& created : g_createdClasses) if (_wcsicmp(created.c_str(), name) == 0) return true;
        return false;
    }

    // Records WM_NOTIFY and can veto TCN_SELCHANGING.
    class Host : public CWnd
    {
    public:
        std::vector<std::pair<UINT, UINT_PTR>> notes;
        bool veto{};
        int Count(UINT code) const { int n = 0; for (const auto& e : notes) if (e.first == code) ++n; return n; }
        int Total() const { return static_cast<int>(notes.size()); }
        UINT_PTR LastId() const { return notes.empty() ? 0 : notes.back().second; }
    protected:
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
        {
            if (message == WM_NOTIFY)
            {
                const auto* header = reinterpret_cast<const NMHDR*>(l);
                notes.emplace_back(header->code, header->idFrom);
                if (header->code == TCN_SELCHANGING && veto) return TRUE;
                return 0;
            }
            return CWnd::WindowProc(message, w, l);
        }
    };
    // A plain window with a registered (non-stock) class: a page or a tooltip tool.
    class Plain : public CWnd
    {
    public:
        BOOL Make(CWnd* parent, const RECT& bounds, UINT id) { return Create(AfxRegisterWndClass(0), L"", WS_CHILD | WS_VISIBLE, bounds, parent, id); }
    };

    void Pump()
    {
        MSG message;
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&message); DispatchMessage(&message); }
    }
    void PumpFor(DWORD milliseconds)
    {
        const ULONGLONG end = GetTickCount64() + milliseconds;
        while (GetTickCount64() < end) { Pump(); Sleep(5); }
    }
    bool WaitFor(const std::function<bool()>& condition, DWORD timeout)
    {
        const ULONGLONG end = GetTickCount64() + timeout;
        while (GetTickCount64() < end) { Pump(); if (condition()) return true; Sleep(5); }
        Pump();
        return condition();
    }
    LPARAM Point(int x, int y) { return MAKELPARAM(static_cast<short>(x), static_cast<short>(y)); }
    void Click(HWND window, int x, int y)
    {
        SendMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, Point(x, y));
        SendMessage(window, WM_LBUTTONUP, 0, Point(x, y));
    }
    void ClickRect(Tabs& tabs, const CRect& r) { Click(tabs.GetSafeHwnd(), r.CenterPoint().x, r.CenterPoint().y); }
    void Key(HWND window, UINT key) { SendMessage(window, WM_KEYDOWN, key, 0); SendMessage(window, WM_KEYUP, key, 0); }
    bool Visible(const CWnd& window) { return (GetWindowLong(window.GetSafeHwnd(), GWL_STYLE) & WS_VISIBLE) != 0; }
    CRect WorkArea()
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromPoint(CPoint(0, 0), MONITOR_DEFAULTTOPRIMARY), &info);
        return info.rcWork;
    }
    bool Inside(const CRect& inner, const CRect& outer)
    {
        return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right && inner.bottom <= outer.bottom;
    }

    // ----------------------------------------------------------- offscreen renders
    struct Picture { std::vector<DWORD> pixels; D2D1_COLOR_F background{}; };
    Picture Render(UINT dpi, int widthDip, int heightDip, Appearance appearance, const std::function<void(ID2D1RenderTarget*, const Palette&)>& draw,
        const std::function<D2D1_COLOR_F(const Palette&)>& backgroundOf = nullptr)
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
        Picture picture;
        picture.background = backgroundOf ? backgroundOf(palette) : palette.surface;
        if (SUCCEEDED(result))
        {
            target->BeginDraw();
            target->Clear(palette.surface);
            draw(target.Get(), palette);
            result = target->EndDraw();
        }
        if (FAILED(result)) Check(false, "real Direct2D render succeeds");
        GdiFlush();
        picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(w) * h);
        target.Reset(); factory.Reset();
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return picture;
    }
    bool Near(DWORD pixel, D2D1_COLOR_F s)
    {
        auto within = [](int a, float b) { return std::abs(a - static_cast<int>(b * 255 + .5f)) <= 2; };
        return within((pixel >> 16) & 255, s.r) && within((pixel >> 8) & 255, s.g) && within(pixel & 255, s.b);
    }
    size_t Ink(const Picture& p) { size_t n = 0; for (DWORD px : p.pixels) if (!Near(px, p.background)) ++n; return n; }
    size_t Difference(const Picture& a, const Picture& b)
    {
        if (a.pixels.size() != b.pixels.size()) return 1u << 30;
        size_t n = 0;
        for (size_t i = 0; i < a.pixels.size(); ++i) if (a.pixels[i] != b.pixels[i]) ++n;
        return n;
    }
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
        picture.background = Palette::Resolve(window.GetAppearance()).surface;
        picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(client.Width()) * client.Height());
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return SUCCEEDED(result) ? Ink(picture) : 0;
    }
    size_t PopupInk(TooltipWindow& popup)
    {
        CRect client;
        popup.GetClientRect(client);
        HDC dc = CreateCompatibleDC(nullptr);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = client.Width(); info.bmiHeader.biHeight = -client.Height();
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
        void* bits{};
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        auto previous = SelectObject(dc, bitmap);
        const HRESULT result = popup.PaintTo(dc);
        GdiFlush();
        Picture picture;
        picture.background = TooltipFill(Palette::Resolve(Appearance::Light), popup.Content().Rich());
        picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(client.Width()) * client.Height());
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return SUCCEEDED(result) ? Ink(picture) : 0;
    }

    template<class T> ComPtr<T> Pattern(IRawElementProviderSimple* provider, PATTERNID id)
    {
        ComPtr<T> pattern;
        ComPtr<IUnknown> unknown;
        if (provider && SUCCEEDED(provider->GetPatternProvider(id, unknown.GetAddressOf())) && unknown) unknown.As(&pattern);
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

    TabsModel Model(TabStyle style, int selected, int hot = -1, bool icons = false)
    {
        TabsModel model;
        model.labels = { L"General", L"Network", L"Advanced", L"Plugins" };
        model.icons = icons ? std::vector<wchar_t>{ 0xE713, 0xE774, 0xE115, 0xEA86 } : std::vector<wchar_t>{ 0, 0, 0, 0 };
        model.selected = selected; model.hot = hot; model.style = style;
        return model;
    }
}

int wmain()
{
    // Everything below happens on a private desktop so no popup can reach a user's screen.
    HDESK desktop = CreateDesktopW(L"TrafficMonitorTabsTooltipVerify", nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    if (!desktop || !SetThreadDesktop(desktop)) { std::printf("FAIL private desktop for the verification could not be created\n"); return 2; }
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HHOOK hook = SetWindowsHookExW(WH_CBT, CreateHook, nullptr, GetCurrentThreadId());
    Check(hook != nullptr, "window creation audit hook installed");

    const CRect work = WorkArea();
    Host host;
    Check(host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Tabs and tooltip verification", WS_POPUP, 40, 40, 900, 700, nullptr, 0) != FALSE, "native test host created");
    host.ShowWindow(SW_SHOWNOACTIVATE);

    // ======================================================================= tab geometry (pure)
    {
        const std::vector<float> three{ 100.f, 100.f, 100.f }, labels{ 60.f, 60.f, 60.f };
        auto fixedLayout = LayoutTabs(400.f, 48.f, three, labels, 0.f, TabStyle::Primary, 1);
        Check(!fixedLayout.scrollable && fixedLayout.tabs.size() == 3 && std::fabs((fixedLayout.tabs[0].right - fixedLayout.tabs[0].left) - 400.f / 3) < .01f,
            "tabs that fit share the width equally");
        Check(fixedLayout.tabs[1].left == fixedLayout.tabs[0].right && fixedLayout.tabs[2].right <= 400.01f, "fixed tabs are contiguous and fill the bar");
        auto scrolling = LayoutTabs(250.f, 48.f, three, labels, 0.f, TabStyle::Primary, 0);
        Check(scrolling.scrollable && scrolling.maxScroll == 50.f && scrolling.contentWidth == 300.f, "tabs that do not fit scroll with their own widths");
        scrolling = LayoutTabs(250.f, 48.f, three, labels, 999.f, TabStyle::Primary, 0);
        Check(scrolling.tabs[0].left == -50.f, "scroll offset is clamped to the overflow");
        auto stretched = LayoutTabs(500.f, 48.f, { 90.f, 300.f, 90.f }, { 40.f, 240.f, 40.f }, 0.f, TabStyle::Primary, 0);
        Check(!stretched.scrollable && std::fabs(stretched.tabs[1].right - stretched.tabs[1].left - (300.f + 20.f / 3)) < .01f, "mixed tabs that still fit keep their own width plus an equal share of the slack");
        auto primary = LayoutTabs(400.f, 48.f, three, labels, 0.f, TabStyle::Primary, 1);
        auto secondary = LayoutTabs(400.f, 48.f, three, labels, 0.f, TabStyle::Secondary, 1);
        Check(primary.indicator.bottom == 48.f && primary.indicator.bottom - primary.indicator.top == 3.f
            && primary.indicator.right - primary.indicator.left == 60.f && primary.indicator.left >= primary.tabs[1].left && primary.indicator.right <= primary.tabs[1].right,
            "primary indicator is 3 dip, label wide, centered under the active tab");
        Check(secondary.indicator.bottom - secondary.indicator.top == 2.f && secondary.indicator.left == secondary.tabs[1].left && secondary.indicator.right == secondary.tabs[1].right,
            "secondary indicator is 2 dip and spans the whole active tab");
        Check(LayoutTabs(250.f, 48.f, three, labels, 0.f, TabStyle::Primary, -1).indicator.right == 0.f, "no selection draws no indicator");
        const auto unscrolled = LayoutTabs(250.f, 48.f, three, labels, 0.f, TabStyle::Primary, 0);
        Check(TabScrollToReveal(unscrolled, 2, 250.f, 0.f) == 50.f && TabScrollToReveal(unscrolled, 0, 250.f, 50.f) == 0.f && TabScrollToReveal(unscrolled, 1, 250.f, 0.f) == 0.f,
            "reveal scrolls the least distance and leaves a visible tab alone");
        Check(TabsBarHeight(false, TabStyle::Primary) == 48.f && TabsBarHeight(true, TabStyle::Primary) == 64.f && TabsBarHeight(true, TabStyle::Secondary) == 48.f,
            "bar height is 48 dip, 64 dip for primary tabs with icons");
        const auto widths = MeasureTabWidths({ L"A", L"A very very long tab label that should be clamped to the maximum width of a tab" }, { 0, 0 });
        Check(widths[0] == TabMetrics::MinimumWidth && widths[1] == TabMetrics::MaximumWidth, "intrinsic tab widths are clamped to 90..360 dip");
    }

    // ======================================================================= fixed tab bar
    Tabs tabs;
    Check(tabs.Create(CRect(10, 10, 610, 58), &host, 201) != FALSE, "tab bar created");
    tabs.AddTab(L"General"); tabs.AddTab(L"Network"); tabs.AddTab(L"Advanced"); tabs.AddTab(L"Plugins");
    Check(tabs.GetTabCount() == 4 && tabs.GetSelected() == 0 && host.Total() == 0, "tabs are added silently and the first becomes selected");
    Check(!tabs.IsScrollable() && std::abs(tabs.TabRect(0).Width() - tabs.TabRect(3).Width()) <= 1, "a wide bar shows equal fixed tabs");
    CRect client;
    tabs.GetClientRect(client);
    {
        const CRect indicator = tabs.IndicatorRect();
        Check(indicator.bottom == client.bottom && indicator.Height() == tabs.TabRect(0).Height() * 3 / 48 && indicator.left >= tabs.TabRect(0).left && indicator.right <= tabs.TabRect(0).right,
            "the active indicator sits on the bottom edge under the selected tab");
    }
    ClickRect(tabs, tabs.TabRect(1));
    Check(tabs.GetSelected() == 1 && host.Count(TCN_SELCHANGING) == 1 && host.Count(TCN_SELCHANGE) == 1 && host.LastId() == 201,
        "tab click selects, asks TCN_SELCHANGING and notifies TCN_SELCHANGE once");
    Check(tabs.IndicatorRect().left >= tabs.TabRect(1).left && tabs.IndicatorRect().right <= tabs.TabRect(1).right, "the indicator follows the selection");
    ClickRect(tabs, tabs.TabRect(1));
    Check(host.Total() == 2, "clicking the selected tab again sends nothing");
    Key(tabs.GetSafeHwnd(), VK_RIGHT);
    Key(tabs.GetSafeHwnd(), VK_RIGHT);
    Check(tabs.GetSelected() == 3 && host.Count(TCN_SELCHANGE) == 3, "Right arrow selects the next tab and notifies");
    const int afterRight = host.Total();
    Key(tabs.GetSafeHwnd(), VK_RIGHT);
    Check(tabs.GetSelected() == 3 && host.Total() == afterRight, "Right arrow at the last tab keeps the selection and sends nothing");
    Key(tabs.GetSafeHwnd(), VK_LEFT);
    Check(tabs.GetSelected() == 2, "Left arrow selects the previous tab");
    Key(tabs.GetSafeHwnd(), VK_HOME);
    Check(tabs.GetSelected() == 0, "Home selects the first tab");
    Key(tabs.GetSafeHwnd(), VK_END);
    Check(tabs.GetSelected() == 3, "End selects the last tab");
    const int beforeSilent = host.Total();
    tabs.SetSelected(1);
    Check(tabs.GetSelected() == 1 && host.Total() == beforeSilent, "programmatic selection does not notify");
    const int changesBeforeVeto = host.Count(TCN_SELCHANGE);
    host.veto = true;
    ClickRect(tabs, tabs.TabRect(3));
    Check(tabs.GetSelected() == 1 && host.Count(TCN_SELCHANGE) == changesBeforeVeto, "the parent can veto TCN_SELCHANGING");
    host.veto = false;
    SendMessage(tabs.GetSafeHwnd(), WM_LBUTTONDOWN, MK_LBUTTON, Point(tabs.TabRect(2).CenterPoint().x, 10));
    SendMessage(tabs.GetSafeHwnd(), WM_LBUTTONUP, 0, Point(-5, -5));
    Check(tabs.GetSelected() == 1, "a press released outside the bar does not select");
    {
        tabs.SetSelected(3);
        const int total = host.Total();
        Check(tabs.HandleTabNavigation(WM_KEYDOWN, VK_TAB, true, false) && tabs.GetSelected() == 0 && host.Total() == total + 2, "Ctrl+Tab wraps from the last tab to the first and notifies");
        Check(tabs.HandleTabNavigation(WM_KEYDOWN, VK_TAB, true, true) && tabs.GetSelected() == 3, "Ctrl+Shift+Tab wraps from the first tab to the last");
        Check(tabs.HandleTabNavigation(WM_KEYDOWN, VK_NEXT, true, false) && tabs.GetSelected() == 0, "Ctrl+PageDown also cycles forward");
        Check(!tabs.HandleTabNavigation(WM_KEYDOWN, VK_TAB, false, false) && tabs.GetSelected() == 0, "a plain Tab is not consumed");
        MSG outside{ host.GetSafeHwnd(), WM_KEYDOWN, VK_TAB, 0 };
        Check(!tabs.HandleTabNavigation(MSG{ nullptr, WM_KEYDOWN, VK_TAB, 0 }), "a message for another window is not consumed");
        (void)outside;
    }
    tabs.EnableWindow(FALSE);
    {
        const int total = host.Total();
        ClickRect(tabs, tabs.TabRect(2));
        Key(tabs.GetSafeHwnd(), VK_END);
        Check(tabs.GetSelected() == 0 && host.Total() == total && !tabs.HandleTabNavigation(WM_KEYDOWN, VK_TAB, true, false), "a disabled bar ignores mouse and keyboard");
    }
    tabs.EnableWindow(TRUE);

    // ----------------------------------------------------------- page binding
    Plain page0, page1, page2;
    page0.Make(&host, CRect(10, 70, 610, 300), 301);
    page1.Make(&host, CRect(10, 70, 610, 300), 302);
    page2.Make(&host, CRect(10, 70, 610, 300), 303);
    tabs.SetSelected(0);
    tabs.BindPage(0, &page0); tabs.BindPage(1, &page1); tabs.BindPage(2, &page2);
    Check(Visible(page0) && !Visible(page1) && !Visible(page2) && tabs.GetBoundPage(1) == &page1, "binding shows only the selected tab's page");
    ClickRect(tabs, tabs.TabRect(2));
    Check(!Visible(page0) && !Visible(page1) && Visible(page2), "selecting a tab shows its page and hides the others");
    tabs.SetSelected(1);
    Check(!Visible(page0) && Visible(page1) && !Visible(page2), "programmatic selection also switches pages");
    tabs.SetSelected(3);
    Check(!Visible(page0) && !Visible(page1) && !Visible(page2), "a tab without a page hides every bound page");
    tabs.SetSelected(0);

    // ----------------------------------------------------------- automation
    {
        ComPtr<IRawElementProviderSimple> provider;
        tabs.AutomationProvider(provider.GetAddressOf());
        Check(provider && ControlType(provider.Get()) == UIA_TabControlTypeId, "the bar is exposed as a Tab control");
        auto selection = Pattern<ISelectionProvider>(provider.Get(), UIA_SelectionPatternId);
        BOOL required = FALSE, multiple = TRUE;
        Check(selection && SUCCEEDED(selection->get_IsSelectionRequired(&required)) && required && SUCCEEDED(selection->get_CanSelectMultiple(&multiple)) && !multiple,
            "the Selection pattern requires exactly one tab");
        SAFEARRAY* array{};
        bool itemOk = false;
        if (selection && SUCCEEDED(selection->GetSelection(&array)) && array)
        {
            LONG lower = 0, upper = -1;
            SafeArrayGetLBound(array, 1, &lower); SafeArrayGetUBound(array, 1, &upper);
            if (upper >= lower)
            {
                IUnknown* unknown{};
                SafeArrayGetElement(array, &lower, &unknown);
                ComPtr<IRawElementProviderSimple> item;
                if (unknown) { unknown->QueryInterface(IID_PPV_ARGS(item.GetAddressOf())); unknown->Release(); }
                auto selectionItem = Pattern<ISelectionItemProvider>(item.Get(), UIA_SelectionItemPatternId);
                BOOL selected = FALSE;
                itemOk = item && ControlType(item.Get()) == UIA_TabItemControlTypeId && Name(item.Get()) == L"General"
                    && selectionItem && SUCCEEDED(selectionItem->get_IsSelected(&selected)) && selected;
            }
            SafeArrayDestroy(array);
        }
        Check(itemOk, "the selected tab is a TabItem named by its label with SelectionItem selected");
        const int total = host.Total();
        SendMessage(tabs.GetSafeHwnd(), InputAction, static_cast<WPARAM>(InputActionId::Select), 2);
        Check(tabs.GetSelected() == 2 && host.Total() == total + 2, "an automation Select moves the tab and notifies");
        tabs.SetSelected(0);
    }

    // ======================================================================= scrolling bar
    Tabs narrow;
    Check(narrow.Create(CRect(10, 320, 260, 368), &host, 202) != FALSE, "narrow tab bar created");
    for (const wchar_t* label : { L"Overview", L"Connection", L"Notifications", L"Appearance", L"Plugins", L"Language", L"Advanced", L"About" }) narrow.AddTab(label);
    CRect narrowClient;
    narrow.GetClientRect(narrowClient);
    Check(narrow.IsScrollable() && narrow.MaximumScroll() > 0 && narrow.ScrollOffset() == 0.f, "tabs that do not fit make the bar scrollable");
    Check(narrow.TabRect(7).left >= narrowClient.right, "the last tab starts out of view");
    narrow.Select(7);
    Check(narrow.GetSelected() == 7 && narrow.ScrollOffset() > 0 && narrow.TabRect(7).right <= narrowClient.right && narrow.TabRect(7).left >= 0, "selecting an off-screen tab scrolls it fully into view");
    narrow.Select(0);
    Check(narrow.ScrollOffset() == 0.f && narrow.TabRect(0).left == 0, "selecting the first tab scrolls back");
    {
        const float before = narrow.ScrollOffset();
        SendMessage(narrow.GetSafeHwnd(), WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), 0);
        Check(narrow.ScrollOffset() > before, "the mouse wheel scrolls the bar");
        for (int i = 0; i < 30; ++i) SendMessage(narrow.GetSafeHwnd(), WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), 0);
        Check(narrow.ScrollOffset() == narrow.MaximumScroll(), "scrolling stops at the end");
        for (int i = 0; i < 60; ++i) SendMessage(narrow.GetSafeHwnd(), WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0);
        Check(narrow.ScrollOffset() == 0.f, "scrolling stops at the start");
        narrow.Select(7);
        narrow.SetWindowPos(nullptr, 0, 0, 900, 48, SWP_NOMOVE | SWP_NOZORDER);
        Pump();
        Check(!narrow.IsScrollable() && narrow.ScrollOffset() == 0.f, "widening the bar until everything fits resets the scroll");
    }
    {
        ClickRect(tabs, tabs.TabRect(0));
        Tabs wheelless;
        const BOOL created = wheelless.Create(CRect(10, 380, 610, 428), &host, 203);
        wheelless.AddTab(L"One"); wheelless.AddTab(L"Two");
        SendMessage(wheelless.GetSafeHwnd(), WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), 0);
        Check(created && wheelless.ScrollOffset() == 0.f, "a bar that fits ignores the wheel");
        wheelless.SetStyle(TabStyle::Secondary);
        Check(wheelless.GetStyle() == TabStyle::Secondary, "the style can be switched");
        wheelless.DestroyWindow();
    }

    // ======================================================================= tab rendering
    {
        for (UINT dpi : { 96u, 144u })
            for (Appearance appearance : { Appearance::Light, Appearance::Dark })
                for (TabStyle style : { TabStyle::Primary, TabStyle::Secondary })
                {
                    const std::string tag = std::string(dpi == 96 ? "96" : "144") + (appearance == Appearance::Light ? " light " : " dark ") + (style == TabStyle::Primary ? "primary" : "secondary");
                    const InputState normal{};
                    const auto first = Render(dpi, 600, 48, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(style, 0), normal); });
                    const auto second = Render(dpi, 600, 48, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(style, 1), normal); });
                    Check(Ink(first) > 400 && Ink(second) > 400, ("tab render is not blank: " + tag).c_str());
                    Check(Difference(first, second) > 200, ("moving the selection changes the render: " + tag).c_str());
                    const auto hot = Render(dpi, 600, 48, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(style, 0, 2), normal); });
                    Check(Difference(first, hot) > 200, ("hover draws a state layer: " + tag).c_str());
                    InputState focused; focused.focused = true;
                    const auto focus = Render(dpi, 600, 48, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(style, 0), focused); });
                    Check(Difference(first, focus) > 100, ("keyboard focus draws a ring: " + tag).c_str());
                    InputState off; off.enabled = false;
                    const auto disabled = Render(dpi, 600, 48, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(style, 0), off); });
                    Check(Difference(first, disabled) > 200, ("disabled tabs are dimmed: " + tag).c_str());
                }
        const auto light = Render(96, 600, 48, Appearance::Light, [](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(TabStyle::Primary, 0), InputState{}); });
        const auto dark = Render(96, 600, 48, Appearance::Dark, [](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(TabStyle::Primary, 0), InputState{}); });
        Check(Difference(light, dark) > 10000, "light and dark tab renders differ");
        const auto plain = Render(96, 600, 64, Appearance::Light, [](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(TabStyle::Primary, 0, -1, false), InputState{}); });
        const auto icons = Render(96, 600, 64, Appearance::Light, [](ID2D1RenderTarget* t, const Palette& p) { DrawTabs(t, p, Model(TabStyle::Primary, 0, -1, true), InputState{}); });
        Check(Ink(icons) > Ink(plain) && Difference(plain, icons) > 400, "tabs with icons draw the glyphs");
        Check(WindowInk(tabs) > 400 && WindowInk(narrow) > 400, "real tab windows paint through WM_PRINTCLIENT");
        CString stock;
        Check(!CreatedClass(L"SysTabControl32") && !CreatedClass(L"tooltips_class32"), "no SysTabControl32 or tooltips_class32 window was created by the tab bar");
    }

    // ======================================================================= tooltip geometry (pure)
    {
        const CRect area(0, 0, 1000, 800);
        const CSize size(200, 40);
        auto below = PlaceTooltip(size, CRect(100, 100, 101, 120), 100, area, 2);
        Check(below.left == 100 && below.top == 122 && below.Width() == 200 && below.Height() == 40, "placement puts the tip below the pointer with the gap");
        auto shifted = PlaceTooltip(size, CRect(950, 100, 951, 120), 950, area, 2);
        Check(shifted.right == 1000 && shifted.left == 800, "placement shifts left to stay in the work area");
        auto flipped = PlaceTooltip(size, CRect(100, 770, 101, 790), 100, area, 2);
        Check(flipped.bottom <= 770 - 2 + 0 && flipped.top == 770 - 2 - 40, "placement flips above when there is no room below");
        auto corner = PlaceTooltip(size, CRect(990, 790, 991, 799), 990, area, 2);
        Check(Inside(corner, area) && corner.bottom <= 790, "placement in the bottom right corner stays inside and above the pointer");
        auto left = PlaceTooltip(size, CRect(-50, 100, -49, 120), -50, area, 2);
        Check(left.left == 0, "placement clamps to the left edge");
        auto huge = PlaceTooltip(CSize(1200, 900), CRect(100, 100, 101, 120), 100, area, 2);
        Check(huge.left == 0 && huge.top == 0, "a tip larger than the work area is pinned to its top left corner");
        auto offset = PlaceTooltip(size, CRect(1100, 1000, 1101, 1020), 1100, CRect(1000, 600, 1900, 1100), 2);
        Check(Inside(offset, CRect(1000, 600, 1900, 1100)), "placement honors a work area that does not start at the origin");
    }

    // ======================================================================= tooltip behavior
    Tooltip tip;
    Check(tip.Create() != FALSE, "tooltip popup created");
    Plain tool, richTool, edgeToolHost;
    tool.Make(&host, CRect(700, 20, 860, 70), 401);
    richTool.Make(&host, CRect(700, 90, 860, 140), 402);
    tip.AddTool(&tool, L"Switches to the dark appearance");
    tip.AddTool(&richTool, L"Appearance", L"Choose between the light and the dark appearance. The change applies to every open window immediately.");
    Check(tip.HasTool(&tool) && tip.GetTipText(&tool) == L"Switches to the dark appearance", "AddTool registers a tool and keeps its text");
    tip.SetInitialDelay(500);
    HWND foreground = GetForegroundWindow();
    HWND active = GetActiveWindow();
    {
        const ULONGLONG start = GetTickCount64();
        SendMessage(tool.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(10, 10));
        Check(!tip.IsVisible(), "the tip is not shown the moment the pointer arrives");
        PumpFor(300);
        Check(!tip.IsVisible(), "the tip is still hidden before the delay has passed");
        SendMessage(tool.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(14, 12));
        PumpFor(300);
        Check(!tip.IsVisible(), "moving the pointer restarts the delay");
        const bool shown = WaitFor([&] { return tip.IsVisible(); }, 2000);
        const ULONGLONG elapsed = GetTickCount64() - start;
        Check(shown && elapsed >= 750 && elapsed <= 1800, "the tip appears once the pointer has rested for the initial delay");
        Check(tip.ActiveTool() == &tool || (tip.ActiveTool() && tip.ActiveTool()->m_hWnd == tool.m_hWnd), "the tip belongs to the hovered tool");
        Check(tip.ShownText() == L"Switches to the dark appearance", "the tip shows the registered text");
        CPoint pointer(14, 12);
        ::ClientToScreen(tool.m_hWnd, &pointer);
        const CRect bounds = tip.Bounds();
        Check(Inside(bounds, work) && bounds.top >= pointer.y && bounds.left >= pointer.x - 1, "the tip sits below the pointer inside the work area");
        const LONG ex = GetWindowLong(tip.PopupWindow(), GWL_EXSTYLE);
        Check((ex & WS_EX_NOACTIVATE) && (ex & WS_EX_TOPMOST) && (ex & WS_EX_LAYERED), "the popup is layered, topmost and no-activate");
        Check(SendMessage(tip.PopupWindow(), WM_NCHITTEST, 0, 0) == HTTRANSPARENT && SendMessage(tip.PopupWindow(), WM_MOUSEACTIVATE, 0, 0) == MA_NOACTIVATE,
            "the popup is mouse-transparent and refuses activation");
        Check(GetForegroundWindow() == foreground && GetActiveWindow() == active, "showing the tip does not change the foreground or active window");
        wchar_t className[64]{};
        GetClassNameW(tip.PopupWindow(), className, 64);
        Check(_wcsicmp(className, L"tooltips_class32") != 0 && PopupInk(tip.Popup()) > 100, "the popup is a self-drawn window with visible text");
        ComPtr<IAccessible> accessible;
        Check(SUCCEEDED(AccessibleObjectFromWindow(tip.PopupWindow(), OBJID_CLIENT, IID_PPV_ARGS(accessible.GetAddressOf()))) && accessible, "the popup has an accessible object");
        if (accessible)
        {
            VARIANT self{}; self.vt = VT_I4; self.lVal = CHILDID_SELF;
            VARIANT role{}; BSTR name{};
            const bool roleOk = SUCCEEDED(accessible->get_accRole(self, &role)) && role.vt == VT_I4 && role.lVal == ROLE_SYSTEM_TOOLTIP;
            const bool nameOk = SUCCEEDED(accessible->get_accName(self, &name)) && name && CString(name) == L"Switches to the dark appearance";
            SysFreeString(name);
            Check(roleOk, "the popup exposes the ToolTip role");
            Check(nameOk, "the popup is named by the tip text");
        }
        ComPtr<IAccessible> targetAccessible;
        BSTR help{};
        VARIANT self{}; self.vt = VT_I4; self.lVal = CHILDID_SELF;
        const bool helpOk = SUCCEEDED(AccessibleObjectFromWindow(tool.m_hWnd, OBJID_CLIENT, IID_PPV_ARGS(targetAccessible.GetAddressOf()))) && targetAccessible
            && SUCCEEDED(targetAccessible->get_accHelp(self, &help)) && help && CString(help) == L"Switches to the dark appearance";
        SysFreeString(help);
        Check(helpOk, "the registered tool carries the tip text as its help text");
    }
    SendMessage(tool.GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);
    Check(!tip.IsVisible(), "pointer leave hides the tip");
    // Dismissals while the tip is up.
    struct Dismissal { const char* name; std::function<void()> act; };
    const Dismissal dismissals[] = {
        { "a mouse button press hides the tip", [&] { SendMessage(tool.GetSafeHwnd(), WM_LBUTTONDOWN, MK_LBUTTON, Point(5, 5)); SendMessage(tool.GetSafeHwnd(), WM_LBUTTONUP, 0, Point(5, 5)); } },
        { "a key press hides the tip", [&] { SendMessage(tool.GetSafeHwnd(), WM_KEYDOWN, VK_SPACE, 0); } },
        { "the mouse wheel hides the tip", [&] { SendMessage(tool.GetSafeHwnd(), WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0); } },
    };
    for (const auto& dismissal : dismissals)
    {
        SendMessage(tool.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(10, 10));
        tip.ShowFor(&tool, false);
        const bool wasVisible = tip.IsVisible();
        dismissal.act();
        Check(wasVisible && !tip.IsVisible(), dismissal.name);
        SendMessage(tool.GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);
    }
    // A pending tip is cancelled when the pointer leaves before the delay.
    SendMessage(tool.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(10, 10));
    SendMessage(tool.GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);
    PumpFor(800);
    Check(!tip.IsVisible(), "leaving before the delay cancels the pending tip");

    // The delay can be skipped.
    tip.SetInitialDelay(0);
    SendMessage(tool.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(10, 10));
    Check(tip.IsVisible(), "a zero delay shows the tip at once");
    SendMessage(tool.GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);
    tip.SetInitialDelay(500);

    // Text updates reach a visible tip.
    SendMessage(tool.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(10, 10));
    tip.ShowFor(&tool, false);
    tip.UpdateTipText(L"Switches to the light appearance", &tool);
    Check(tip.ShownText() == L"Switches to the light appearance" && tip.GetTipText(&tool) == L"Switches to the light appearance", "UpdateTipText refreshes a tip that is showing");
    SendMessage(tool.GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);

    // Keyboard focus.
    {
        const ULONGLONG start = GetTickCount64();
        tip.NotifyFocus(&tool, true);
        Check(!tip.IsVisible(), "keyboard focus does not show the tip before the delay");
        const bool shown = WaitFor([&] { return tip.IsVisible(); }, 2000);
        Check(shown && GetTickCount64() - start >= 400, "keyboard focus shows the tip after the delay");
        CRect target;
        tool.GetWindowRect(target);
        const CRect bounds = tip.Bounds();
        Check(bounds.top >= target.bottom && Inside(bounds, work) && std::abs(bounds.CenterPoint().x - target.CenterPoint().x) <= bounds.Width() / 2 + 1, "the focus tip sits below the focused tool");
        SendMessage(tool.GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);
        Check(tip.IsVisible(), "pointer leave does not hide a tip shown for keyboard focus");
        SendMessage(tool.GetSafeHwnd(), WM_KILLFOCUS, 0, 0);
        Check(!tip.IsVisible(), "losing keyboard focus hides the tip");
        tip.NotifyFocus(&tool, false);
        PumpFor(800);
        Check(!tip.IsVisible(), "a focus that did not come from the keyboard never shows a tip");
        tip.NotifyFocus(&tool, true);
        SendMessage(tool.GetSafeHwnd(), WM_KEYDOWN, VK_ESCAPE, 0);
        PumpFor(700);
        Check(!tip.IsVisible(), "a key press cancels a pending focus tip");
    }

    // Rich tooltip.
    {
        tip.ShowFor(&richTool, true);
        Check(tip.IsVisible() && tip.Popup().Content().Rich() && tip.Popup().Content().title == L"Appearance", "a tool registered with a title shows a rich tip");
        const CRect rich = tip.Bounds();
        Check(rich.Width() <= static_cast<int>(MulDiv(312, GetDpiForWindow(richTool.m_hWnd), 96)) + 2 && rich.Height() > 48 && PopupInk(tip.Popup()) > 300, "the rich tip wraps at 312 dip and draws its title and text");
        tip.Hide();
        Check(!tip.IsVisible(), "Hide removes the tip");
    }

    // Activation and removal.
    tip.Activate(false);
    Check(!tip.ShowFor(&tool, false) && !tip.IsVisible(), "an inactive tooltip never shows");
    tip.Activate(true);
    {
        Plain doomed;
        doomed.Make(&host, CRect(700, 160, 860, 200), 403);
        tip.AddTool(&doomed, L"Temporary");
        SendMessage(doomed.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(5, 5));
        tip.ShowFor(&doomed, false);
        const HWND handle = doomed.m_hWnd;
        doomed.DestroyWindow();
        Check(!tip.IsVisible() && !tip.HasTool(CWnd::FromHandle(handle)), "destroying a tool hides its tip and removes it");
        Plain removed;
        removed.Make(&host, CRect(700, 160, 860, 200), 404);
        tip.AddTool(&removed, L"Removed");
        tip.RemoveTool(&removed);
        SendMessage(removed.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(5, 5));
        tip.SetInitialDelay(0);
        SendMessage(removed.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(6, 6));
        Check(!tip.HasTool(&removed) && !tip.IsVisible(), "a removed tool no longer shows a tip");
        tip.SetInitialDelay(500);
        Plain disabled;
        disabled.Make(&host, CRect(700, 160, 860, 200), 405);
        tip.AddTool(&disabled, L"Disabled tool");
        tip.ShowFor(&disabled, false);
        disabled.EnableWindow(FALSE);
        Check(!tip.IsVisible(), "disabling the tool hides its tip");
        Check(!tip.ShowFor(&disabled, false), "a disabled tool cannot show a tip");
        tip.RemoveTool(&disabled);
        removed.DestroyWindow(); disabled.DestroyWindow();
    }

    // Placement against the real work area edges.
    {
        Host edgeHost;
        edgeHost.CreateEx(0, AfxRegisterWndClass(0), L"edge", WS_POPUP, work.right - 60, work.bottom - 40, 60, 40, nullptr, 0);
        edgeHost.ShowWindow(SW_SHOWNOACTIVATE);
        edgeToolHost.Make(&edgeHost, CRect(0, 0, 60, 40), 410);
        Tooltip edgeTip;
        edgeTip.AddTool(&edgeToolHost, L"This is a long supporting text that has to be placed next to the bottom right corner of the work area.");
        edgeTip.SetInitialDelay(0);
        SendMessage(edgeToolHost.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(50, 30));
        CPoint pointer(50, 30);
        ::ClientToScreen(edgeToolHost.m_hWnd, &pointer);
        const CRect pointerBounds = edgeTip.Bounds();
        Check(edgeTip.IsVisible() && Inside(pointerBounds, work) && pointerBounds.bottom <= pointer.y, "a tip next to the bottom right corner is clamped inside the work area and flips above the pointer");
        edgeTip.Hide();
        edgeTip.ShowFor(&edgeToolHost, true);
        CRect target;
        edgeToolHost.GetWindowRect(target);
        Check(edgeTip.IsVisible() && Inside(edgeTip.Bounds(), work) && edgeTip.Bounds().bottom <= target.top, "a focus tip at the bottom edge flips above the tool inside the work area");
        edgeTip.RemoveAllTools();
        edgeToolHost.DestroyWindow();
        edgeHost.DestroyWindow();
    }

    // Motion.
    {
        Tooltip::SetAnimationOverride(0);
        SendMessage(tool.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(10, 10));
        tip.ShowFor(&tool, false);
        Check(tip.IsVisible() && tip.Alpha() == 255, "with animation off (reduced motion) the tip appears at full opacity at once");
        tip.Hide();
        Tooltip::SetAnimationOverride(1);
        tip.ShowFor(&tool, false);
        const BYTE start = tip.Alpha();
        const bool settled = WaitFor([&] { return tip.Alpha() == 255; }, 1000);
        Check(tip.IsVisible() && start < 255 && settled, "with animation on the tip fades in and reaches full opacity");
        tip.Hide();
        Tooltip::SetAnimationOverride(-1);
    }

    // Offscreen renders of the tooltip content.
    {
        const TooltipContent plainContent{ L"", L"Switches to the dark appearance" };
        const TooltipContent richContent{ L"Appearance", L"Choose between the light and the dark appearance. The change applies to every open window immediately." };
        for (UINT dpi : { 96u, 144u })
            for (Appearance appearance : { Appearance::Light, Appearance::Dark })
                for (const auto* content : { &plainContent, &richContent })
                {
                    const auto size = MeasureTooltip(*content);
                    const std::string tag = std::string(dpi == 96 ? "96 " : "144 ") + (appearance == Appearance::Light ? "light " : "dark ") + (content->Rich() ? "rich" : "plain");
                    const bool rich = content->Rich();
                    const auto picture = Render(dpi, static_cast<int>(std::ceil(size.width)), static_cast<int>(std::ceil(size.height)), appearance,
                        [&](ID2D1RenderTarget* t, const Palette& p) { DrawTooltip(t, p, *content); },
                        [&](const Palette& p) { return TooltipFill(p, rich); });
                    Check(Ink(picture) > 60, ("tooltip render is not blank: " + tag).c_str());
                }
        const TooltipContent empty{ L"", L"" };
        const auto blank = Render(96, 80, 24, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTooltip(t, p, empty); },
            [](const Palette& p) { return TooltipFill(p, false); });
        Check(Ink(blank) == 0, "an empty tip renders only its container");
        const auto light = Render(96, 200, 40, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTooltip(t, p, plainContent); });
        const auto dark = Render(96, 200, 40, Appearance::Dark, [&](ID2D1RenderTarget* t, const Palette& p) { DrawTooltip(t, p, plainContent); });
        Check(Difference(light, dark) > 3000, "light and dark tooltip renders differ");
        const auto plainSize = MeasureTooltip(plainContent), richSize = MeasureTooltip(richContent);
        Check(plainSize.height >= TooltipMetrics::PlainMinHeight && richSize.height > plainSize.height && richSize.width <= 312.f + .01f && richSize.width >= TooltipMetrics::RichMinWidth, "measured tip sizes follow the plain and rich metrics");
    }

    Check(!CreatedClass(L"tooltips_class32") && !CreatedClass(L"SysTabControl32"), "no tooltips_class32 or SysTabControl32 window was created during the whole run");

    tip.RemoveAllTools();
    if (hook) UnhookWindowsHookEx(hook);
    return failures ? 1 : 0;
}
