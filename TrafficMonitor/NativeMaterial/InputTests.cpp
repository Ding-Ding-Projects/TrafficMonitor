// Hidden-window verification of the Material 3 input components. Each component is created in a real,
// never-shown parent and driven with window messages. Nothing here needs a visible desktop.
#include "../stdafx.h"
#include "InputControls.h"
#include "../NativePreferences/Text.h"
#include "../NativeMenus/SearchableMenu.h"
#include <cstdio>
#include <functional>
#include <wrl/client.h>
#include <oleacc.h>

// The focused test links only the input sources, so the two application services they reach are stubbed.
namespace NativePreferences { Snapshot Current() { return Snapshot{}; } }
namespace NativeMenus { UINT Track(CMenu*, UINT, int, int, CWnd*, NativeRegex::Options*) { return 0; } }

namespace
{
    using Microsoft::WRL::ComPtr;
    using namespace NativeMaterial;
    CWinApp application;
    int failures{};
    void Check(bool value, const char* name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name); if (!value) ++failures; }

    // Records every WM_COMMAND a component sends to its parent.
    class Host : public CWnd
    {
    public:
        std::vector<std::pair<UINT, UINT>> log;
        int Count(UINT id, UINT code) const
        {
            int n = 0;
            for (const auto& entry : log) if (entry.first == id && entry.second == code) ++n;
            return n;
        }
        int Total() const { return static_cast<int>(log.size()); }
    protected:
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
        {
            if (message == WM_COMMAND) log.emplace_back(LOWORD(w), HIWORD(w));
            return CWnd::WindowProc(message, w, l);
        }
    };

    void Pump()
    {
        MSG message;
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&message); DispatchMessage(&message); }
    }
    LPARAM Point(int x, int y) { return MAKELPARAM(static_cast<short>(x), static_cast<short>(y)); }
    void Click(HWND window, int x, int y)
    {
        SendMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, Point(x, y));
        SendMessage(window, WM_LBUTTONUP, 0, Point(x, y));
    }
    void ClickCenter(CWnd& window)
    {
        CRect client;
        window.GetClientRect(client);
        Click(window.GetSafeHwnd(), client.CenterPoint().x, client.CenterPoint().y);
    }
    void Key(HWND window, UINT key) { SendMessage(window, WM_KEYDOWN, key, 0); SendMessage(window, WM_KEYUP, key, 0); }
    void Chars(HWND window, const wchar_t* text) { for (; *text; ++text) SendMessage(window, WM_CHAR, *text, 0); }
    CString TextOf(HWND window) { CString text; CWnd::FromHandle(window)->GetWindowText(text); return text; }
    int Px(CWnd& window, float dip) { return static_cast<int>(std::lround(dip * GetDpiForWindow(window.GetSafeHwnd()) / 96.0)); }
    float Dip(CWnd& window, int px) { return px * 96.f / GetDpiForWindow(window.GetSafeHwnd()); }

    // No generic system control may appear anywhere below a component.
    bool HasStockChild(HWND root, CString* found = nullptr)
    {
        struct Context { bool bad{}; CString name; } context;
        EnumChildWindows(root, [](HWND child, LPARAM l) -> BOOL
        {
            auto* c = reinterpret_cast<Context*>(l);
            wchar_t name[128]{};
            GetClassNameW(child, name, 128);
            for (const wchar_t* stock : { L"Button", L"ComboBox", L"ComboLBox", L"msctls_updown32", L"msctls_trackbar32" })
                if (_wcsicmp(name, stock) == 0) { c->bad = true; c->name = name; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&context));
        if (found) *found = context.name;
        return context.bad;
    }
    bool IsStockClass(HWND window)
    {
        wchar_t name[128]{};
        GetClassNameW(window, name, 128);
        for (const wchar_t* stock : { L"Button", L"ComboBox", L"ComboLBox", L"msctls_updown32", L"msctls_trackbar32" })
            if (_wcsicmp(name, stock) == 0) return true;
        return false;
    }

    // ----------------------------------------------------------- offscreen renders
    struct Picture { std::vector<DWORD> pixels; D2D1_COLOR_F surface{}; };
    Picture Render(UINT dpi, int widthDip, int heightDip, Appearance appearance, const std::function<void(ID2D1RenderTarget*, const Palette&)>& draw)
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
        if (SUCCEEDED(result))
        {
            target->BeginDraw();
            target->Clear(palette.surface);
            draw(target.Get(), palette);
            result = target->EndDraw();
        }
        if (FAILED(result)) Check(false, "real Direct2D render succeeds");
        GdiFlush();
        Picture picture;
        picture.surface = palette.surface;
        picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(w) * h);
        target.Reset(); factory.Reset();
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return picture;
    }
    bool NearSurface(DWORD pixel, D2D1_COLOR_F s)
    {
        auto within = [](int a, float b) { return std::abs(a - static_cast<int>(b * 255 + .5f)) <= 2; };
        return within((pixel >> 16) & 255, s.r) && within((pixel >> 8) & 255, s.g) && within(pixel & 255, s.b);
    }
    size_t Ink(const Picture& p) { size_t n = 0; for (DWORD px : p.pixels) if (!NearSurface(px, p.surface)) ++n; return n; }
    size_t Difference(const Picture& a, const Picture& b)
    {
        if (a.pixels.size() != b.pixels.size()) return 1u << 30;
        size_t n = 0;
        for (size_t i = 0; i < a.pixels.size(); ++i) if (a.pixels[i] != b.pixels[i]) ++n;
        return n;
    }
    // Paints a real window through its WM_PRINTCLIENT route into a DIB.
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
        picture.surface = Palette::Resolve(window.GetAppearance()).surface;
        picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(client.Width()) * client.Height());
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return SUCCEEDED(result) ? Ink(picture) : 0;
    }

    // ------------------------------------------------------------ automation helpers
    template<class T> ComPtr<T> Pattern(IRawElementProviderSimple* provider, PATTERNID id)
    {
        ComPtr<T> pattern;
        ComPtr<IUnknown> unknown;
        if (provider && SUCCEEDED(provider->GetPatternProvider(id, unknown.GetAddressOf())) && unknown)
            unknown.As(&pattern);
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
    ComPtr<IRawElementProviderSimple> Provider(InputWindow& window)
    {
        ComPtr<IRawElementProviderSimple> provider;
        window.AutomationProvider(provider.GetAddressOf());
        return provider;
    }
}

int wmain()
{
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Host host;
    Check(host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Material input verification", WS_POPUP, 0, 0, 900, 900, nullptr, 0) != FALSE, "hidden native parent created");

    // ----------------------------------------------------------------- Switch
    Switch sw;
    Check(sw.Create(L"Dark mode", CRect(10, 10, 250, 58), &host, 101) != FALSE, "switch created");
    host.log.clear();
    Check(!sw.IsChecked(), "switch starts off");
    ClickCenter(sw);
    Check(sw.IsChecked() && host.Count(101, BN_CLICKED) == 1, "switch click turns it on and notifies the parent once");
    ClickCenter(sw);
    Check(!sw.IsChecked() && host.Count(101, BN_CLICKED) == 2, "switch click turns it off and notifies again");
    Key(sw.GetSafeHwnd(), VK_SPACE);
    Check(sw.IsChecked() && host.Count(101, BN_CLICKED) == 3, "switch Space toggles and notifies");
    Key(sw.GetSafeHwnd(), VK_RETURN);
    Check(sw.IsChecked() && host.Count(101, BN_CLICKED) == 3, "Enter does not toggle a switch");
    sw.SetChecked(false);
    Check(!sw.IsChecked() && host.Count(101, BN_CLICKED) == 3, "programmatic switch change does not notify");
    sw.SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, Point(20, 20));
    sw.SendMessage(WM_LBUTTONUP, 0, Point(-5, -5));
    Check(!sw.IsChecked() && host.Count(101, BN_CLICKED) == 3, "press released outside does not toggle a switch");
    sw.EnableWindow(FALSE);
    ClickCenter(sw);
    Key(sw.GetSafeHwnd(), VK_SPACE);
    Check(!sw.IsChecked() && host.Count(101, BN_CLICKED) == 3, "disabled switch ignores mouse and keyboard");
    sw.EnableWindow(TRUE);
    {
        auto provider = Provider(sw);
        Check(Name(provider.Get()) == L"Dark mode" && ControlType(provider.Get()) == UIA_ButtonControlTypeId, "switch exposes its name and a button role");
        auto toggle = Pattern<IToggleProvider>(provider.Get(), UIA_TogglePatternId);
        ToggleState state{};
        Check(toggle && SUCCEEDED(toggle->get_ToggleState(&state)) && state == ToggleState_Off, "switch Toggle pattern reports off");
        if (toggle) toggle->Toggle();
        Pump();
        Check(sw.IsChecked() && host.Count(101, BN_CLICKED) == 4, "automation Toggle changes the switch and notifies");
        toggle->get_ToggleState(&state);
        Check(state == ToggleState_On, "switch Toggle pattern reports on after change");
    }
    sw.SetChecked(false);

    // --------------------------------------------------------------- Checkbox
    Checkbox cb;
    Check(cb.Create(L"Accept terms", CRect(10, 70, 250, 118), &host, 102) != FALSE, "checkbox created");
    CRect cbClient;
    cb.GetClientRect(cbClient);
    Click(cb.GetSafeHwnd(), cbClient.right - 12, cbClient.CenterPoint().y);
    Check(cb.IsChecked() && host.Count(102, BN_CLICKED) == 1, "clicking the label area toggles the checkbox");
    Key(cb.GetSafeHwnd(), VK_SPACE);
    Check(!cb.IsChecked() && host.Count(102, BN_CLICKED) == 2, "checkbox Space toggles");
    cb.SetCheckState(CheckState::Indeterminate);
    Check(cb.GetCheckState() == CheckState::Indeterminate && host.Count(102, BN_CLICKED) == 2, "checkbox indeterminate is programmatic and silent");
    {
        auto provider = Provider(cb);
        auto toggle = Pattern<IToggleProvider>(provider.Get(), UIA_TogglePatternId);
        ToggleState state{};
        Check(toggle && SUCCEEDED(toggle->get_ToggleState(&state)) && state == ToggleState_Indeterminate && ControlType(provider.Get()) == UIA_CheckBoxControlTypeId,
            "checkbox exposes the indeterminate Toggle state and a check box role");
    }
    ClickCenter(cb);
    Check(cb.GetCheckState() == CheckState::Checked, "clicking an indeterminate checkbox checks it");
    cb.SetErrorState(true);
    Check(cb.IsChecked(), "error state keeps the checkbox value");
    cb.SetErrorState(false);

    // ------------------------------------------------------------ RadioButton
    RadioButton r1, r2, r3, r4, r5;
    Check(r1.Create(L"Small", CRect(10, 130, 250, 178), &host, 111, true) && r2.Create(L"Medium", CRect(10, 180, 250, 228), &host, 112)
        && r3.Create(L"Large", CRect(10, 230, 250, 278), &host, 113) && r4.Create(L"Left", CRect(300, 130, 500, 178), &host, 114, true)
        && r5.Create(L"Right", CRect(300, 180, 500, 228), &host, 115), "radio buttons created in two groups");
    auto tabStops = [](RadioButton& a) { return (a.GetStyle() & WS_TABSTOP) != 0; };
    Check(tabStops(r1) && !tabStops(r2) && !tabStops(r3), "an unchecked group exposes exactly one tab stop");
    host.log.clear();
    ClickCenter(r2);
    Check(r2.IsChecked() && !r1.IsChecked() && !r3.IsChecked() && host.Count(112, BN_CLICKED) == 1, "choosing a radio clears the rest of its group and notifies");
    Check(tabStops(r2) && !tabStops(r1) && !tabStops(r3), "the checked radio is the single tab stop");
    Key(r2.GetSafeHwnd(), VK_DOWN);
    Check(r3.IsChecked() && !r2.IsChecked() && host.Count(113, BN_CLICKED) == 1, "arrow key moves the selection within the group");
    Key(r3.GetSafeHwnd(), VK_DOWN);
    Check(r1.IsChecked() && host.Count(111, BN_CLICKED) == 1, "arrow key wraps to the first radio");
    Key(r1.GetSafeHwnd(), VK_UP);
    Check(r3.IsChecked(), "up arrow wraps to the last radio");
    ClickCenter(r5);
    Check(r5.IsChecked() && r3.IsChecked(), "a radio in another group does not clear this group");
    r1.SetChecked(true);
    Check(r1.IsChecked() && !r3.IsChecked() && host.Count(111, BN_CLICKED) == 1, "programmatic radio selection is silent but keeps the group exclusive");
    r2.EnableWindow(FALSE);
    Key(r1.GetSafeHwnd(), VK_DOWN);
    Check(r3.IsChecked() && !r2.IsChecked(), "arrow navigation skips a disabled radio");
    r2.EnableWindow(TRUE);
    {
        auto provider = Provider(r3);
        auto item = Pattern<ISelectionItemProvider>(provider.Get(), UIA_SelectionItemPatternId);
        BOOL selected = FALSE;
        Check(item && SUCCEEDED(item->get_IsSelected(&selected)) && selected && ControlType(provider.Get()) == UIA_RadioButtonControlTypeId, "radio exposes SelectionItem state and a radio role");
        auto other = Provider(r1);
        auto otherItem = Pattern<ISelectionItemProvider>(other.Get(), UIA_SelectionItemPatternId);
        if (otherItem) otherItem->Select();
        Pump();
        Check(r1.IsChecked() && !r3.IsChecked(), "automation Select chooses the radio");
    }

    // ------------------------------------------------------------- RadioGroup
    RadioGroup group;
    Check(group.Create(CRect(10, 300, 250, 450), &host, 120) != FALSE, "radio group created");
    group.SetOptions({ L"One", L"Two", L"Three" });
    group.SetSelected(0);
    host.log.clear();
    Check((group.GetStyle() & WS_TABSTOP) != 0, "radio group is a single tab stop");
    Click(group.GetSafeHwnd(), 40, Px(group, 48 * 2 + 24));
    Check(group.GetSelected() == 2 && host.Count(120, CBN_SELCHANGE) == 1, "clicking a radio row selects it and notifies");
    Click(group.GetSafeHwnd(), 40, Px(group, 48 * 2 + 24));
    Check(host.Count(120, CBN_SELCHANGE) == 1, "reselecting the same row does not notify");
    Key(group.GetSafeHwnd(), VK_UP);
    Check(group.GetSelected() == 1, "up arrow selects the previous option");
    Key(group.GetSafeHwnd(), VK_HOME);
    Check(group.GetSelected() == 0, "Home selects the first option");
    Key(group.GetSafeHwnd(), VK_END);
    Check(group.GetSelected() == 2, "End selects the last option");
    Key(group.GetSafeHwnd(), VK_DOWN);
    Check(group.GetSelected() == 0, "down arrow wraps to the first option");
    group.SetSelected(1);
    Check(host.Count(120, CBN_SELCHANGE) == 5, "programmatic radio group change does not notify");
    {
        auto provider = Provider(group);
        auto selection = Pattern<ISelectionProvider>(provider.Get(), UIA_SelectionPatternId);
        SAFEARRAY* selected = nullptr;
        bool ok = selection && SUCCEEDED(selection->GetSelection(&selected)) && selected && selected->rgsabound[0].cElements == 1;
        if (ok)
        {
            IUnknown* unknown = nullptr; LONG index = 0;
            SafeArrayGetElement(selected, &index, &unknown);
            ComPtr<IRawElementProviderSimple> item;
            if (unknown) { unknown->QueryInterface(IID_PPV_ARGS(item.GetAddressOf())); unknown->Release(); }
            ok = item && Name(item.Get()) == L"Two";
        }
        if (selected) SafeArrayDestroy(selected);
        Check(ok, "radio group Selection pattern reports the chosen option");
    }

    // ------------------------------------------------------- SegmentedButton
    SegmentedButton seg;
    Check(seg.Create(CRect(10, 470, 330, 518), &host, 130) != FALSE, "segmented button created");
    Check(!seg.SetSegments({ L"Only" }) && !seg.SetSegments({ L"1", L"2", L"3", L"4", L"5", L"6" }) && seg.SetSegments({ L"Day", L"Week", L"Month" }), "segmented button accepts exactly two to five segments");
    Check(seg.GetSelected() == 0, "segmented button starts with the first segment selected");
    host.log.clear();
    {
        CRect client;
        seg.GetClientRect(client);
        const auto rects = SegmentRects(Dip(seg, client.Width()), Dip(seg, client.Height()), 3);
        Click(seg.GetSafeHwnd(), Px(seg, (rects[2].left + rects[2].right) / 2), Px(seg, (rects[2].top + rects[2].bottom) / 2));
    }
    Check(seg.GetSelected() == 2 && host.Count(130, CBN_SELCHANGE) == 1, "clicking a segment selects it and notifies");
    Key(seg.GetSafeHwnd(), VK_LEFT);
    Check(seg.GetSelected() == 1 && host.Count(130, CBN_SELCHANGE) == 2, "left arrow selects the previous segment");
    Key(seg.GetSafeHwnd(), VK_END);
    Key(seg.GetSafeHwnd(), VK_RIGHT);
    Check(seg.GetSelected() == 2 && host.Count(130, CBN_SELCHANGE) == 3, "right arrow stops at the last segment without notifying");
    Key(seg.GetSafeHwnd(), VK_HOME);
    Check(seg.GetSelected() == 0, "Home selects the first segment");
    {
        auto provider = Provider(seg);
        Check(Pattern<ISelectionProvider>(provider.Get(), UIA_SelectionPatternId) != nullptr, "segmented button exposes the Selection pattern");
    }

    // ----------------------------------------------------------- NumberStepper
    NumberStepper stepper;
    Check(stepper.Create(L"Interval", CRect(10, 540, 250, 624), &host, 140) != FALSE, "number stepper created");
    stepper.SetRange(0, 10);
    stepper.SetStep(2);
    stepper.SetValue(4);
    host.log.clear();
    const HWND edit = stepper.EditWindow();
    Check(edit != nullptr && TextOf(edit) == L"4", "stepper shows its value in the inner edit");
    {
        CRect client;
        stepper.GetClientRect(client);
        const auto layout = LayoutStepper(Dip(stepper, client.Width()), Dip(stepper, client.Height()));
        Click(stepper.GetSafeHwnd(), Px(stepper, (layout.plus.left + layout.plus.right) / 2), Px(stepper, (layout.plus.top + layout.plus.bottom) / 2));
        Check(stepper.GetValue() == 6 && host.Count(140, EN_CHANGE) == 1, "plus button adds one step and notifies");
        Click(stepper.GetSafeHwnd(), Px(stepper, (layout.minus.left + layout.minus.right) / 2), Px(stepper, (layout.minus.top + layout.minus.bottom) / 2));
        Check(stepper.GetValue() == 4 && host.Count(140, EN_CHANGE) == 2, "minus button subtracts one step and notifies");
    }
    Check(stepper.StepBy(10) && stepper.GetValue() == 10 && host.Count(140, EN_CHANGE) == 3, "stepping clamps at the maximum");
    Check(!stepper.StepBy(1) && stepper.GetValue() == 10 && host.Count(140, EN_CHANGE) == 3, "stepping past the maximum changes and notifies nothing");
    Key(edit, VK_DOWN);
    Check(stepper.GetValue() == 8 && TextOf(edit) == L"8", "Down key subtracts a step and updates the text");
    Key(edit, VK_NEXT);
    Check(stepper.GetValue() == 0, "PageDown subtracts ten steps and clamps at the minimum");
    Key(edit, VK_PRIOR);
    Check(stepper.GetValue() == 10, "PageUp adds ten steps and clamps at the maximum");
    Key(edit, VK_UP);
    Check(stepper.GetValue() == 10, "Up key at the maximum keeps the value");
    host.log.clear();
    SendMessage(edit, EM_SETSEL, 0, -1);
    Chars(edit, L"7");
    Check(stepper.GetValidationMessage().IsEmpty() && stepper.GetValue() == 10 && host.Total() == 0, "typing a valid number validates without committing");
    Key(edit, VK_RETURN);
    Check(stepper.GetValue() == 7 && host.Count(140, EN_CHANGE) == 1, "Enter commits the typed number and notifies");
    SendMessage(edit, EM_SETSEL, 0, -1);
    Chars(edit, L"500");
    Check(stepper.GetValidationMessage().Find(L"0 to 10") >= 0, "out-of-range typed input shows a range message");
    Key(edit, VK_RETURN);
    Check(stepper.GetValue() == 7 && host.Count(140, EN_CHANGE) == 1 && !stepper.GetValidationMessage().IsEmpty(), "Enter on an out-of-range number keeps the value and the message");
    Key(edit, VK_ESCAPE);
    Check(TextOf(edit) == L"7" && stepper.GetValidationMessage().IsEmpty(), "Escape reverts the typed text and clears the message");
    SendMessage(edit, EM_SETSEL, 0, -1);
    Chars(edit, L"x");
    Check(stepper.GetValidationMessage() == L"Enter a whole number.", "non-numeric input shows the whole-number message");
    SendMessage(edit, EM_SETSEL, 0, -1);
    Chars(edit, L"9");
    SendMessage(edit, WM_KILLFOCUS, 0, 0);
    Check(stepper.GetValue() == 9 && host.Count(140, EN_CHANGE) == 2, "leaving the field commits a valid typed number");
    SendMessage(edit, EM_SETSEL, 0, -1);
    Chars(edit, L"99");
    SendMessage(edit, WM_KILLFOCUS, 0, 0);
    Check(stepper.GetValue() == 9 && TextOf(edit) == L"9" && host.Count(140, EN_CHANGE) == 2, "leaving the field discards an out-of-range number");
    stepper.SetValue(-50);
    Check(stepper.GetValue() == 0 && host.Count(140, EN_CHANGE) == 2, "programmatic value clamps into range silently");
    stepper.SetRange(10, 0);
    Check(stepper.GetMinimum() == 0 && stepper.GetMaximum() == 10, "a reversed range is normalised");
    {
        long long v = 0;
        Check(NumberStepper::ParseInteger(L"12", v) && v == 12 && NumberStepper::ParseInteger(L" -5 ", v) && v == -5 && NumberStepper::ParseInteger(L"+3", v) && v == 3, "integer parser accepts signs and padding");
        Check(!NumberStepper::ParseInteger(L"", v) && !NumberStepper::ParseInteger(L"-", v) && !NumberStepper::ParseInteger(L"1a", v) && !NumberStepper::ParseInteger(L"1.5", v)
            && !NumberStepper::ParseInteger(L"99999999999999", v), "integer parser rejects empty, partial, fractional and overflowing text");
    }
    {
        auto provider = Provider(stepper);
        auto range = Pattern<IRangeValueProvider>(provider.Get(), UIA_RangeValuePatternId);
        double value = -1, minimum = -1, maximum = -1;
        Check(range && SUCCEEDED(range->get_Value(&value)) && SUCCEEDED(range->get_Minimum(&minimum)) && SUCCEEDED(range->get_Maximum(&maximum))
            && value == 0 && minimum == 0 && maximum == 10 && ControlType(provider.Get()) == UIA_SpinnerControlTypeId, "stepper exposes RangeValue and a spinner role");
        host.log.clear();
        Check(range && SUCCEEDED(range->SetValue(6.0)), "RangeValue accepts an in-range value");
        Pump();
        Check(stepper.GetValue() == 6 && host.Count(140, EN_CHANGE) == 1, "automation SetValue changes the stepper and notifies");
        Check(range && range->SetValue(500.0) == E_INVALIDARG && range->SetValue(2.5) == E_INVALIDARG, "RangeValue rejects out-of-range and fractional values");
    }

    // --------------------------------------------------------------- Dropdown
    {
        const std::vector<CString> items{ L"Alpha", L"Beta", L"Gamma", L"Delta", L"Epsilon" };
        CString err;
        Check(FilterDropdownItems(items, L"AL", false, &err) == std::vector<int>{ 0 }, "plain filter is case insensitive and narrows the list");
        Check(FilterDropdownItems(items, L"", false, &err).size() == 5, "empty filter keeps every item");
        Check(FilterDropdownItems(items, L"^(al|ga)", true, &err) == std::vector<int>({ 0, 2 }) && err.IsEmpty(), "regex filter matches by pattern");
        Check(FilterDropdownItems(items, L"(", true, &err).empty() && !err.IsEmpty(), "invalid regex yields no rows and a message");
        Check(FilterDropdownItems(items, CString(L'a', 300), true, &err).empty() && !err.IsEmpty(), "over-long regex is refused before it runs");
        Check(FilterDropdownItems(items, L"(", false, &err).empty() && err.IsEmpty(), "plain filter treats parentheses literally");
    }
    Dropdown dd;
    Check(dd.Create(L"Fruit", CRect(10, 640, 290, 724), &host, 150) != FALSE, "dropdown created");
    dd.SetPrompt(L"Choose a fruit");
    for (const wchar_t* item : { L"Alpha", L"Beta", L"Gamma", L"Delta", L"Epsilon" }) dd.AddItem(item);
    dd.SetCurSel(1);
    host.log.clear();
    Check(dd.GetCurSel() == 1 && dd.GetCurText() == L"Beta" && host.Total() == 0, "programmatic dropdown selection is silent");
    Key(dd.GetSafeHwnd(), VK_DOWN);
    Check(dd.GetCurSel() == 2 && host.Count(150, CBN_SELCHANGE) == 1, "closed dropdown Down arrow selects the next item and notifies");
    Key(dd.GetSafeHwnd(), VK_UP);
    Key(dd.GetSafeHwnd(), VK_END);
    Check(dd.GetCurSel() == 4 && host.Count(150, CBN_SELCHANGE) == 3, "closed dropdown Up and End change the selection");
    Key(dd.GetSafeHwnd(), VK_HOME);
    Check(dd.GetCurSel() == 0, "closed dropdown Home selects the first item");
    Chars(dd.GetSafeHwnd(), L"g");
    Check(dd.GetCurSel() == 2, "typeahead selects the first item starting with the typed letter");
    Sleep(1100);
    Chars(dd.GetSafeHwnd(), L"e");
    Check(dd.GetCurSel() == 4, "typeahead restarts after a pause");
    host.log.clear();
    Check(!dd.IsOpen() && dd.PopupWindow() == nullptr, "dropdown starts closed with no popup window");
    Key(dd.GetSafeHwnd(), VK_RETURN);
    Check(dd.IsOpen() && dd.PopupWindow() != nullptr && dd.PopupEditWindow() != nullptr, "Enter opens the self-drawn popup with its search field");
    Check(dd.PopupVisibleItems().size() == 5 && dd.PopupHotItem() == 4, "opened popup lists every item with the current one highlighted");
    Check(!HasStockChild(dd.PopupWindow()) && !IsStockClass(dd.PopupWindow()), "popup uses no stock control class");
    Chars(dd.PopupEditWindow(), L"a");
    Check(dd.PopupVisibleItems() == std::vector<int>({ 0, 1, 2, 3 }), "typing in the popup search field filters the list");
    Chars(dd.PopupEditWindow(), L"l");
    Check(dd.PopupVisibleItems() == std::vector<int>({ 0 }) && dd.PopupQuery() == L"al", "continued typing narrows the filter further");
    Key(dd.PopupEditWindow(), VK_RETURN);
    Check(!dd.IsOpen() && dd.GetCurSel() == 0 && host.Count(150, CBN_SELCHANGE) == 1 && dd.PopupWindow() == nullptr, "Enter in the popup picks the item, closes it and notifies");
    dd.SetCurSel(0);
    Key(dd.GetSafeHwnd(), VK_RETURN);
    Key(dd.PopupEditWindow(), VK_DOWN);
    Key(dd.PopupEditWindow(), VK_DOWN);
    Key(dd.PopupEditWindow(), VK_UP);
    Check(dd.PopupHotItem() == 1, "popup Up and Down move the highlighted item");
    Key(dd.PopupEditWindow(), VK_NEXT);
    Check(dd.PopupHotItem() == 4, "popup PageDown moves toward the end of the list");
    Key(dd.PopupEditWindow(), VK_RETURN);
    Check(dd.GetCurSel() == 4 && !dd.IsOpen(), "the highlighted item is chosen with Enter");
    dd.SetCurSel(2);
    host.log.clear();
    Key(dd.GetSafeHwnd(), VK_RETURN);
    Chars(dd.PopupEditWindow(), L"z");
    Check(dd.PopupVisibleItems().empty(), "a search with no match shows no rows");
    Key(dd.PopupEditWindow(), VK_ESCAPE);
    Check(dd.IsOpen() && dd.PopupVisibleItems().size() == 5 && dd.PopupQuery().IsEmpty(), "first Escape clears the search and keeps the popup open");
    Key(dd.PopupEditWindow(), VK_ESCAPE);
    Check(!dd.IsOpen() && dd.GetCurSel() == 2 && host.Total() == 0, "second Escape closes the popup without choosing");
    Key(dd.GetSafeHwnd(), VK_RETURN);
    {
        CRect popupClient;
        ::GetClientRect(dd.PopupWindow(), popupClient);
        const float widthDip = popupClient.Width() * 96.f / GetDpiForWindow(dd.PopupWindow());
        const auto layout = LayoutPopup(widthDip, 5, false);
        const float dpiScale = GetDpiForWindow(dd.PopupWindow()) / 96.f;
        Click(dd.PopupWindow(), static_cast<int>((layout.regex.left + layout.regex.right) / 2 * dpiScale), static_cast<int>((layout.regex.top + layout.regex.bottom) / 2 * dpiScale));
        Check(dd.PopupRegexMode(), "Regex chip in the popup switches the search to regular expressions");
        SendMessage(dd.PopupEditWindow(), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"^(Al|Ga)"));
        Check(dd.PopupVisibleItems() == std::vector<int>({ 0, 2 }), "popup regex search filters by pattern");
        SendMessage(dd.PopupEditWindow(), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"("));
        Check(dd.PopupVisibleItems().empty() && !dd.PopupError().IsEmpty(), "invalid popup regex reports a message instead of failing");
        SendMessage(dd.PopupEditWindow(), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
        Click(dd.PopupWindow(), static_cast<int>((layout.regex.left + layout.regex.right) / 2 * dpiScale), static_cast<int>((layout.regex.top + layout.regex.bottom) / 2 * dpiScale));
        Check(!dd.PopupRegexMode() && dd.PopupVisibleItems().size() == 5, "Regex chip toggles back to plain search");
        const auto rows = LayoutPopup(widthDip, 5, false);
        host.log.clear();
        Click(dd.PopupWindow(), static_cast<int>(40 * dpiScale), static_cast<int>((rows.list.top + 3 * InputMetrics::RowHeight + 24) * dpiScale));
        Check(!dd.IsOpen() && dd.GetCurSel() == 3 && host.Count(150, CBN_SELCHANGE) == 1, "clicking a popup row chooses that item and notifies");
    }
    {
        auto provider = Provider(dd);
        Check(Name(provider.Get()) == L"Fruit" && ControlType(provider.Get()) == UIA_ComboBoxControlTypeId, "dropdown exposes its name and a combo box role");
        auto expand = Pattern<IExpandCollapseProvider>(provider.Get(), UIA_ExpandCollapsePatternId);
        auto value = Pattern<IValueProvider>(provider.Get(), UIA_ValuePatternId);
        auto selection = Pattern<ISelectionProvider>(provider.Get(), UIA_SelectionPatternId);
        ExpandCollapseState state{};
        BSTR text = nullptr;
        Check(expand && value && selection && SUCCEEDED(expand->get_ExpandCollapseState(&state)) && state == ExpandCollapseState_Collapsed
            && SUCCEEDED(value->get_Value(&text)) && text && CString(text) == L"Delta", "dropdown exposes ExpandCollapse, Value and Selection");
        SysFreeString(text);
        if (expand) expand->Expand();
        Pump();
        expand->get_ExpandCollapseState(&state);
        Check(dd.IsOpen() && state == ExpandCollapseState_Expanded, "automation Expand opens the dropdown");
        if (expand) expand->Collapse();
        Pump();
        expand->get_ExpandCollapseState(&state);
        Check(!dd.IsOpen() && state == ExpandCollapseState_Collapsed, "automation Collapse closes the dropdown");
        Check(value && value->SetValue(L"Beta") == S_OK, "Value pattern accepts a known item");
        Pump();
        Check(dd.GetCurSel() == 1, "automation Value selects the matching item");
    }
    {
        dd.Open();
        Check(dd.IsOpen(), "dropdown reopens after a close");
        dd.Close(false);
        Check(!dd.IsOpen() && dd.PopupWindow() == nullptr, "Close destroys the popup window");
    }

    // -------------------------------------------------------- LabeledTextField
    LabeledTextField field;
    Check(field.Create(L"Name", CRect(300, 470, 580, 590), &host, 160) != FALSE, "labeled text field created");
    Check(field.Field().IsKindOf(RUNTIME_CLASS(TextField)), "labeled text field hosts the existing NativeMaterial::TextField");
    host.log.clear();
    field.SetText(L"hello");
    Check(field.GetText() == L"hello" && field.ClearVisible() && host.Total() == 0, "programmatic text is silent and shows the clear button");
    SendMessage(field.Field().GetSafeHwnd(), EM_SETSEL, 5, 5);
    Chars(field.Field().GetSafeHwnd(), L"!");
    Check(field.GetText() == L"hello!" && host.Count(160, EN_CHANGE) == 1, "typing reaches the parent as a change notification");
    Click(field.ClearControl().GetSafeHwnd(), 20, 20);
    Check(field.GetText().IsEmpty() && host.Count(160, EN_CHANGE) == 2 && !field.ClearVisible(), "clear button empties the field through the change path and hides itself");
    field.SetText(L"x");
    Key(field.ClearControl().GetSafeHwnd(), VK_RETURN);
    Check(field.GetText().IsEmpty() && host.Count(160, EN_CHANGE) == 3, "clear button is keyboard operable");
    field.Clear();
    Check(host.Count(160, EN_CHANGE) == 3, "clearing an empty field changes nothing");
    field.SetSupportingText(L"Shown to everyone");
    field.SetErrorText(L"Required");
    Check(field.GetErrorText() == L"Required", "error text is stored and replaces supporting text");
    field.SetErrorText(L"");
    Check(field.GetErrorText().IsEmpty(), "empty error text clears the error");
    {
        IAccessible* accessible = nullptr;
        bool named = false;
        if (SUCCEEDED(AccessibleObjectFromWindow(field.Field().GetSafeHwnd(), OBJID_CLIENT, IID_IAccessible, reinterpret_cast<void**>(&accessible))) && accessible)
        {
            VARIANT self; self.vt = VT_I4; self.lVal = CHILDID_SELF;
            BSTR name = nullptr;
            named = SUCCEEDED(accessible->get_accName(self, &name)) && name && CString(name) == L"Name";
            SysFreeString(name);
            accessible->Release();
        }
        Check(named, "the inner edit carries the visible label as its accessible name");
    }

    // ------------------------------------------------------ stock control scan
    {
        CString found;
        Check(!HasStockChild(host.GetSafeHwnd(), &found), "no component creates a stock Button, ComboBox, ComboLBox, up-down or trackbar child");
        const HWND windows[] = { sw, cb, r1, r2, r3, r4, r5, group, seg, stepper, dd, field, field.ClearControl() };
        bool stock = false;
        for (const HWND w : windows) stock = stock || IsStockClass(w);
        Check(!stock, "no component window itself is a stock control class");
    }

    // ------------------------------------------------------- minimum targets
    {
        const UINT dpi = GetDpiForWindow(host.GetSafeHwnd());
        auto tall = [&](InputWindow& w) { return w.MinimumSize().cy * 96.0 / dpi >= 40 && w.MinimumSize().cx * 96.0 / dpi >= 40; };
        Check(tall(sw) && tall(cb) && tall(r1) && tall(group) && tall(seg) && tall(stepper) && tall(dd) && tall(field) && tall(field.ClearControl()), "every component reports a minimum target of at least 40 logical pixels");
    }

    // ------------------------------------------------------------ real window paint
    for (auto appearance : { Appearance::Light, Appearance::Dark })
    {
        InputWindow* all[] = { &sw, &cb, &r1, &group, &seg, &stepper, &dd, &field, &field.ClearControl() };
        for (auto* component : all) component->SetAppearance(appearance);
        bool allInk = true;
        for (auto* component : all) allInk = allInk && WindowInk(*component) > 40;
        Check(allInk, appearance == Appearance::Light ? "every real component paints visible light output through WM_PRINTCLIENT" : "every real component paints visible dark output through WM_PRINTCLIENT");
    }

    // ---------------------------------------------------- offscreen rendering
    for (UINT dpi : { 96u, 144u })
    for (auto appearance : { Appearance::Light, Appearance::Dark })
    {
        const char* tag = appearance == Appearance::Light ? "light" : "dark";
        char label[160];
        auto named = [&](const char* what) { std::snprintf(label, sizeof(label), "%s renders non-blank at %u dpi %s", what, dpi, tag); return label; };
        InputState on; InputState focus; focus.focused = true; InputState off; off.enabled = false;
        const std::vector<CString> labels{ L"Day", L"Week", L"Month" };
        const auto switchOn = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawSwitch(t, p, L"Dark mode", true, false, on); });
        const auto switchOff = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawSwitch(t, p, L"Dark mode", false, false, on); });
        const auto switchFocus = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawSwitch(t, p, L"Dark mode", true, false, focus); });
        const auto switchDisabled = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawSwitch(t, p, L"Dark mode", true, false, off); });
        const auto switchIcon = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawSwitch(t, p, L"Dark mode", true, true, on); });
        Check(Ink(switchOn) > 300 && Ink(switchOff) > 200, named("switch"));
        Check(Difference(switchOn, switchOff) > 100 && Difference(switchOn, switchFocus) > 20 && Difference(switchOn, switchDisabled) > 100 && Difference(switchOn, switchIcon) > 10,
            "switch on, off, focus, disabled and icon states are visibly different");
        const auto boxOff = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawCheckbox(t, p, L"Accept", CheckState::Unchecked, on); });
        const auto boxOn = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawCheckbox(t, p, L"Accept", CheckState::Checked, on); });
        const auto boxMixed = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawCheckbox(t, p, L"Accept", CheckState::Indeterminate, on); });
        InputState bad; bad.error = true;
        const auto boxError = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawCheckbox(t, p, L"Accept", CheckState::Unchecked, bad); });
        Check(Ink(boxOff) > 100 && Ink(boxOn) > 200, named("checkbox"));
        Check(Difference(boxOff, boxOn) > 50 && Difference(boxOn, boxMixed) > 5 && Difference(boxOff, boxError) > 20, "checkbox unchecked, checked, indeterminate and error states are visibly different");
        const auto radioOff = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawRadio(t, p, L"Medium", false, on); });
        const auto radioOn = Render(dpi, 240, 48, appearance, [&](auto* t, const auto& p) { DrawRadio(t, p, L"Medium", true, on); });
        Check(Ink(radioOff) > 100 && Difference(radioOff, radioOn) > 20, named("radio"));
        const auto groupPicture = Render(dpi, 240, 150, appearance, [&](auto* t, const auto& p) { DrawRadioGroup(t, p, labels, 1, 1, -1, on); });
        Check(Ink(groupPicture) > 300, named("radio group"));
        const auto seg0 = Render(dpi, 320, 48, appearance, [&](auto* t, const auto& p) { DrawSegments(t, p, labels, 0, -1, on); });
        const auto seg1 = Render(dpi, 320, 48, appearance, [&](auto* t, const auto& p) { DrawSegments(t, p, labels, 1, -1, on); });
        Check(Ink(seg0) > 500 && Difference(seg0, seg1) > 200, named("segmented button"));
        const auto stepperPlain = Render(dpi, 240, 84, appearance, [&](auto* t, const auto& p) { DrawStepper(t, p, L"Interval", L"", true, true, StepperPart::None, StepperPart::None, on); });
        const auto stepperError = Render(dpi, 240, 84, appearance, [&](auto* t, const auto& p) { DrawStepper(t, p, L"Interval", L"Enter a value from 0 to 10.", true, true, StepperPart::None, StepperPart::None, bad); });
        const auto stepperHover = Render(dpi, 240, 84, appearance, [&](auto* t, const auto& p) { DrawStepper(t, p, L"Interval", L"", true, true, StepperPart::Plus, StepperPart::None, on); });
        Check(Ink(stepperPlain) > 500 && Difference(stepperPlain, stepperError) > 100 && Difference(stepperPlain, stepperHover) > 50, named("number stepper"));
        const auto fieldClosed = Render(dpi, 280, 84, appearance, [&](auto* t, const auto& p) { DrawDropdownField(t, p, L"Fruit", L"Alpha", L"", false, false, on); });
        const auto fieldOpen = Render(dpi, 280, 84, appearance, [&](auto* t, const auto& p) { DrawDropdownField(t, p, L"Fruit", L"Alpha", L"", false, true, on); });
        Check(Ink(fieldClosed) > 500 && Difference(fieldClosed, fieldOpen) > 50, named("dropdown field"));
        PopupModel model;
        model.rows = { L"Alpha", L"Beta", L"Gamma" };
        model.selectedRow = 1; model.regexLabel = L"Regex"; model.emptyText = L"No matches";
        const auto popupPlain = Render(dpi, 280, 260, appearance, [&](auto* t, const auto& p) { DrawDropdownPopup(t, p, model); });
        model.hotRow = 2;
        const auto popupHot = Render(dpi, 280, 260, appearance, [&](auto* t, const auto& p) { DrawDropdownPopup(t, p, model); });
        Check(Ink(popupPlain) > 2000 && Difference(popupPlain, popupHot) > 100, named("dropdown popup"));
        const auto chrome = Render(dpi, 280, 120, appearance, [&](auto* t, const auto& p) { DrawLabeledFieldChrome(t, p, L"Name", L"Shown to everyone", on); });
        const auto clear = Render(dpi, 40, 40, appearance, [&](auto* t, const auto& p) { DrawClearButton(t, p, false, on); });
        Check(Ink(chrome) > 100 && Ink(clear) > 40, named("labeled field chrome and clear button"));
    }

    host.DestroyWindow();
    std::printf("RESULT failures=%d\n", failures);
    return failures ? 1 : 0;
}
