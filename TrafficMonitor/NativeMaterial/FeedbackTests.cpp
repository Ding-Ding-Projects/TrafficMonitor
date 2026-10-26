// Hidden-window verification of the Material 3 feedback components: snackbar, progress indicators and chips.
// Each component is created in a real, never-shown parent and driven with window messages. Nothing here needs a
// visible desktop. Time comes from a test clock, so no check waits for a real timeout.
#include "../stdafx.h"
#include "Snackbar.h"
#include "Progress.h"
#include "Chip.h"
#include "../NativePreferences/Text.h"
#include "../NativeMenus/SearchableMenu.h"
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <wrl/client.h>

// The focused test links only the feedback sources, so the two application services they reach are stubbed.
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
    void ClickAt(HWND window, CPoint p)
    {
        SendMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, Point(p.x, p.y));
        SendMessage(window, WM_LBUTTONUP, 0, Point(p.x, p.y));
    }
    void Key(HWND window, UINT key) { SendMessage(window, WM_KEYDOWN, key, 0); SendMessage(window, WM_KEYUP, key, 0); }
    int Px(CWnd& window, float dip) { return static_cast<int>(std::lround(dip * GetDpiForWindow(window.GetSafeHwnd()) / 96.0)); }
    bool Visible(CWnd& window) { return (window.GetStyle() & WS_VISIBLE) != 0; }
    CRect InParent(CWnd& window, CWnd& parent) { CRect r; window.GetWindowRect(r); parent.ScreenToClient(r); return r; }

    bool IsStockClass(HWND window)
    {
        wchar_t name[128]{};
        GetClassNameW(window, name, 128);
        for (const wchar_t* stock : { L"Button", L"ComboBox", L"ComboLBox", L"msctls_updown32", L"msctls_trackbar32", L"msctls_progress32", L"Static", L"Edit" })
            if (_wcsicmp(name, stock) == 0) return true;
        return false;
    }
    bool HasStockChild(HWND root)
    {
        bool bad = false;
        EnumChildWindows(root, [](HWND child, LPARAM l) -> BOOL { if (IsStockClass(child)) *reinterpret_cast<bool*>(l) = true; return TRUE; }, reinterpret_cast<LPARAM>(&bad));
        return bad;
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
    // Paints a real window through its WM_PRINTCLIENT route into a DIB and counts the pixels that differ from the surface.
    size_t WindowInk(InputWindow& window)
    {
        CRect client;
        window.GetClientRect(client);
        if (client.Width() <= 0 || client.Height() <= 0) return 0;
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
        if (provider && SUCCEEDED(provider->GetPatternProvider(id, unknown.GetAddressOf())) && unknown) unknown.As(&pattern);
        return pattern;
    }
    CString TextProperty(IRawElementProviderSimple* provider, PROPERTYID id)
    {
        VARIANT v{};
        CString text;
        if (SUCCEEDED(provider->GetPropertyValue(id, &v)) && v.vt == VT_BSTR) text = v.bstrVal;
        VariantClear(&v);
        return text;
    }
    long NumberProperty(IRawElementProviderSimple* provider, PROPERTYID id)
    {
        VARIANT v{};
        long number = -1;
        if (SUCCEEDED(provider->GetPropertyValue(id, &v)) && v.vt == VT_I4) number = v.lVal;
        return number;
    }
    int BoolProperty(IRawElementProviderSimple* provider, PROPERTYID id)
    {
        VARIANT v{};
        int result = -1;
        if (SUCCEEDED(provider->GetPropertyValue(id, &v)) && v.vt == VT_BOOL) result = v.boolVal != VARIANT_FALSE ? 1 : 0;
        return result;
    }
    ComPtr<IRawElementProviderSimple> Provider(InputWindow& window)
    {
        ComPtr<IRawElementProviderSimple> provider;
        window.AutomationProvider(provider.GetAddressOf());
        return provider;
    }

    ULONGLONG g_now = 1000000;
}

int wmain()
{
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Host host;
    Check(host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Material feedback verification", WS_POPUP, 0, 0, 900, 700, nullptr, 0) != FALSE, "hidden native parent created");

    // ========================================================================= Snackbar
    Snackbar::SetClock([] { return g_now; });
    std::vector<std::pair<CString, bool>> announced;
    Snackbar::SetNotificationObserver([&](const CString& text, bool important) { announced.emplace_back(text, important); });

    Check(Snackbar::EffectiveTimeout(SnackbarMessage{ L"x" }) == 5000, "a message without an action stays 5 seconds");
    Check(Snackbar::EffectiveTimeout(SnackbarMessage{ L"x", L"Undo" }) == 10000, "a message with an action stays 10 seconds");
    {
        SnackbarMessage tooShort{ L"x" }, tooLong{ L"x" }, inside{ L"x" }, persistent{ L"x" };
        tooShort.timeoutMs = 1000; tooLong.timeoutMs = 99999; inside.timeoutMs = 6500; persistent.persistent = true; persistent.timeoutMs = 4500;
        Check(Snackbar::EffectiveTimeout(tooShort) == 4000 && Snackbar::EffectiveTimeout(tooLong) == 10000 && Snackbar::EffectiveTimeout(inside) == 6500,
            "explicit snackbar timeouts are clamped to 4 to 10 seconds");
        Check(Snackbar::EffectiveTimeout(persistent) == 0, "a persistent snackbar has no timeout");
    }

    Snackbar bar;
    Check(bar.Create(&host, 301) != FALSE, "snackbar created");
    Check(!Visible(bar) && !bar.IsShowing(), "snackbar is hidden until a message is shown");
    Check(bar.Show(SnackbarMessage{}) == 0, "an empty message is refused");
    host.log.clear();

    const HWND focusBefore = ::GetFocus();
    const HWND foregroundBefore = ::GetForegroundWindow();
    SnackbarMessage plain; plain.text = L"Settings saved";
    std::vector<std::pair<unsigned, SnackbarResult>> results;
    plain.onResult = [&](unsigned id, SnackbarResult r) { results.emplace_back(id, r); };
    SnackbarMessage withAction; withAction.text = L"Collection deleted"; withAction.action = L"Undo"; withAction.closable = true;
    withAction.onResult = plain.onResult;
    SnackbarMessage persistent; persistent.text = L"Update failed"; persistent.persistent = true; persistent.closable = true;
    persistent.onResult = plain.onResult;

    const unsigned idA = bar.Show(plain);
    Check(idA != 0 && bar.IsShowing() && bar.CurrentId() == idA && bar.CurrentText() == L"Settings saved" && Visible(bar), "the first message is shown at once");
    Check(::GetFocus() == focusBefore && ::GetForegroundWindow() == foregroundBefore && !bar.HasKeyboardFocus(), "showing a snackbar does not move focus or activate a window");
    Check(SendMessage(bar.GetSafeHwnd(), WM_MOUSEACTIVATE, 0, 0) == MA_NOACTIVATE, "clicking the snackbar never activates a window");
    Check(announced.size() == 1 && announced[0].first == L"Settings saved" && !announced[0].second && SUCCEEDED(bar.LastNotificationResult()),
        "showing a message raises one polite UI Automation notification");
    const unsigned idB = bar.Show(withAction);
    const unsigned idC = bar.Show(persistent);
    Check(idB != 0 && idC != 0 && idB != idA && idC != idB && bar.QueuedCount() == 2 && bar.CurrentId() == idA && announced.size() == 1,
        "later messages wait in a queue and are not announced yet");

    // Layout: bottom-left of the host, 672 wide on a 900 wide host, 48 high for one line.
    {
        const CRect r = InParent(bar, host);
        CRect hostClient; host.GetClientRect(hostClient);
        Check(r.left == Px(host, 16) && r.bottom == hostClient.bottom - Px(host, 16) && r.Height() == Px(host, 48) && r.Width() == Px(host, 672) && !bar.TwoLines(),
            "a one-line snackbar sits 16 DIP from the bottom-left corner of the host");
    }

    // Timeout through the test clock.
    g_now += 4999; bar.Tick();
    Check(bar.CurrentId() == idA && results.empty(), "a message is still shown just before its timeout");
    g_now += 1; bar.Tick();
    Check(bar.CurrentId() == idB && bar.QueuedCount() == 1 && bar.LastResult() == SnackbarResult::Timeout && bar.LastResultId() == idA
        && results.size() == 1 && results[0].first == idA && results[0].second == SnackbarResult::Timeout, "a message goes away at its timeout and the next queued one appears");
    Check(host.Count(301, SnackbarNotifyDismissed) == 1 && host.Count(301, SnackbarNotifyAction) == 0, "the parent is told once about the dismissal");
    Check(announced.size() == 2 && announced[1].first == L"Collection deleted. Undo", "the next message is announced when it appears, with its action");

    // The countdown is held while the pointer is over the snackbar and restarts afterwards.
    g_now += 9000; bar.Tick();
    SendMessage(bar.GetSafeHwnd(), WM_MOUSEMOVE, 0, Point(Px(bar, 100), Px(bar, 24)));
    g_now += 2000; bar.Tick();
    Check(bar.CurrentId() == idB, "the pointer over the snackbar holds the countdown");
    g_now += 60000; bar.Tick();
    Check(bar.CurrentId() == idB, "a held message does not time out however long it is held");
    SendMessage(bar.GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);
    bar.Tick();
    g_now += 9999; bar.Tick();
    Check(bar.CurrentId() == idB, "the full countdown restarts after the pointer leaves");

    // Keyboard focus also holds the countdown; focus parts move with the arrow keys.
    SendMessage(bar.GetSafeHwnd(), WM_SETFOCUS, 0, 0);
    Check(bar.HasKeyboardFocus() && bar.FocusedPart() == SnackbarPart::Action, "focus on a snackbar with an action starts on the action");
    g_now += 30000; bar.Tick();
    Check(bar.CurrentId() == idB, "keyboard focus inside the snackbar holds the countdown");
    Key(bar.GetSafeHwnd(), VK_RIGHT);
    Check(bar.FocusedPart() == SnackbarPart::Close, "Right moves focus from the action to the close icon");
    Key(bar.GetSafeHwnd(), VK_RIGHT);
    Check(bar.FocusedPart() == SnackbarPart::Action, "Right wraps back to the action");
    Key(bar.GetSafeHwnd(), VK_LEFT);
    Check(bar.FocusedPart() == SnackbarPart::Close, "Left moves back to the close icon");
    {
        MSG f6{}; f6.message = WM_KEYDOWN; f6.wParam = VK_F6;
        MSG other{}; other.message = WM_KEYDOWN; other.wParam = VK_F5;
        Check(bar.TranslateShortcut(f6) && !bar.TranslateShortcut(other), "F6 is the focus shortcut and no other key is taken");
        Check(!bar.HasKeyboardFocus(), "F6 inside the snackbar leaves it");
    }
    SendMessage(bar.GetSafeHwnd(), WM_KILLFOCUS, 0, 0);
    bar.Tick();   // restarts the countdown after the hold

    // Chooses the action with the mouse: notified before the dismissal, next message appears.
    const CRect actionRect = bar.PartRect(SnackbarPart::Action);
    Check(!actionRect.IsRectEmpty() && !bar.PartRect(SnackbarPart::Close).IsRectEmpty(), "an action and a close icon both have a target");
    Check(actionRect.Width() >= Px(bar, 48) && bar.PartRect(SnackbarPart::Close).Width() >= Px(bar, 48), "snackbar targets are at least 48 DIP wide");
    host.log.clear();
    const HWND focusAtClick = ::GetFocus();
    ClickAt(bar.GetSafeHwnd(), actionRect.CenterPoint());
    Check(host.Count(301, SnackbarNotifyAction) == 1 && host.Count(301, SnackbarNotifyDismissed) == 1 && bar.LastResult() == SnackbarResult::Action && bar.LastResultId() == idB
        && results.back().first == idB && results.back().second == SnackbarResult::Action, "choosing the action notifies the parent and the callback once");
    Check(host.log.size() == 2 && host.log[0].second == SnackbarNotifyAction && host.log[1].second == SnackbarNotifyDismissed, "the action notification arrives before the dismissal");
    Check(bar.CurrentId() == idC && ::GetFocus() == focusAtClick && !bar.HasKeyboardFocus(), "choosing the action moves nothing but the queue forward");

    // A persistent message never times out and is announced as important.
    Check(announced.size() == 3 && announced[2].second, "a persistent message is announced as important");
    g_now += 3600000; bar.Tick();
    Check(bar.CurrentId() == idC, "a persistent message stays until dismissed");
    {
        auto provider = Provider(bar);
        Check(TextProperty(provider.Get(), UIA_NamePropertyId) == L"Update failed" && NumberProperty(provider.Get(), UIA_ControlTypePropertyId) == UIA_GroupControlTypeId
            && TextProperty(provider.Get(), UIA_LocalizedControlTypePropertyId) == L"snackbar", "the snackbar exposes its message as the name of a group");
        Check(NumberProperty(provider.Get(), UIA_LiveSettingPropertyId) == Polite, "the snackbar is a polite live region");
        Check(TextProperty(provider.Get(), UIA_AcceleratorKeyPropertyId) == L"F6", "the snackbar publishes its F6 shortcut");
        Check(!Pattern<IToggleProvider>(provider.Get(), UIA_TogglePatternId) && !Pattern<IRangeValueProvider>(provider.Get(), UIA_RangeValuePatternId), "a snackbar offers no Toggle or RangeValue pattern");
        auto invoke = Pattern<IInvokeProvider>(provider.Get(), UIA_InvokePatternId);
        host.log.clear();
        Check(invoke && SUCCEEDED(invoke->Invoke()), "automation Invoke is available");
        Pump();
        Check(bar.LastResult() == SnackbarResult::Close && bar.LastResultId() == idC && !bar.IsShowing() && !Visible(bar), "Invoke on a snackbar without an action dismisses it");
    }

    // Close icon and Escape.
    bar.Show(withAction);
    ClickAt(bar.GetSafeHwnd(), bar.PartRect(SnackbarPart::Close).CenterPoint());
    Check(bar.LastResult() == SnackbarResult::Close && !bar.IsShowing(), "the close icon dismisses the message");
    bar.Show(plain);
    SendMessage(bar.GetSafeHwnd(), WM_SETFOCUS, 0, 0);
    Key(bar.GetSafeHwnd(), VK_ESCAPE);
    Check(bar.LastResult() == SnackbarResult::Close && !bar.IsShowing() && !bar.HasKeyboardFocus(), "Escape dismisses the message and releases focus");
    bar.Show(withAction);
    SendMessage(bar.GetSafeHwnd(), WM_SETFOCUS, 0, 0);
    Key(bar.GetSafeHwnd(), VK_RETURN);
    Check(bar.LastResult() == SnackbarResult::Action && !bar.IsShowing(), "Enter on the focused action chooses it");
    bar.Show(withAction);
    {
        auto provider = Provider(bar);
        auto invoke = Pattern<IInvokeProvider>(provider.Get(), UIA_InvokePatternId);
        if (invoke) invoke->Invoke();
        Pump();
        Check(bar.LastResult() == SnackbarResult::Action, "Invoke on a snackbar with an action runs the action");
    }
    bar.Show(plain);
    ClickAt(bar.GetSafeHwnd(), CPoint(Px(bar, 40), Px(bar, 24)));
    Check(bar.IsShowing() && !bar.HasKeyboardFocus(), "a click on the message text neither dismisses it nor takes focus");
    SendMessage(bar.GetSafeHwnd(), WM_LBUTTONDOWN, MK_LBUTTON, Point(Px(bar, 40), Px(bar, 24)));
    bar.Dismiss();
    Check(!bar.IsShowing() && bar.LastResult() == SnackbarResult::Dismissed, "Dismiss removes the current message");
    SendMessage(bar.GetSafeHwnd(), WM_LBUTTONUP, 0, Point(0, 0));
    Check(!bar.IsShowing(), "a press that ends after the message went away does nothing");

    // Queue limit and Clear.
    results.clear();
    for (int i = 0; i < 17; ++i) { SnackbarMessage m; m.text.Format(L"Message %d", i); m.onResult = plain.onResult; bar.Show(m); }
    Check(bar.QueuedCount() == 16, "sixteen messages wait behind the one that is showing");
    { SnackbarMessage m; m.text = L"Overflow"; m.onResult = plain.onResult; bar.Show(m); }
    Check(bar.QueuedCount() == 16 && results.size() == 1 && results[0].second == SnackbarResult::Dropped, "a full queue drops its oldest message and says so");
    bar.Clear();
    Check(!bar.IsShowing() && bar.QueuedCount() == 0 && results.size() == 18, "Clear removes the message and the whole queue, telling every owner");

    // Two lines and narrow hosts.
    {
        SnackbarMessage longText; longText.text = L"Your changes could not be saved because the file is open in another program and the folder is read-only, so nothing was written to disk.";
        longText.action = L"Retry";
        bar.Show(longText);
        const CRect r = InParent(bar, host);
        Check(bar.TwoLines() && r.Height() == Px(host, 68), "text that does not fit on one line makes a 68 DIP two-line snackbar");
        bar.Dismiss();
    }
    Check(SnackbarWidthFor(900) == 672.f && SnackbarWidthFor(400) == 368.f && SnackbarWidthFor(300) >= 200.f && SnackbarWidthFor(100) == 200.f, "the snackbar width follows the host between 344 and 672");
    bar.Show(plain);
    Check(bar.GetStyle() & WS_VISIBLE, "snackbar is visible while showing");
    bar.Dismiss();

    // ========================================================================= Progress
    Progress::SetReducedMotionProbe([] { return false; });
    Progress linear, circular;
    Check(linear.Create(L"Copying files", ProgressShape::Linear, CRect(10, 100, 410, 140), &host, 501) != FALSE
        && circular.Create(L"Scanning", ProgressShape::Circular, CRect(10, 150, 210, 210), &host, 502) != FALSE, "progress indicators created");
    linear.SetRange(0, 200);
    linear.SetValue(500);
    const bool upper = linear.GetValue() == 200;
    linear.SetValue(-5);
    const bool lower = linear.GetValue() == 0;
    linear.SetValue(50);
    const bool middle = linear.GetValue() == 50 && std::abs(linear.Fraction() - .25f) < .0001f;
    Check(upper && lower && middle, "progress value is clamped into its range");
    linear.SetRange(10, 20);
    Check(linear.GetValue() == 20 && linear.GetMinimum() == 10 && linear.GetMaximum() == 20, "narrowing the range clamps the value again");
    linear.SetRange(5, 5);
    Check(linear.GetMaximum() > linear.GetMinimum() && linear.Fraction() >= 0.f && linear.Fraction() <= 1.f, "an empty range stays valid");
    linear.SetRange(0, 100); linear.SetValue(40);
    {
        auto provider = Provider(linear);
        Check(NumberProperty(provider.Get(), UIA_ControlTypePropertyId) == UIA_ProgressBarControlTypeId && TextProperty(provider.Get(), UIA_NamePropertyId) == L"Copying files", "progress exposes a progress bar role and its name");
        auto range = Pattern<IRangeValueProvider>(provider.Get(), UIA_RangeValuePatternId);
        double value = -1, minimum = -1, maximum = -1; BOOL readOnly = FALSE;
        Check(range && SUCCEEDED(range->get_Value(&value)) && SUCCEEDED(range->get_Minimum(&minimum)) && SUCCEEDED(range->get_Maximum(&maximum)) && value == 40 && minimum == 0 && maximum == 100,
            "determinate progress exposes RangeValue with its value and bounds");
        Check(range && SUCCEEDED(range->get_IsReadOnly(&readOnly)) && readOnly && range->SetValue(75) == UIA_E_INVALIDOPERATION && linear.GetValue() == 40, "progress RangeValue is read only");
        Check(BoolProperty(provider.Get(), UIA_IsKeyboardFocusablePropertyId) == 0 && !(linear.GetStyle() & WS_TABSTOP), "progress is not keyboard focusable");
        linear.SetValue(60);
        range->get_Value(&value);
        Check(value == 60, "RangeValue follows the progress value");
    }
    linear.SetIndeterminate(true);
    linear.SetStatusText(L"Preparing");
    {
        auto provider = Provider(linear);
        Check(!Pattern<IRangeValueProvider>(provider.Get(), UIA_RangeValuePatternId), "indeterminate progress offers no RangeValue pattern");
        Check(TextProperty(provider.Get(), UIA_HelpTextPropertyId) == L"Preparing", "indeterminate progress publishes its status text as help text");
    }
    Check(Progress::RegisteredIndicators() == 1 && Progress::ClockRunning(), "an indeterminate indicator joins the shared animation clock");
    circular.SetIndeterminate(true);
    Check(Progress::RegisteredIndicators() == 2, "several indicators share one clock");
    {
        const float before = linear.Phase();
        const ULONGLONG start = GetTickCount64();
        while (GetTickCount64() - start < 300) { Pump(); Sleep(10); }
        Check(linear.Phase() != before && linear.IsAnimating(), "the shared clock advances an indeterminate indicator");
    }
    linear.SetIndeterminate(false);
    circular.SetIndeterminate(false);
    Check(Progress::RegisteredIndicators() == 0 && !Progress::ClockRunning(), "the clock stops when no indicator is indeterminate");

    // Reduced motion: no motion, the same frame at every phase, and a text.
    Progress::SetReducedMotionProbe([] { return true; });
    linear.SetStatusText(CString());
    linear.SetIndeterminate(true);
    Check(linear.ReducedMotion() && !linear.IsAnimating(), "reduced motion switches the animation off");
    Check(!linear.EffectiveStatusText().IsEmpty(), "reduced motion adds a status text to an indeterminate indicator");
    {
        linear.Advance(1234);
        const float phase = linear.Phase();
        const ULONGLONG start = GetTickCount64();
        while (GetTickCount64() - start < 200) { Pump(); Sleep(10); }
        Check(linear.Phase() == phase, "the shared clock does not move an indicator in reduced motion");
    }
    linear.SetIndeterminate(false);
    Progress::SetReducedMotionProbe([] { return false; });
    Check(linear.EffectiveStatusText().IsEmpty(), "no status text is invented without reduced motion");

    // ========================================================================= Chips
    ChipSet chips;
    chips.Attach(&host);
    Chip* assist = chips.Add(ChipKind::Assist, L"Add to calendar", 601);
    Chip* filter = chips.Add(ChipKind::Filter, L"Unread", 602);
    Chip* input = chips.Add(ChipKind::Input, L"alice@example.com", 603);
    Chip* suggestion = chips.Add(ChipKind::Suggestion, L"Try a shorter name", 604);
    Check(assist && filter && input && suggestion && chips.Count() == 4, "chips of all four kinds created");
    chips.Layout(CPoint(10, 250), 880);
    host.log.clear();
    ClickAt(assist->GetSafeHwnd(), CPoint(Px(*assist, 20), Px(*assist, 20)));
    Check(host.Count(601, BN_CLICKED) == 1, "clicking an assist chip notifies the parent once");
    Key(assist->GetSafeHwnd(), VK_SPACE);
    Key(assist->GetSafeHwnd(), VK_RETURN);
    Check(host.Count(601, BN_CLICKED) == 3, "Space and Enter activate a chip");
    Key(assist->GetSafeHwnd(), VK_DELETE);
    Check(!assist->IsRemoved() && host.Count(601, ChipNotifyRemoved) == 0, "Delete does not remove a chip that is not an input chip");
    assist->SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, Point(Px(*assist, 20), Px(*assist, 20)));
    assist->SendMessage(WM_LBUTTONUP, 0, Point(-9, -9));
    Check(host.Count(601, BN_CLICKED) == 3, "a press released outside a chip does not activate it");
    ClickAt(suggestion->GetSafeHwnd(), CPoint(Px(*suggestion, 30), Px(*suggestion, 20)));
    Check(host.Count(604, BN_CLICKED) == 1, "clicking a suggestion chip notifies the parent");

    // Filter chip: toggles, shows a check, grows, and pushes its neighbours along.
    const int filterWidth = InParent(*filter, host).Width();
    const int inputLeft = InParent(*input, host).left;
    ClickAt(filter->GetSafeHwnd(), CPoint(Px(*filter, 30), Px(*filter, 20)));
    Check(filter->IsSelected() && host.Count(602, BN_CLICKED) == 1, "clicking a filter chip selects it and notifies once");
    Check(InParent(*filter, host).Width() > filterWidth && InParent(*input, host).left > inputLeft, "a selected filter chip grows for its check and the next chip moves over");
    Check(chips.SelectedIds().size() == 1 && chips.SelectedIds()[0] == 602, "the set reports its selected chips");
    Key(filter->GetSafeHwnd(), VK_SPACE);
    Check(!filter->IsSelected() && InParent(*filter, host).Width() == filterWidth && InParent(*input, host).left == inputLeft && host.Count(602, BN_CLICKED) == 2,
        "Space clears the selection and the layout returns");
    filter->SetSelected(true);
    Check(filter->IsSelected() && host.Count(602, BN_CLICKED) == 2, "programmatic selection is silent");
    filter->SetSelected(false);
    assist->SetSelected(true);
    Check(!assist->IsSelected(), "an assist chip cannot be selected");
    {
        auto provider = Provider(*filter);
        auto toggle = Pattern<IToggleProvider>(provider.Get(), UIA_TogglePatternId);
        ToggleState state{};
        Check(toggle && SUCCEEDED(toggle->get_ToggleState(&state)) && state == ToggleState_Off && !Pattern<IInvokeProvider>(provider.Get(), UIA_InvokePatternId)
            && NumberProperty(provider.Get(), UIA_ControlTypePropertyId) == UIA_ButtonControlTypeId, "a filter chip is a toggle button for automation");
        if (toggle) toggle->Toggle();
        Pump();
        toggle->get_ToggleState(&state);
        Check(filter->IsSelected() && state == ToggleState_On && host.Count(602, BN_CLICKED) == 3, "automation Toggle selects the filter chip and notifies");
        filter->SetSelected(false);
        auto assistProvider = Provider(*assist);
        auto invoke = Pattern<IInvokeProvider>(assistProvider.Get(), UIA_InvokePatternId);
        Check(invoke && !Pattern<IToggleProvider>(assistProvider.Get(), UIA_TogglePatternId) && TextProperty(assistProvider.Get(), UIA_NamePropertyId) == L"Add to calendar"
            && TextProperty(assistProvider.Get(), UIA_LocalizedControlTypePropertyId) == L"chip", "an assist chip is an invokable button named by its label");
        if (invoke) invoke->Invoke();
        Pump();
        Check(host.Count(601, BN_CLICKED) == 4, "automation Invoke activates the chip");
        auto inputProvider = Provider(*input);
        Check(TextProperty(inputProvider.Get(), UIA_AcceleratorKeyPropertyId) == L"Delete" && !TextProperty(inputProvider.Get(), UIA_HelpTextPropertyId).IsEmpty(), "an input chip publishes its Delete shortcut");
    }

    // Input chip removal.
    host.log.clear();
    const int suggestionLeftBefore = InParent(*suggestion, host).left;
    Check(!input->PartRect(ChipPart::Close).IsRectEmpty() && input->PartRect(ChipPart::Close).Width() >= Px(*input, 24), "an input chip has a close target");
    input->SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, Point(Px(*input, 20), Px(*input, 20)));
    input->SendMessage(WM_LBUTTONUP, 0, Point(Px(*input, 20), Px(*input, 20)));
    Check(host.Count(603, BN_CLICKED) == 1 && !input->IsRemoved(), "clicking an input chip body activates it without removing it");
    ClickAt(input->GetSafeHwnd(), input->PartRect(ChipPart::Close).CenterPoint());
    Check(host.Count(603, ChipNotifyRemoved) == 1 && input->IsRemoved() && !Visible(*input), "removing an input chip notifies the parent once and hides it");
    Check(chips.Count() == 3 && chips.Find(603) == nullptr, "a removed chip leaves the set");
    Check(InParent(*suggestion, host).left < suggestionLeftBefore, "the remaining chips close up over the removed one");
    Check(!input->Remove(), "removing a chip twice does nothing");
    chips.Layout(CPoint(10, 250), 880);
    Check(chips.Count() == 3, "the next layout destroys the removed chip");
    Chip* second = chips.Add(ChipKind::Input, L"bob@example.com", 605);
    Chip* third = chips.Add(ChipKind::Input, L"carol@example.com", 606);
    host.log.clear();
    Key(second->GetSafeHwnd(), VK_DELETE);
    Key(third->GetSafeHwnd(), VK_BACK);
    Check(host.Count(605, ChipNotifyRemoved) == 1 && host.Count(606, ChipNotifyRemoved) == 1, "Delete and Backspace remove an input chip");
    chips.Layout(CPoint(10, 250), 880);

    // Keyboard navigation reaches the owner as requests.
    {
        Chip lone;
        Check(lone.Create(ChipKind::Assist, L"Solo", &host, 650, CPoint(500, 250)) != FALSE, "a stand-alone chip can be created");
        std::vector<int> moves;
        lone.SetNavigationHandler([&](Chip&, int delta) { moves.push_back(delta); });
        Key(lone.GetSafeHwnd(), VK_LEFT); Key(lone.GetSafeHwnd(), VK_RIGHT); Key(lone.GetSafeHwnd(), VK_HOME); Key(lone.GetSafeHwnd(), VK_END);
        Check(moves == std::vector<int>({ -1, 1, -2, 2 }), "arrow, Home and End keys ask for the neighbouring chip");
        lone.EnableWindow(FALSE);
        host.log.clear();
        Key(lone.GetSafeHwnd(), VK_SPACE);
        ClickAt(lone.GetSafeHwnd(), CPoint(Px(lone, 20), Px(lone, 20)));
        Check(host.log.empty(), "a disabled chip ignores mouse and keyboard");
    }

    // Wrapping.
    {
        ChipSet wrap;
        wrap.Attach(&host);
        for (int i = 0; i < 8; ++i) { CString label; label.Format(L"Filter number %d", i); wrap.Add(ChipKind::Filter, label, 700 + i); }
        const int wide = wrap.Layout(CPoint(10, 300), Px(host, 1400));
        const int narrow = wrap.Layout(CPoint(10, 300), Px(host, 360));
        const int rowHeight = Px(host, 40);
        Check(wide == rowHeight, "chips that fit stay on one row");
        Check(narrow >= 3 * rowHeight, "chips wrap onto further rows when the width is short");
        bool inside = true; int rows = 0, lastTop = -1;
        for (int i = 0; i < wrap.Count(); ++i)
        {
            const CRect r = InParent(*wrap.At(i), host);
            if (r.right > 10 + Px(host, 360) || r.left < 10) inside = false;
            if (r.top != lastTop) { ++rows; lastTop = r.top; }
        }
        Check(inside && rows >= 3, "no wrapped chip leaves the layout width");
        Check(wrap.Layout(CPoint(10, 300), 5) >= 8 * rowHeight, "a very narrow width puts one chip on each row");
    }

    // ================================================================== no stock controls
    {
        bool stock = HasStockChild(host.GetSafeHwnd());
        for (InputWindow* window : std::initializer_list<InputWindow*>{ &bar, &linear, &circular, assist, filter, suggestion })
            if (IsStockClass(window->GetSafeHwnd())) stock = true;
        Check(!stock, "no feedback component is, or contains, a stock system control");
    }

    // ================================================================== offscreen renders
    for (UINT dpi : { 96u, 144u })
    {
        for (Appearance appearance : { Appearance::Light, Appearance::Dark })
        {
            const char* mode = appearance == Appearance::Light ? "light" : "dark";
            auto name = [&](const char* what) { static char buffer[160]; std::snprintf(buffer, sizeof(buffer), "%s renders non-blank at %u dpi in %s", what, dpi, mode); return buffer; };
            SnackbarModel snack; snack.text = L"Collection deleted"; snack.action = L"Undo"; snack.closable = true;
            const float aw = SnackbarActionWidth(snack.action);
            auto drawSnack = [&](const SnackbarModel& m) { return Render(dpi, 400, 70, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { InputState s; DrawSnackbar(t, p, m, s); }); };
            const auto oneLine = drawSnack(snack);
            Check(Ink(oneLine) > oneLine.pixels.size() / 2, name("snackbar"));
            SnackbarModel empty = snack; empty.text = CString(); empty.action = CString();
            Check(Difference(oneLine, drawSnack(empty)) > 50, "snackbar text and action change the picture");
            SnackbarModel hot = snack; hot.hoverPart = SnackbarPart::Action;
            SnackbarModel two = snack; two.twoLines = true; two.text = L"Your changes could not be saved because the file is open in another program.";
            Check(Difference(oneLine, drawSnack(hot)) > 20 && Difference(oneLine, drawSnack(two)) > 50, "snackbar hover and two-line states differ from the idle one-line state");
            (void)aw;

            auto drawProgress = [&](const ProgressModel& m, int w, int h) { return Render(dpi, w, h, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { InputState s; DrawProgress(t, p, m, s); }); };
            ProgressModel bar50; bar50.fraction = .5f;
            ProgressModel ring50; ring50.shape = ProgressShape::Circular; ring50.fraction = .5f;
            Check(Ink(drawProgress(bar50, 200, 8)) > 100 && Ink(drawProgress(ring50, 48, 48)) > 100, name("determinate linear and circular progress"));
            ProgressModel bar25 = bar50; bar25.fraction = .25f; ProgressModel ring75 = ring50; ring75.fraction = .75f;
            Check(Difference(drawProgress(bar50, 200, 8), drawProgress(bar25, 200, 8)) > 50 && Difference(drawProgress(ring50, 48, 48), drawProgress(ring75, 48, 48)) > 20, "progress fraction changes the picture");
            ProgressModel indeterminateBar; indeterminateBar.indeterminate = true; indeterminateBar.phase = .1f;
            ProgressModel laterBar = indeterminateBar; laterBar.phase = .6f;
            ProgressModel indeterminateRing; indeterminateRing.shape = ProgressShape::Circular; indeterminateRing.indeterminate = true; indeterminateRing.phase = .1f;
            ProgressModel laterRing = indeterminateRing; laterRing.phase = .6f;
            Check(Difference(drawProgress(indeterminateBar, 200, 8), drawProgress(laterBar, 200, 8)) > 50 && Difference(drawProgress(indeterminateRing, 48, 48), drawProgress(laterRing, 48, 48)) > 20, "indeterminate progress moves with its phase");
            ProgressModel staticBar = indeterminateBar; staticBar.reducedMotion = true; staticBar.status = L"Working";
            ProgressModel staticLater = staticBar; staticLater.phase = .6f;
            ProgressModel staticRing = indeterminateRing; staticRing.reducedMotion = true; staticRing.status = L"Working";
            ProgressModel staticRingLater = staticRing; staticRingLater.phase = .6f;
            Check(Difference(drawProgress(staticBar, 200, 32), drawProgress(staticLater, 200, 32)) == 0, "reduced motion draws the same linear frame at every phase");
            Check(Difference(drawProgress(staticRing, 200, 48), drawProgress(staticRingLater, 200, 48)) == 0 && Ink(drawProgress(staticRing, 200, 48)) > 100, "reduced motion draws the same circular frame at every phase");
            ProgressModel noText = staticBar; noText.status = CString();
            Check(Difference(drawProgress(staticBar, 200, 32), drawProgress(noText, 200, 32)) > 50, "the reduced-motion presentation carries its status text");

            auto drawChip = [&](ChipKind kind, bool selected, ChipPart hover) { return Render(dpi, 180, 40, appearance,
                [&](ID2D1RenderTarget* t, const Palette& p) { InputState s; ChipModel m; m.kind = kind; m.label = L"Chip label"; m.selected = selected; m.hoverPart = hover; DrawChip(t, p, m, s); }); };
            bool allDrawn = true;
            for (ChipKind kind : { ChipKind::Assist, ChipKind::Filter, ChipKind::Input, ChipKind::Suggestion })
                if (Ink(drawChip(kind, false, ChipPart::None)) < 100) allDrawn = false;
            Check(allDrawn, name("every chip kind"));
            Check(Difference(drawChip(ChipKind::Filter, false, ChipPart::None), drawChip(ChipKind::Filter, true, ChipPart::None)) > 100, "a selected filter chip differs from an unselected one");
            Check(Difference(drawChip(ChipKind::Assist, false, ChipPart::None), drawChip(ChipKind::Assist, false, ChipPart::Body)) > 20, "a hovered chip shows its state layer");
            Check(Difference(drawChip(ChipKind::Input, false, ChipPart::None), drawChip(ChipKind::Input, false, ChipPart::Close)) > 10, "a hovered close icon shows its own state layer");
            Check(Difference(drawChip(ChipKind::Assist, false, ChipPart::None), drawChip(ChipKind::Input, false, ChipPart::None)) > 10, "an input chip draws a close icon");
        }
        auto light = Render(dpi, 400, 70, Appearance::Light, [](ID2D1RenderTarget* t, const Palette& p) { InputState s; SnackbarModel m; m.text = L"Saved"; DrawSnackbar(t, p, m, s); });
        auto dark = Render(dpi, 400, 70, Appearance::Dark, [](ID2D1RenderTarget* t, const Palette& p) { InputState s; SnackbarModel m; m.text = L"Saved"; DrawSnackbar(t, p, m, s); });
        Check(Difference(light, dark) > light.pixels.size() / 2, "the snackbar follows the light and dark themes");
    }

    // Real windows paint through their print route.
    bar.Show(withAction);
    Check(WindowInk(bar) > 200 && WindowInk(circular) > 50 && WindowInk(*assist) > 50 && WindowInk(*suggestion) > 50, "real windows paint through WM_PRINTCLIENT");
    Check(SUCCEEDED(bar.RenderStatus(0)) && SUCCEEDED(circular.RenderStatus(0)) && SUCCEEDED(assist->RenderStatus(0)), "real windows attach a Direct2D render target");
    bar.SetAppearance(Appearance::Dark);
    Check(WindowInk(bar) > 200, "the snackbar window repaints in the dark theme");
    bar.Clear();

    Snackbar::SetClock({});
    Snackbar::SetNotificationObserver({});
    Progress::SetReducedMotionProbe({});
    return failures == 0 ? 0 : 1;
}
