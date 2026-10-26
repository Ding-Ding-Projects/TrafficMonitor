// Hidden-desktop verification of the Material 3 pickers (color, font, calendar). The modal dialogs are real
// top-level windows, so they are created on a private desktop and driven with window messages and the
// dialog's scripted-hook; nothing here needs (or reaches) the visible desktop.
#include "../stdafx.h"
#include "ColorPicker.h"
#include "FontPicker.h"
#include "Calendar.h"
#include "../NativePreferences/Text.h"
#include "../NativeMenus/SearchableMenu.h"
#include <cstdio>
#include <functional>
#include <wrl/client.h>

// The focused test links only the picker, input and table sources, so the two application services they reach are stubbed.
namespace NativePreferences { Snapshot Current() { return Snapshot{}; } }
namespace NativeMenus { UINT Track(CMenu*, UINT, int, int, CWnd*, NativeRegex::Options*) { return 0; } }

namespace
{
    using Microsoft::WRL::ComPtr;
    using namespace NativeMaterial;
    CWinApp application;
    int failures{};
    void Check(bool value, const char* name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name); if (!value) ++failures; }

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
    void TypeAll(HWND edit, const wchar_t* text) { SendMessage(edit, EM_SETSEL, 0, -1); Chars(edit, text); }
    CString TextOf(HWND window) { CString text; CWnd::FromHandle(window)->GetWindowText(text); return text; }
    int Px(HWND window, float dip) { return static_cast<int>(std::lround(dip * GetDpiForWindow(window) / 96.0)); }
    bool Near(double a, double b, double tolerance = 1e-6) { return std::fabs(a - b) <= tolerance; }
    bool Contains(const CString& text, const wchar_t* needle) { return text.Find(needle) >= 0; }

    // ------------------------------------------------------------ stock control detector
    CString ClassOf(HWND window) { wchar_t name[128]{}; GetClassNameW(window, name, 128); return name; }
    std::vector<CString> StockViolations(HWND root)
    {
        struct Context { HWND root; std::vector<CString> bad; } context{ root, {} };
        if (ClassOf(root).CompareNoCase(L"#32770") == 0) context.bad.push_back(L"#32770 root");
        EnumChildWindows(root, [](HWND child, LPARAM l) -> BOOL
        {
            auto* c = reinterpret_cast<Context*>(l);
            const CString name = ClassOf(child);
            for (const wchar_t* stock : { L"#32770", L"ComboBox", L"ComboLBox", L"Button", L"msctls_updown32", L"msctls_trackbar32", L"SysListView32",
                L"SysHeader32", L"ScrollBar", L"Static", L"ListBox", L"tooltips_class32", L"SysTabControl32", L"SysTreeView32", L"SysMonthCal32", L"SysDateTimePick32" })
                if (name.CompareNoCase(stock) == 0) c->bad.push_back(name);
            // The only stock class that may exist is the borrowed text editor inside an owned compound control.
            if (name.CompareNoCase(L"Edit") == 0)
            {
                const HWND parent = GetParent(child);
                if (parent == c->root || ClassOf(parent).Left(4) != L"Afx:") c->bad.push_back(L"Edit outside an owned control");
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&context));
        return context.bad;
    }

    // ----------------------------------------------------------- offscreen renders
    struct Raster { std::vector<DWORD> pixels; D2D1_COLOR_F surface{}; };
    Raster Render(UINT dpi, int widthDip, int heightDip, Appearance appearance, const std::function<void(ID2D1RenderTarget*, const Palette&)>& draw)
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
        Raster picture;
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
    size_t Ink(const Raster& p) { size_t n = 0; for (DWORD px : p.pixels) if (!NearSurface(px, p.surface)) ++n; return n; }
    size_t Difference(const Raster& a, const Raster& b)
    {
        if (a.pixels.size() != b.pixels.size()) return 1u << 30;
        size_t n = 0;
        for (size_t i = 0; i < a.pixels.size(); ++i) if (a.pixels[i] != b.pixels[i]) ++n;
        return n;
    }
    Raster PaintWindowRaster(const std::function<HRESULT(HDC)>& paint, CWnd& window, D2D1_COLOR_F surface)
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
        const HRESULT result = paint(dc);
        GdiFlush();
        Raster picture;
        picture.surface = surface;
        if (SUCCEEDED(result)) picture.pixels.assign(static_cast<DWORD*>(bits), static_cast<DWORD*>(bits) + static_cast<size_t>(client.Width()) * client.Height());
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
        return picture;
    }
    Raster PaintInput(InputWindow& window)
    {
        return PaintWindowRaster([&](HDC dc) { return window.PaintTo(dc, true); }, window, Palette::Resolve(window.GetAppearance()).surface);
    }

    // ------------------------------------------------------------ automation helpers
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
    bool Enabled(IRawElementProviderSimple* provider)
    {
        VARIANT v{};
        bool enabled = false;
        if (SUCCEEDED(provider->GetPropertyValue(UIA_IsEnabledPropertyId, &v)) && v.vt == VT_BOOL) enabled = v.boolVal != VARIANT_FALSE;
        return enabled;
    }
    ComPtr<IRawElementProviderSimple> Provider(InputWindow& window)
    {
        ComPtr<IRawElementProviderSimple> provider;
        window.AutomationProvider(provider.GetAddressOf());
        return provider;
    }

    // ------------------------------------------------------------ scripted modal runs
    std::function<void(HWND)> g_script;
    void Shown(HWND dialog, void*) { if (g_script) g_script(dialog); }
}

int wmain()
{
    // Private desktop: the dialogs below are real top-level windows and must never reach the user's screen.
    HDESK original = GetThreadDesktop(GetCurrentThreadId());
    const std::wstring desktopName = L"TrafficMonitorPickerTest" + std::to_wstring(GetCurrentProcessId());
    HDESK desktop = CreateDesktopW(desktopName.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    Check(desktop != nullptr && SetThreadDesktop(desktop), "tests run on a private desktop");
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    PickerTestHooks hooks;
    hooks.shown = Shown;
    SetPickerTestHooks(&hooks);

    // =========================================================================== color math
    {
        const Hsv red = RgbToHsv(RGB(255, 0, 0)), green = RgbToHsv(RGB(0, 255, 0)), blue = RgbToHsv(RGB(0, 0, 255));
        Check(Near(red.h, 0) && Near(red.s, 1) && Near(red.v, 1) && Near(green.h, 120) && Near(blue.h, 240), "primary colors convert to the expected hues");
        const Hsv gray = RgbToHsv(RGB(128, 128, 128)), black = RgbToHsv(RGB(0, 0, 0)), white = RgbToHsv(RGB(255, 255, 255));
        Check(Near(gray.s, 0) && Near(black.v, 0) && Near(white.s, 0) && Near(white.v, 1), "gray, black and white have no saturation or no value");
        Check(HsvToRgb(Hsv{ 60, 1, 1 }) == RGB(255, 255, 0) && HsvToRgb(Hsv{ 180, 1, 1 }) == RGB(0, 255, 255) && HsvToRgb(Hsv{ 300, 1, 1 }) == RGB(255, 0, 255),
            "secondary hues convert back to the expected colors");
        Check(HsvToRgb(Hsv{ 360, 1, 1 }) == RGB(255, 0, 0) && HsvToRgb(Hsv{ -120, 1, 1 }) == RGB(0, 0, 255) && HsvToRgb(Hsv{ 0, 2, 2 }) == RGB(255, 0, 0) && HsvToRgb(Hsv{ 0, -1, 2 }) == RGB(255, 255, 255),
            "hue wraps and saturation and value clamp");
        bool roundTrip = true;
        int tested = 0;
        std::vector<int> levels;
        for (int i = 0; i <= 255; i += 15) levels.push_back(i);
        for (int extra : { 1, 2, 127, 128, 254 }) levels.push_back(extra);
        for (int r : levels) for (int g : levels) for (int b : levels)
        {
            ++tested;
            if (HsvToRgb(RgbToHsv(RGB(r, g, b))) != RGB(r, g, b)) roundTrip = false;
        }
        Check(roundTrip && tested > 9000, "RGB to HSV to RGB round trips exactly across the color cube");
        bool dense = true;
        for (int i = 0; i < 4000; ++i)
        {
            const int r = (i * 37) & 255, g = (i * 91 + 13) & 255, b = (i * 53 + 7) & 255;
            if (HsvToRgb(RgbToHsv(RGB(r, g, b))) != RGB(r, g, b)) dense = false;
        }
        Check(dense, "pseudo-random colors round trip through HSV");

        COLORREF c{};
        Check(ParseHexColor(L"#3A7BD5", c) && c == RGB(0x3A, 0x7B, 0xD5), "hex parser reads #RRGGBB");
        Check(ParseHexColor(L"3a7bd5", c) && c == RGB(0x3A, 0x7B, 0xD5) && ParseHexColor(L"#FFAA00", c) && c == RGB(255, 170, 0) && ParseHexColor(L"#ffaa00", c) && c == RGB(255, 170, 0),
            "hex parser accepts upper and lower case digits with or without #");
        Check(ParseHexColor(L"  #abc ", c) && c == RGB(0xAA, 0xBB, 0xCC), "hex parser expands the short form and trims blanks");
        Check(!ParseHexColor(L"#00000G", c) && !ParseHexColor(L"#GGGGGG", c) && !ParseHexColor(L"#12345Z", c), "hex parser rejects a non-hex letter");
        Check(!ParseHexColor(L"", c) && !ParseHexColor(L"#", c) && !ParseHexColor(L"#12", c) && !ParseHexColor(L"#1234", c) && !ParseHexColor(L"#12345", c)
            && !ParseHexColor(L"#1234567", c) && !ParseHexColor(L"0x123456", c) && !ParseHexColor(L"#12 456", c) && !ParseHexColor(L"##123456", c), "hex parser rejects wrong lengths and stray characters");
        Check(FormatHexColor(RGB(0x3A, 0x7B, 0xD5)) == L"#3A7BD5" && FormatHexColor(RGB(0, 0, 0)) == L"#000000", "hex formatter writes upper case #RRGGBB");
        Check(ParseHexColor(FormatHexColor(RGB(1, 2, 3)), c) && c == RGB(1, 2, 3), "hex formatter output parses back");
        Check(Near(ContrastRatio(RGB(255, 255, 255), RGB(0, 0, 0)), 21.0, 0.01) && Near(ContrastRatio(RGB(10, 20, 30), RGB(10, 20, 30)), 1.0, 1e-9), "contrast ratio spans 1 to 21");
        Check(CString(ContrastGrade(4.5)) == L"AA" && CString(ContrastGrade(3.0)) == L"AA large" && CString(ContrastGrade(2.9)) == L"Low", "contrast grades follow the WCAG thresholds");
        Check(PresetPalette().size() == 20, "the preset palette has two rows of ten");
        bool unique = true;
        for (size_t i = 0; i < PresetPalette().size(); ++i) for (size_t j = i + 1; j < PresetPalette().size(); ++j) if (PresetPalette()[i] == PresetPalette()[j]) unique = false;
        Check(unique, "preset colors are distinct");
        SetRecentColors({});
        for (int i = 0; i < 12; ++i) RememberColor(RGB(i, i, i));
        RememberColor(RGB(5, 5, 5));
        const auto recent = RecentColors();
        Check(recent.size() == RecentColorLimit && recent[0] == RGB(5, 5, 5) && recent[1] == RGB(11, 11, 11), "recent colors keep the newest first, unique, at most eight");
        SetRecentColors({});
        Check(RecentColors().empty(), "recent colors can be cleared");
        double s = 0, v = 0;
        ColorArea::PointToSv(-50, 5000, 320, 176, s, v);
        Check(Near(s, 0) && Near(v, 0), "a point beyond the lower left corner clamps to saturation 0 and value 0");
        ColorArea::PointToSv(5000, -50, 320, 176, s, v);
        Check(Near(s, 1) && Near(v, 1), "a point beyond the upper right corner clamps to saturation 1 and value 1");
    }

    // =========================================================================== stock detector is not vacuous
    Host host;
    Check(host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Picker verification", WS_POPUP, 0, 0, 1000, 1000, nullptr, 0) != FALSE, "hidden native parent created");
    {
        CButton stock;
        stock.Create(L"stock", WS_CHILD | BS_PUSHBUTTON, CRect(0, 0, 50, 20), &host, 900);
        CEdit edit;
        edit.Create(WS_CHILD, CRect(0, 30, 50, 50), &host, 901);
        Check(StockViolations(host.GetSafeHwnd()).size() == 2, "the stock control detector flags a button and an edit placed directly in a window");
        stock.DestroyWindow();
        edit.DestroyWindow();
    }

    // =========================================================================== color components
    {
        ColorArea area;
        Check(area.Create(L"Color area", CRect(10, 10, 330, 186), &host, 301) != FALSE, "color area created");
        host.log.clear();
        area.SetHsv(Hsv{ 200, 0.5, 0.5 });
        Check(Near(area.GetHsv().s, .5) && host.log.empty(), "programmatic color area change is silent");
        Key(area.GetSafeHwnd(), VK_RIGHT);
        Check(Near(area.GetHsv().s, .51) && host.Count(301, EN_CHANGE) == 1, "color area Right arrow adds one percent of saturation and notifies");
        Key(area.GetSafeHwnd(), VK_UP);
        Check(Near(area.GetHsv().v, .51) && host.Count(301, EN_CHANGE) == 2, "color area Up arrow adds one percent of value and notifies");
        Key(area.GetSafeHwnd(), VK_END);
        Key(area.GetSafeHwnd(), VK_PRIOR);
        Check(Near(area.GetHsv().s, 1) && Near(area.GetHsv().v, 1), "color area End and PageUp jump to the right and top edges");
        const int before = host.Count(301, EN_CHANGE);
        Key(area.GetSafeHwnd(), VK_RIGHT);
        Check(host.Count(301, EN_CHANGE) == before && Near(area.GetHsv().s, 1), "color area arrow at the edge keeps the value and notifies nothing");
        Key(area.GetSafeHwnd(), VK_HOME);
        Key(area.GetSafeHwnd(), VK_NEXT);
        Check(Near(area.GetHsv().s, 0) && Near(area.GetHsv().v, 0), "color area Home and PageDown jump to the left and bottom edges");
        CRect client;
        area.GetClientRect(client);
        Click(area.GetSafeHwnd(), client.right - 1, 1);
        Check(Near(area.GetHsv().s, 1) && Near(area.GetHsv().v, 1), "clicking the upper right corner chooses full saturation and value");
        Click(area.GetSafeHwnd(), client.CenterPoint().x, client.CenterPoint().y);
        Check(Near(area.GetHsv().s, .5, .02) && Near(area.GetHsv().v, .5, .02), "clicking the middle chooses half saturation and half value");
        area.SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, Point(client.CenterPoint().x, client.CenterPoint().y));
        area.SendMessage(WM_MOUSEMOVE, MK_LBUTTON, Point(client.left + 1, client.bottom - 1));
        area.SendMessage(WM_LBUTTONUP, 0, Point(client.left + 1, client.bottom - 1));
        Check(Near(area.GetHsv().s, 0, .02) && Near(area.GetHsv().v, 0, .02), "dragging follows the pointer to the lower left corner");
        {
            auto provider = Provider(area);
            VARIANT type{};
            provider->GetPropertyValue(UIA_ControlTypePropertyId, &type);
            Check(Name(provider.Get()) == L"Color area" && type.lVal == UIA_CustomControlTypeId, "color area exposes its name and a custom role");
            auto value = Pattern<IValueProvider>(provider.Get(), UIA_ValuePatternId);
            BSTR text = nullptr;
            BOOL readOnly = FALSE;
            const bool ok = value && SUCCEEDED(value->get_Value(&text)) && text && Contains(CString(text), L"percent") && SUCCEEDED(value->get_IsReadOnly(&readOnly)) && readOnly;
            SysFreeString(text);
            Check(ok && value->SetValue(L"#FF0000") != S_OK, "color area exposes a read only Value pattern describing saturation and brightness");
        }
        area.DestroyWindow();

        HueSlider hue;
        Check(hue.Create(L"Hue", CRect(10, 200, 330, 240), &host, 302) != FALSE, "hue slider created");
        host.log.clear();
        hue.SetHue(400);
        Check(hue.GetHue() == 40 && host.log.empty(), "programmatic hue wraps into 0..359 and is silent");
        Key(hue.GetSafeHwnd(), VK_RIGHT);
        Check(hue.GetHue() == 41 && host.Count(302, EN_CHANGE) == 1, "hue Right arrow adds one degree and notifies");
        Key(hue.GetSafeHwnd(), VK_PRIOR);
        Check(hue.GetHue() == 56, "hue PageUp adds fifteen degrees");
        Key(hue.GetSafeHwnd(), VK_END);
        Check(hue.GetHue() == 359, "hue End jumps to 359");
        const int hueBefore = host.Count(302, EN_CHANGE);
        Key(hue.GetSafeHwnd(), VK_RIGHT);
        Check(hue.GetHue() == 359 && host.Count(302, EN_CHANGE) == hueBefore, "hue at the end keeps the value and notifies nothing");
        Key(hue.GetSafeHwnd(), VK_HOME);
        Check(hue.GetHue() == 0, "hue Home jumps to 0");
        hue.GetClientRect(client);
        Click(hue.GetSafeHwnd(), client.CenterPoint().x, client.CenterPoint().y);
        Check(hue.GetHue() > 170 && hue.GetHue() < 190, "clicking the middle of the hue slider chooses about 180 degrees");
        {
            auto provider = Provider(hue);
            auto range = Pattern<IRangeValueProvider>(provider.Get(), UIA_RangeValuePatternId);
            double value = -1, maximum = -1;
            Check(range && SUCCEEDED(range->get_Value(&value)) && SUCCEEDED(range->get_Maximum(&maximum)) && maximum == 359 && value == hue.GetHue() && ControlType(provider.Get()) == UIA_SliderControlTypeId,
                "hue slider exposes RangeValue 0 to 359 and a slider role");
            range->SetValue(90.0);
            Pump();
            Check(hue.GetHue() == 90, "automation SetValue moves the hue slider");
        }
        hue.DestroyWindow();

        SwatchGrid swatches;
        Check(swatches.Create(L"Preset colors", 10, CRect(10, 260, 340, 330), &host, 303) != FALSE, "swatch grid created");
        swatches.SetColors(PresetPalette());
        host.log.clear();
        const CSize natural = swatches.NaturalSize();
        Check(natural.cx <= 320 && swatches.Rows() == 2 && natural.cy > 2 * 26, "twenty swatches in ten columns need two rows inside the dialog width");
        Click(swatches.GetSafeHwnd(), Px(swatches.GetSafeHwnd(), 2 + 3 * 32 + 13), Px(swatches.GetSafeHwnd(), 2 + 13));
        Check(swatches.GetSelected() == 3 && host.Count(303, CBN_SELCHANGE) == 1, "clicking a swatch selects it and notifies once");
        Click(swatches.GetSafeHwnd(), Px(swatches.GetSafeHwnd(), 2 + 3 * 32 + 13), Px(swatches.GetSafeHwnd(), 2 + 13));
        Check(host.Count(303, CBN_SELCHANGE) == 1, "choosing the selected swatch again does not notify");
        Click(swatches.GetSafeHwnd(), Px(swatches.GetSafeHwnd(), 2 + 3 * 32 + 29), Px(swatches.GetSafeHwnd(), 2 + 13));
        Check(swatches.GetSelected() == 3, "a click in the gap between swatches chooses nothing");
        Key(swatches.GetSafeHwnd(), VK_RIGHT);
        Check(swatches.GetSelected() == 4 && host.Count(303, CBN_SELCHANGE) == 2, "Right arrow chooses the next swatch");
        Key(swatches.GetSafeHwnd(), VK_DOWN);
        Check(swatches.GetSelected() == 14, "Down arrow moves one row");
        Key(swatches.GetSafeHwnd(), VK_END);
        Key(swatches.GetSafeHwnd(), VK_RIGHT);
        Check(swatches.GetSelected() == 19, "arrows stop at the last swatch");
        swatches.SetSelected(2);
        Check(swatches.GetSelected() == 2 && host.Count(303, CBN_SELCHANGE) == 4, "programmatic swatch selection is silent");
        {
            auto provider = Provider(swatches);
            auto selection = Pattern<ISelectionProvider>(provider.Get(), UIA_SelectionPatternId);
            SAFEARRAY* selected = nullptr;
            const bool ok = selection && SUCCEEDED(selection->GetSelection(&selected)) && selected && selected->rgsabound[0].cElements == 1;
            if (selected) SafeArrayDestroy(selected);
            Check(ok && Name(provider.Get()) == L"Preset colors", "swatch grid exposes a Selection pattern with one selected item");
        }
        swatches.DestroyWindow();

        // Offscreen renders of every drawn part: light and dark, 96 and 144 dpi.
        bool drawn = true;
        size_t changes = 0;
        for (const UINT dpi : { 96u, 144u })
        {
            Raster light[4], dark[4];
            for (int part = 0; part < 4; ++part)
            {
                auto draw = [&](ID2D1RenderTarget* t, const Palette& p)
                {
                    if (part == 0) DrawColorArea(t, p, Hsv{ 210, .6, .7 }, InputState{ true, false, false, true, false });
                    else if (part == 1) DrawHueSlider(t, p, 210, InputState{ true, false, false, true, false });
                    else if (part == 2) DrawSwatches(t, p, PresetPalette(), 10, 5, 5, 7, InputState{ true, false, false, true, false });
                    else DrawContrastPreview(t, p, RGB(52, 94, 156), D2D1::RectF(4, 4, 324, 60));
                };
                const int height = part == 0 ? 176 : part == 1 ? 40 : part == 2 ? 70 : 64;
                light[part] = Render(dpi, 330, height, Appearance::Light, draw);
                dark[part] = Render(dpi, 330, height, Appearance::Dark, draw);
                if (Ink(light[part]) == 0 || Ink(dark[part]) == 0) drawn = false;
                changes += Difference(light[part], dark[part]);
            }
        }
        Check(drawn, "color area, hue slider, swatches and contrast preview render non-blank in light and dark at 96 and 144 dpi");
        Check(changes > 1000, "the theme changes the drawn pixels");
    }

    // =========================================================================== color picker dialog
    SetRecentColors({});
    COLORREF color = RGB(52, 94, 156);
    {
        std::vector<HWND> walked;
        g_script = [&](HWND handle)
        {
            auto* dialog = ColorPickerDialog::FromHandle(handle);
            Check(dialog != nullptr, "the color picker dialog is reachable from its window");
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            Check(ClassOf(handle).CompareNoCase(L"#32770") != 0 && StockViolations(handle).empty(), "the color picker uses no common dialog and no stock child control");
            Check(dialog->Color() == RGB(52, 94, 156) && dialog->OriginalColor() == RGB(52, 94, 156), "the dialog starts on the given color");
            Check(dialog->HexField().GetText() == L"#345E9C" && dialog->Red().GetValue() == 52 && dialog->Green().GetValue() == 94 && dialog->Blue().GetValue() == 156,
                "hex field and steppers show the starting color");
            const Hsv start = RgbToHsv(RGB(52, 94, 156));
            Check(Near(dialog->Area().GetHsv().s, start.s) && Near(dialog->Area().GetHsv().v, start.v) && dialog->Hue().GetHue() == static_cast<int>(std::lround(start.h)),
                "color area and hue slider show the starting color");
            const auto order = dialog->FocusOrder();
            Check(order.size() == 9 && order.front() == dialog->Area().GetSafeHwnd() && order.back() == dialog->OkWindow(), "the focus order runs from the color area to OK");
            Check(::GetFocus() == dialog->Area().GetSafeHwnd(), "the color area has the initial focus");
            bool cycle = true;
            for (size_t i = 1; i <= order.size(); ++i)
            {
                dialog->HandleKey(VK_TAB, false);
                const HWND expected = order[i % order.size()];
                const HWND focus = ::GetFocus();
                if (!(focus == expected || ::IsChild(expected, focus))) cycle = false;
            }
            Check(cycle && ::GetFocus() == dialog->Area().GetSafeHwnd(), "Tab visits every control once and wraps back to the color area");
            dialog->HandleKey(VK_TAB, true);
            Check(::GetFocus() == dialog->OkWindow(), "Shift+Tab from the first control wraps to OK");
            ::SetFocus(dialog->Area().GetSafeHwnd());

            // Hex typing: invalid while incomplete, valid at the end.
            const HWND hexEdit = dialog->HexField().Field().GetSafeHwnd();
            TypeAll(hexEdit, L"#FF88");
            Check(!dialog->HexValid() && !dialog->IsOkEnabled() && !::IsWindowEnabled(dialog->OkWindow()) && !dialog->HexField().GetErrorText().IsEmpty(),
                "an incomplete hex value shows an error and disables OK");
            ::SetFocus(hexEdit);
            dialog->HandleKey(VK_RETURN, false);
            Check(!dialog->Finished(), "Enter does not accept while the hex value is invalid");
            Chars(hexEdit, L"00");
            Check(dialog->HexValid() && dialog->IsOkEnabled() && dialog->HexField().GetErrorText().IsEmpty() && dialog->Color() == RGB(255, 136, 0),
                "a complete hex value applies, clears the error and enables OK");
            Check(dialog->Red().GetValue() == 255 && dialog->Green().GetValue() == 136 && dialog->Blue().GetValue() == 0 && dialog->Hue().GetHue() == 32,
                "typing hex updates the steppers and the hue slider");
            dialog->Red().StepBy(-5);
            Check(dialog->Color() == RGB(250, 136, 0) && dialog->HexField().GetText() == L"#FA8800", "stepping red updates the color and the hex field");
            Key(dialog->Area().GetSafeHwnd(), VK_LEFT);
            Check(Near(dialog->Area().GetHsv().s, dialog->CurrentHsv().s) && dialog->Color() != RGB(250, 136, 0) && dialog->Red().GetValue() == GetRValue(dialog->Color()),
                "an arrow key in the color area changes the color and the steppers");
            const COLORREF afterArea = dialog->Color();
            Key(dialog->Hue().GetSafeHwnd(), VK_RIGHT);
            Check(dialog->Color() != afterArea && dialog->Hue().GetHue() == static_cast<int>(std::lround(dialog->CurrentHsv().h)) % 360, "an arrow key on the hue slider changes the hue and the color");
            const COLORREF blue = PresetPalette()[5];
            Click(dialog->Presets().GetSafeHwnd(), Px(handle, 2 + 5 * 32 + 13), Px(handle, 2 + 13));
            Check(dialog->Color() == blue && dialog->HexField().GetText() == FormatHexColor(blue) && dialog->Presets().GetSelected() == 5, "choosing a preset swatch applies it everywhere");
            const Hsv hueBefore = dialog->CurrentHsv();
            dialog->SetColor(RGB(0, 0, 0));
            Check(Near(dialog->CurrentHsv().h, RgbToHsv(RGB(0, 0, 0)).h) || Near(dialog->CurrentHsv().h, hueBefore.h), "setting black keeps a defined hue");
            dialog->SetColor(blue);
            Check(dialog->Color() == blue && dialog->HexField().GetText() == FormatHexColor(blue), "SetColor updates every part");
            ::SetFocus(dialog->Presets().GetSafeHwnd());
            // Enter reaches the dialog through the real message loop.
            PostMessage(handle, WM_KEYDOWN, VK_RETURN, 0);
        };
        const bool accepted = PickColor(nullptr, color, L"Text color");
        Check(accepted && color == PresetPalette()[5], "Enter through the real message loop accepts and returns the chosen color");
        Check(RecentColors().size() == 1 && RecentColors()[0] == PresetPalette()[5], "an accepted color is remembered as a recent color");
    }
    {
        const COLORREF unchanged = RGB(10, 20, 30);
        COLORREF value = unchanged;
        g_script = [&](HWND handle)
        {
            auto* dialog = ColorPickerDialog::FromHandle(handle);
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            dialog->SetColor(RGB(200, 100, 50));
            Check(dialog->Recents().Colors().size() == 1 && dialog->FocusOrder().size() == 10, "the recent colors row joins the focus order once it has colors");
            dialog->HandleKey(VK_ESCAPE, false);
        };
        const bool accepted = PickColor(nullptr, value, CString());
        Check(!accepted && value == unchanged, "Escape cancels and leaves the color untouched");

        g_script = [&](HWND handle)
        {
            auto* dialog = ColorPickerDialog::FromHandle(handle);
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            dialog->SetColor(RGB(1, 2, 3));
            ClickCenter(*CWnd::FromHandle(dialog->CancelWindow()));
        };
        value = unchanged;
        Check(!PickColor(nullptr, value, CString()) && value == unchanged, "clicking Cancel returns false and keeps the color");

        g_script = [&](HWND handle)
        {
            auto* dialog = ColorPickerDialog::FromHandle(handle);
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            dialog->SetColor(RGB(1, 2, 3));
            ClickCenter(*CWnd::FromHandle(dialog->OkWindow()));
        };
        value = unchanged;
        Check(PickColor(nullptr, value, CString()) && value == RGB(1, 2, 3), "clicking OK returns true and the new color");

        g_script = [&](HWND handle)
        {
            auto* dialog = ColorPickerDialog::FromHandle(handle);
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            TypeAll(dialog->HexField().Field().GetSafeHwnd(), L"#GG");
            ClickCenter(*CWnd::FromHandle(dialog->OkWindow()));
            Check(!dialog->Finished() && !dialog->HexValid(), "clicking OK with an invalid hex value does nothing");
            TypeAll(dialog->HexField().Field().GetSafeHwnd(), L"#00FF7F");
            Check(dialog->HexValid() && dialog->Color() == RGB(0, 255, 127), "a corrected hex value is accepted again");
            ::SetFocus(dialog->Red().EditWindow());
            TypeAll(dialog->Red().EditWindow(), L"200");
            dialog->HandleKey(VK_RETURN, false);
            Check(dialog->Finished() && dialog->Result() == IDOK && dialog->Color() == RGB(200, 255, 127), "Enter applies typed stepper text before it accepts");
        };
        value = unchanged;
        Check(PickColor(nullptr, value, CString()) && value == RGB(200, 255, 127), "typed stepper text reaches the returned color");

        g_script = [&](HWND handle)
        {
            auto* dialog = ColorPickerDialog::FromHandle(handle);
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            const auto recents = RecentColors();
            Check(!recents.empty() && dialog->Recents().Colors() == recents, "the recent colors row shows the remembered colors");
            Click(dialog->Recents().GetSafeHwnd(), Px(handle, 2 + 13), Px(handle, 2 + 13));
            Check(dialog->Color() == recents[0] && dialog->Recents().GetSelected() == 0, "choosing a recent color applies it");
            dialog->HandleKey(VK_ESCAPE, false);
        };
        value = unchanged;
        PickColor(nullptr, value, CString());
    }
    {
        // Light and dark dialog surfaces differ and both draw.
        Raster lightPicture, darkPicture;
        COLORREF value = RGB(52, 94, 156);
        auto capture = [&](Raster& out, Appearance expected)
        {
            g_script = [&out, expected](HWND handle)
            {
                auto* dialog = ColorPickerDialog::FromHandle(handle);
                if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
                Check(dialog->GetAppearance() == expected, "the dialog follows the forced appearance");
                const auto palette = Palette::Resolve(expected);
                out = PaintWindowRaster([&](HDC dc) { return dialog->PaintTo(dc); }, *dialog, BlendColor(palette.onSurface, palette.surfaceContainer, .04f));
                dialog->HandleKey(VK_ESCAPE, false);
            };
        };
        hooks.forceAppearance = true;
        hooks.appearance = Appearance::Light;
        capture(lightPicture, Appearance::Light);
        PickColor(nullptr, value, CString());
        hooks.appearance = Appearance::Dark;
        capture(darkPicture, Appearance::Dark);
        PickColor(nullptr, value, CString());
        hooks.forceAppearance = false;
        Check(Ink(lightPicture) > 5000 && Ink(darkPicture) > 5000, "the color picker dialog surface renders non-blank in light and dark");
        Check(Difference(lightPicture, darkPicture) > 20000, "light and dark color picker dialogs differ");
    }

    // =========================================================================== font picker
    {
        Check(ClampFontPoints(0) == FontPointsMin && ClampFontPoints(-5) == FontPointsMin && ClampFontPoints(500) == FontPointsMax && ClampFontPoints(12) == 12, "font size clamps into the allowed range");
        const auto families = EnumerateFontFamilies();
        Check(!families.empty(), "installed font families are enumerated");
        bool sorted = true, noVertical = true;
        for (size_t i = 0; i < families.size(); ++i)
        {
            if (families[i].IsEmpty() || families[i][0] == L'@') noVertical = false;
            if (i && families[i - 1].CompareNoCase(families[i]) >= 0) sorted = false;
        }
        Check(sorted && noVertical, "families are sorted, unique and free of vertical @ families");
        const auto contains = [&](const wchar_t* name) { for (const auto& f : families) if (f.CompareNoCase(name) == 0) return true; return false; };
        Check(contains(L"Segoe UI"), "Segoe UI is among the enumerated families");
        CString error;
        const auto all = FilterFontFamilies(families, CString(), false, &error);
        Check(all.size() == families.size() && error.IsEmpty(), "an empty query keeps every family");
        const auto plain = FilterFontFamilies(families, L"SEGOE", false, &error);
        bool plainOk = !plain.empty() && plain.size() < families.size();
        for (const int i : plain) { CString lower = families[i]; lower.MakeLower(); if (lower.Find(L"segoe") < 0) plainOk = false; }
        Check(plainOk, "the plain filter is a case insensitive substring match");
        Check(FilterFontFamilies(families, L"^seg", false, &error).empty() && !FilterFontFamilies(families, L"^seg", true, &error).empty(), "the regex option turns the query into a regular expression");
        const auto bad = FilterFontFamilies(families, L"(", true, &error);
        Check(bad.empty() && !error.IsEmpty(), "an invalid regular expression yields no rows and an error message");
        Check(FilterFontFamilies(families, L"^Segoe UI$", true, &error).size() == 1 && error.IsEmpty(), "an anchored regular expression finds exactly one family");
        LOGFONT font{};
        wcscpy_s(font.lfFaceName, L"Consolas");
        font.lfWeight = FW_BOLD; font.lfItalic = TRUE; font.lfUnderline = FALSE; font.lfStrikeOut = TRUE;
        const FontChoice choice = ChoiceFromLogFont(font, 500);
        Check(choice.face == L"Consolas" && choice.points == FontPointsMax && choice.bold && choice.italic && !choice.underline && choice.strike, "a LOGFONT converts into a clamped font choice");
        LOGFONT written{};
        FontChoice plainChoice;
        plainChoice.face = L"Arial"; plainChoice.points = 14; plainChoice.italic = true; plainChoice.underline = true;
        ApplyFontChoice(plainChoice, written);
        Check(CString(written.lfFaceName) == L"Arial" && written.lfWeight == FW_NORMAL && written.lfItalic && written.lfUnderline && !written.lfStrikeOut && written.lfHeight < 0,
            "a font choice converts back into a LOGFONT");

        bool drawn = true;
        for (const UINT dpi : { 96u, 144u })
            for (const Appearance appearance : { Appearance::Light, Appearance::Dark })
            {
                const auto picture = Render(dpi, 520, 70, appearance, [&](ID2D1RenderTarget* t, const Palette& p) { DrawFontPreview(t, p, plainChoice, D2D1::RectF(4, 4, 516, 68)); });
                if (Ink(picture) < 500) drawn = false;
            }
        Check(drawn, "the font preview renders non-blank in light and dark at 96 and 144 dpi");
        if (contains(L"Consolas"))
        {
            FontChoice a, b;
            a.face = L"Segoe UI"; b.face = L"Consolas";
            const auto pa = Render(96, 520, 70, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawFontPreview(t, p, a, D2D1::RectF(4, 4, 516, 68)); });
            const auto pb = Render(96, 520, 70, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawFontPreview(t, p, b, D2D1::RectF(4, 4, 516, 68)); });
            Check(Difference(pa, pb) > 300, "the preview draws the selected family");
        }
        FontChoice big = plainChoice, bold = plainChoice;
        big.points = 30; bold.bold = true;
        const auto base = Render(96, 520, 70, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawFontPreview(t, p, plainChoice, D2D1::RectF(4, 4, 516, 68)); });
        const auto larger = Render(96, 520, 70, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawFontPreview(t, p, big, D2D1::RectF(4, 4, 516, 68)); });
        const auto heavier = Render(96, 520, 70, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawFontPreview(t, p, bold, D2D1::RectF(4, 4, 516, 68)); });
        Check(Difference(base, larger) > 300 && Difference(base, heavier) > 100, "the preview reflects the size and weight");
    }
    {
        LOGFONT font{};
        wcscpy_s(font.lfFaceName, L"Segoe UI");
        font.lfWeight = FW_NORMAL;
        int points = 10;
        bool arialChosen = false;
        g_script = [&](HWND handle)
        {
            auto* dialog = FontPickerDialog::FromHandle(handle);
            Check(dialog != nullptr, "the font picker dialog is reachable from its window");
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            Check(ClassOf(handle).CompareNoCase(L"#32770") != 0 && StockViolations(handle).empty(), "the font picker uses no common dialog and no stock child control");
            Check(dialog->AllFamilies().size() == EnumerateFontFamilies().size() && dialog->List().GetCount() == static_cast<int>(dialog->VisibleFamilies().size()), "the list shows every installed family");
            const int selected = dialog->List().GetCurSel();
            Check(selected >= 0 && dialog->List().GetItemText(selected).CompareNoCase(L"Segoe UI") == 0, "the incoming family is selected in the list");
            Check(dialog->Size().GetValue() == 10 && !dialog->Bold().IsChecked() && !dialog->Italic().IsChecked() && !dialog->Underline().IsChecked() && !dialog->Strike().IsChecked(),
                "size and style options start from the incoming font");
            Check(dialog->FocusOrder().size() == 10 && ::GetFocus() == dialog->List().GetSafeHwnd(), "the list has the initial focus and the focus order covers every control");

            const size_t everything = dialog->VisibleFamilies().size();
            Chars(dialog->Search().Field().GetSafeHwnd(), L"segoe");
            bool narrowed = !dialog->VisibleFamilies().empty() && dialog->VisibleFamilies().size() < everything && dialog->List().GetCount() == static_cast<int>(dialog->VisibleFamilies().size());
            for (const auto& family : dialog->VisibleFamilies()) { CString lower = family; lower.MakeLower(); if (lower.Find(L"segoe") < 0) narrowed = false; }
            Check(narrowed, "typing in the search field narrows the list");
            Check(dialog->List().GetCurSel() >= 0 && dialog->List().GetItemText(dialog->List().GetCurSel()).CompareNoCase(L"Segoe UI") == 0, "the selection survives a filter that still shows it");
            ClickCenter(dialog->RegexOption());
            Check(dialog->RegexOption().IsChecked() && !dialog->VisibleFamilies().empty(), "the Regex option re-runs the search as a regular expression");
            TypeAll(dialog->Search().Field().GetSafeHwnd(), L"(");
            Check(dialog->VisibleFamilies().empty() && !dialog->Search().GetErrorText().IsEmpty() && dialog->List().GetCount() == 0, "an invalid regular expression empties the list and shows an error");
            TypeAll(dialog->Search().Field().GetSafeHwnd(), L"^Segoe UI$");
            Check(dialog->VisibleFamilies().size() == 1 && dialog->Search().GetErrorText().IsEmpty() && dialog->List().GetCurSel() == 0, "an anchored regular expression shows exactly Segoe UI");
            dialog->Search().Clear();
            Check(dialog->VisibleFamilies().size() == everything, "clearing the search shows every family again");

            const CString before = dialog->Choice().face;
            ::SetFocus(dialog->List().GetSafeHwnd());
            Key(dialog->List().GetSafeHwnd(), VK_DOWN);
            const int row = dialog->List().GetCurSel();
            Check(row >= 0 && dialog->Choice().face == dialog->VisibleFamilies()[row] && dialog->Choice().face != before, "moving in the list chooses the family and updates the preview choice");
            const bool chose = dialog->ChooseFamily(L"Arial");
            arialChosen = chose;
            Check(!chose || dialog->Choice().face == L"Arial", "ChooseFamily selects an installed family");

            Check(dialog->Size().StepBy(4) && dialog->Choice().points == 14, "the size stepper changes the point size");
            dialog->Size().StepBy(1000);
            Check(dialog->Choice().points == FontPointsMax, "the size stops at the maximum");
            dialog->Size().StepBy(-1000);
            Check(dialog->Choice().points == FontPointsMin, "the size stops at the minimum");
            dialog->Size().StepBy(13);
            ClickCenter(dialog->Bold());
            ClickCenter(dialog->Italic());
            ClickCenter(dialog->Underline());
            ClickCenter(dialog->Strike());
            const FontChoice choice = dialog->Choice();
            Check(choice.points == 14 && choice.bold && choice.italic && choice.underline && choice.strike, "the style options reach the font choice");
            ::SetFocus(dialog->List().GetSafeHwnd());
            PostMessage(handle, WM_KEYDOWN, VK_RETURN, 0);
        };
        const bool accepted = PickFont(nullptr, font, points);
        Check(!arialChosen || CString(font.lfFaceName) == L"Arial", "the family chosen in the list is the one written to the LOGFONT");
        Check(accepted && points == 14 && font.lfWeight == FW_BOLD && font.lfItalic && font.lfUnderline && font.lfStrikeOut && font.lfFaceName[0] != L'\0', "Enter through the real message loop accepts and writes the LOGFONT and the point size");

        LOGFONT untouched{};
        wcscpy_s(untouched.lfFaceName, L"Segoe UI");
        untouched.lfWeight = 600;
        int untouchedPoints = 0;
        untouched.lfHeight = -MulDiv(12, static_cast<int>(GetDpiForSystem()), 72);
        g_script = [&](HWND handle)
        {
            auto* dialog = FontPickerDialog::FromHandle(handle);
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            Check(dialog->Size().GetValue() == 12 && dialog->Bold().IsChecked(), "a missing point size is derived from the incoming height and a semi-bold weight shows as bold");
            dialog->HandleKey(VK_ESCAPE, false);
        };
        const LOGFONT before = untouched;
        Check(!PickFont(nullptr, untouched, untouchedPoints) && memcmp(&before, &untouched, sizeof(LOGFONT)) == 0 && untouchedPoints == 0, "Escape cancels and leaves the font and size untouched");
        g_script = [&](HWND handle)
        {
            auto* dialog = FontPickerDialog::FromHandle(handle);
            if (!dialog) { PostMessage(handle, WM_CLOSE, 0, 0); return; }
            dialog->Size().StepBy(2);
            ClickCenter(*CWnd::FromHandle(dialog->OkWindow()));
        };
        Check(PickFont(nullptr, untouched, untouchedPoints) && untouchedPoints == 14 && untouched.lfWeight == 600, "an untouched Bold option keeps the incoming semi-bold weight");
    }

    // =========================================================================== calendar: date math
    {
        Check(IsLeapYear(2000) && IsLeapYear(2028) && IsLeapYear(2024) && !IsLeapYear(1900) && !IsLeapYear(2100) && !IsLeapYear(2023), "leap years follow the 400 year rule");
        Check(DaysInMonth(2028, 2) == 29 && DaysInMonth(2100, 2) == 28 && DaysInMonth(2023, 2) == 28 && DaysInMonth(2024, 2) == 29 && DaysInMonth(2026, 4) == 30 && DaysInMonth(2026, 12) == 31
            && DaysInMonth(2026, 0) == 0 && DaysInMonth(2026, 13) == 0, "days per month, including February in leap years");
        Check(DayOfWeek(1970, 1, 1) == 4 && DayOfWeek(2000, 1, 1) == 6 && DayOfWeek(2028, 2, 1) == 2 && DayOfWeek(2026, 10, 26) == 1 && DayOfWeek(2028, 2, 29) == 2 && DayOfWeek(1969, 12, 31) == 3,
            "weekdays are computed correctly, including before 1970");
        Check(CalendarDate{ 2028, 2, 29 }.Valid() && !CalendarDate{ 2027, 2, 29 }.Valid() && !CalendarDate{ 2028, 13, 1 }.Valid() && !CalendarDate{ 2028, 1, 0 }.Valid(), "date validity respects leap years");
        Check(AddDays({ 2028, 2, 29 }, 1) == CalendarDate{ 2028, 3, 1 } && AddDays({ 2027, 12, 31 }, 1) == CalendarDate{ 2028, 1, 1 } && AddDays({ 2028, 3, 1 }, -1) == CalendarDate{ 2028, 2, 29 }
            && AddDays({ 2028, 1, 1 }, 366) == CalendarDate{ 2029, 1, 1 } && AddDays({ 2028, 1, 1 }, -1) == CalendarDate{ 2027, 12, 31 }, "adding days crosses month, year and leap day boundaries");
        Check(AddMonths({ 2028, 1, 31 }, 1) == CalendarDate{ 2028, 2, 29 } && AddMonths({ 2027, 1, 31 }, 1) == CalendarDate{ 2027, 2, 28 } && AddMonths({ 2028, 12, 15 }, 1) == CalendarDate{ 2029, 1, 15 }
            && AddMonths({ 2028, 1, 15 }, -1) == CalendarDate{ 2027, 12, 15 } && AddMonths({ 2028, 5, 31 }, 12) == CalendarDate{ 2029, 5, 31 }, "adding months clamps the day to the target month");
        const auto feb = LayoutMonth(2028, 2, 0);
        Check(feb.days == 29 && feb.leading == 2 && feb.day[0][2] == 1 && feb.day[0][1] == 0 && feb.day[4][2] == 29 && feb.day[4][3] == 0, "February 2028 (leap year) starts on a Tuesday with 29 days, Sunday first");
        const auto febMonday = LayoutMonth(2028, 2, 1);
        Check(febMonday.leading == 1 && febMonday.day[0][1] == 1 && febMonday.day[0][6] == 6 && febMonday.day[4][0] == 28, "Monday first shifts the grid by one column");
        const auto jan2022 = LayoutMonth(2022, 1, 0);
        Check(jan2022.leading == 6 && jan2022.day[5][1] == 31 && jan2022.day[5][0] == 30, "a month that needs six rows fills the sixth row");
        bool counts = true;
        for (int year : { 2023, 2024, 2028, 2100 }) for (int month = 1; month <= 12; ++month) for (int first : { 0, 1 })
        {
            const auto layout = LayoutMonth(year, month, first);
            int seen = 0, previous = 0;
            bool ordered = true;
            for (int r = 0; r < CalendarRows; ++r) for (int c = 0; c < CalendarColumns; ++c)
                if (layout.day[r][c]) { ++seen; if (layout.day[r][c] != previous + 1) ordered = false; previous = layout.day[r][c]; }
            int row = 0, column = 0;
            if (seen != DaysInMonth(year, month) || !ordered || !layout.Find(1, row, column) || row != 0) counts = false;
        }
        Check(counts, "every month layout holds each day once, in order, starting in the first row");
        const auto grid = LayoutGrid(308, 320);
        Check(grid.cellHeight >= 48.f && grid.Cell(5, 6).bottom <= 320.01f && grid.Cell(5, 6).right <= 308.01f && grid.Cell(0, 0).top == 32.f, "the six week grid fits its minimum size");
    }

    // =========================================================================== calendar control
    {
        Calendar calendar;
        Check(calendar.Create(L"Traffic history", CRect(10, 10, 410, 430), &host, 700) != FALSE, "calendar created");
        calendar.SetToday({ 2028, 2, 14 });
        calendar.SetYearRange(2020, 2030);
        host.log.clear();
        int calls = 0;
        std::vector<CalendarDate> asked;
        calendar.SetDaySource([&](const CalendarDate& date, CalendarDayContent& content)
        {
            ++calls;
            asked.push_back(date);
            if (date.day == 10) { content.secondary = L"1.5 GB"; content.hasIndicator = true; content.indicator = RGB(220, 60, 40); content.description = L"Traffic 1.5 GB"; }
        });
        calendar.SetVisibleMonth(2028, 2);
        Check(calendar.VisibleYear() == 2028 && calendar.VisibleMonth() == 2 && calendar.Month().days == 29 && calendar.Month().leading == 2 && host.log.empty(), "February 2028 is shown with its leap day and silent setters");
        Check(calls >= 29 && asked.back() == CalendarDate{ 2028, 2, 29 }, "the day hook is asked for each day of the shown month");
        const int callsBefore = calls;
        calendar.SetVisibleMonth(2028, 3);
        Check(calls == callsBefore + 31 && asked.back() == CalendarDate{ 2028, 3, 31 }, "changing the month asks the hook for the new month");
        calendar.SetVisibleMonth(2028, 2);
        calendar.Refresh();
        Check(calendar.Content({ 2028, 2, 10 }).secondary == L"1.5 GB" && calendar.Content({ 2028, 2, 10 }).hasIndicator && calendar.Content({ 2028, 2, 11 }).secondary.IsEmpty(), "the hook content is kept per day");
        Check(calendar.Content({ 2028, 3, 10 }).secondary.IsEmpty(), "content outside the shown month is empty");
        Check(StockViolations(calendar.GetSafeHwnd()).empty(), "the calendar uses no stock month calendar, button, combo box or edit");

        calendar.SetSelectedDate({ 2028, 2, 29 });
        Check(calendar.Selected() == CalendarDate{ 2028, 2, 29 } && calendar.Focused() == CalendarDate{ 2028, 2, 29 } && host.log.empty(), "programmatic selection is silent");
        CalendarGrid& gridWindow = calendar.Grid();
        const HWND gw = gridWindow.GetSafeHwnd();
        Key(gw, VK_RIGHT);
        Check(calendar.Focused() == CalendarDate{ 2028, 3, 1 } && calendar.VisibleMonth() == 3 && calendar.Selected() == CalendarDate{ 2028, 2, 29 } && host.Count(700, CalendarMonthChanged) == 1,
            "Right at the end of February moves to March 1 and changes the shown month");
        Check(calendar.MonthDropdown().GetCurSel() == 2, "the month dropdown follows the keyboard");
        Key(gw, VK_LEFT);
        Check(calendar.Focused() == CalendarDate{ 2028, 2, 29 } && calendar.VisibleMonth() == 2 && host.Count(700, CalendarMonthChanged) == 2, "Left from March 1 returns to the leap day");
        Key(gw, VK_DOWN);
        Check(calendar.Focused() == CalendarDate{ 2028, 3, 7 }, "Down moves one week across the month boundary");
        Key(gw, VK_UP);
        Check(calendar.Focused() == CalendarDate{ 2028, 2, 29 }, "Up moves one week back");
        calendar.SetSelectedDate({ 2028, 1, 31 });
        Key(gw, VK_NEXT);
        Check(calendar.Focused() == CalendarDate{ 2028, 2, 29 } && calendar.VisibleMonth() == 2, "PageDown from January 31 clamps to the leap day");
        Key(gw, VK_PRIOR);
        Check(calendar.Focused() == CalendarDate{ 2028, 1, 29 } && calendar.VisibleMonth() == 1, "PageUp goes back one month");
        calendar.SetSelectedDate({ 2028, 2, 29 });
        Key(gw, VK_HOME);
        Check(calendar.Focused() == CalendarDate{ 2028, 2, 27 }, "Home goes to the start of the week (Sunday first)");
        Key(gw, VK_END);
        Check(calendar.Focused() == CalendarDate{ 2028, 3, 4 } && calendar.VisibleMonth() == 3, "End goes to the end of the week and may cross into March");
        Check(calendar.Navigate(CalendarMove::Today) && calendar.Focused() == CalendarDate{ 2028, 2, 14 } && calendar.VisibleMonth() == 2, "Today returns to the today marker");
        Check(calendar.Navigate(CalendarMove::PreviousYear) && calendar.Focused() == CalendarDate{ 2027, 2, 14 } && calendar.VisibleYear() == 2027, "Previous year keeps the month and day");
        Check(calendar.Navigate(CalendarMove::NextYear) && calendar.Focused() == CalendarDate{ 2028, 2, 14 }, "Next year returns");
        calendar.SetSelectedDate({ 2020, 1, 1 });
        Check(!calendar.Navigate(CalendarMove::PreviousDay) && calendar.Focused() == CalendarDate{ 2020, 1, 1 }, "navigation stops at the first allowed year");
        Check(!calendar.PreviousButton().IsWindowEnabled() && calendar.NextButton().IsWindowEnabled(), "the previous button is disabled at the start of the range");
        calendar.SetSelectedDate({ 2028, 2, 29 });

        host.log.clear();
        Key(gw, VK_SPACE);
        Check(calendar.Selected() == CalendarDate{ 2028, 2, 29 } && host.Count(700, CalendarDateChanged) == 0, "Space on the already selected day does not notify");
        Key(gw, VK_LEFT);
        Key(gw, VK_SPACE);
        Check(calendar.Selected() == CalendarDate{ 2028, 2, 28 } && host.Count(700, CalendarDateChanged) == 1 && host.Count(700, CalendarDateActivated) == 0, "Space selects the focused day and notifies once");
        Key(gw, VK_RETURN);
        Check(host.Count(700, CalendarDateActivated) == 1 && host.Count(700, CalendarDateChanged) == 1, "Enter activates the focused day");

        const CRect tenth = calendar.DayRect({ 2028, 2, 10 });
        Check(!tenth.IsRectEmpty() && tenth.Height() >= Px(gw, 48) - 1 && tenth.Width() >= Px(gw, 40), "a day cell is at least 48 DIPs tall");
        Check(calendar.DateAt(tenth.CenterPoint()) == CalendarDate{ 2028, 2, 10 } && !calendar.DateAt(CPoint(2, 2)).Valid(), "hit testing maps points to days and ignores the weekday row");
        const CRect blank = calendar.DayRect({ 2028, 2, 1 });
        Check(!calendar.DateAt(CPoint(blank.left - 8, blank.CenterPoint().y)).Valid(), "a blank cell before the first day is not a date");
        host.log.clear();
        Click(gw, tenth.CenterPoint().x, tenth.CenterPoint().y);
        Check(calendar.Selected() == CalendarDate{ 2028, 2, 10 } && calendar.Focused() == CalendarDate{ 2028, 2, 10 } && host.Count(700, CalendarDateChanged) == 1 && host.Count(700, CalendarDateActivated) == 0,
            "clicking a day selects it and notifies once");
        Click(gw, tenth.CenterPoint().x, tenth.CenterPoint().y);
        Check(host.Count(700, CalendarDateActivated) == 1 && host.Count(700, CalendarDateChanged) == 1, "a quick second click activates the day");
        gridWindow.SendMessage(WM_MOUSEMOVE, 0, Point(tenth.CenterPoint().x, tenth.CenterPoint().y));
        Check(calendar.HoverDate() == CalendarDate{ 2028, 2, 10 }, "the hovered day is tracked for tooltips");
        gridWindow.SendMessage(WM_MOUSELEAVE, 0, 0);
        Check(!calendar.HoverDate().Valid(), "leaving the grid clears the hovered day");
        host.log.clear();
        gridWindow.SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), Point(100, 100));
        Check(calendar.VisibleMonth() == 3 && host.Count(700, CalendarMonthChanged) == 1, "the wheel changes the month");
        ClickCenter(calendar.PreviousButton());
        Check(calendar.VisibleMonth() == 2 && host.Count(700, CalendarMonthChanged) == 2, "the previous month button shows the previous month");
        ClickCenter(calendar.NextButton());
        ClickCenter(calendar.NextButton());
        Check(calendar.VisibleMonth() == 4 && host.Count(700, CalendarMonthChanged) == 4, "the next month button shows the next month");
        calendar.MonthDropdown().SetCurSel(5);
        calendar.SendMessage(WM_COMMAND, MAKEWPARAM(Calendar::IdMonth, CBN_SELCHANGE), reinterpret_cast<LPARAM>(calendar.MonthDropdown().GetSafeHwnd()));
        Check(calendar.VisibleMonth() == 6 && calendar.VisibleYear() == 2028, "choosing a month in the dropdown changes the shown month");
        calendar.YearDropdown().SetCurSel(2);
        calendar.SendMessage(WM_COMMAND, MAKEWPARAM(Calendar::IdYear, CBN_SELCHANGE), reinterpret_cast<LPARAM>(calendar.YearDropdown().GetSafeHwnd()));
        Check(calendar.VisibleYear() == 2022 && calendar.VisibleMonth() == 6 && calendar.YearDropdown().GetCurText() == L"2022", "choosing a year in the dropdown changes the shown year");
        calendar.SetSelectedDate({ 2028, 2, 29 });

        calendar.SetFirstDayOfWeek(1);
        Check(calendar.Month().leading == 1 && calendar.PaintModel().weekdays[0] == L"M" && calendar.PaintModel().weekdays[6] == L"S", "Monday first reorders the weekday row");
        calendar.SetFirstDayOfWeek(0);
        Check(calendar.PaintModel().weekdays[0] == L"S" && calendar.Month().leading == 2, "Sunday first restores the weekday row");

        {
            ComPtr<IRawElementProviderSimple> root;
            Check(SUCCEEDED(calendar.CalendarAutomation(root.GetAddressOf())) && root, "the calendar exposes an automation root");
            auto grid = Pattern<IGridProvider>(root.Get(), UIA_GridPatternId);
            int rows = 0, columns = 0;
            Check(grid && SUCCEEDED(grid->get_RowCount(&rows)) && SUCCEEDED(grid->get_ColumnCount(&columns)) && rows == 6 && columns == 7 && ControlType(root.Get()) == UIA_CalendarControlTypeId
                && Name(root.Get()) == L"Traffic history", "the calendar root has the Grid pattern with 6 rows and 7 columns");
            ComPtr<IRawElementProviderSimple> first, blankCell, tenthCell, leapCell;
            Check(grid && SUCCEEDED(grid->GetItem(0, 2, first.GetAddressOf())) && Contains(Name(first.Get()), L"Tuesday, 1 February 2028"), "Grid.GetItem names a day with its weekday and full date");
            Check(grid && SUCCEEDED(grid->GetItem(0, 0, blankCell.GetAddressOf())) && Name(blankCell.Get()).IsEmpty() && !Enabled(blankCell.Get()), "a blank grid cell has no name and is not enabled");
            Check(grid && FAILED(grid->GetItem(6, 0, blankCell.ReleaseAndGetAddressOf())) && FAILED(grid->GetItem(0, 7, blankCell.ReleaseAndGetAddressOf())), "Grid.GetItem rejects coordinates outside the grid");
            const CalendarMonthLayout layout = calendar.Month();
            int row = 0, column = 0;
            layout.Find(10, row, column);
            Check(grid && SUCCEEDED(grid->GetItem(row, column, tenthCell.GetAddressOf())) && Contains(Name(tenthCell.Get()), L"10 February 2028") && Contains(Name(tenthCell.Get()), L"Traffic 1.5 GB"),
                "a day's accessible name includes the hook description");
            auto item = Pattern<IGridItemProvider>(tenthCell.Get(), UIA_GridItemPatternId);
            int itemRow = -1, itemColumn = -1;
            Check(item && SUCCEEDED(item->get_Row(&itemRow)) && SUCCEEDED(item->get_Column(&itemColumn)) && itemRow == row && itemColumn == column && ControlType(tenthCell.Get()) == UIA_DataItemControlTypeId,
                "a day has the GridItem pattern with its row and column");
            layout.Find(29, row, column);
            grid->GetItem(row, column, leapCell.GetAddressOf());
            auto leapItem = Pattern<ISelectionItemProvider>(leapCell.Get(), UIA_SelectionItemPatternId);
            BOOL isSelected = FALSE;
            Check(leapItem && SUCCEEDED(leapItem->get_IsSelected(&isSelected)) && isSelected, "the selected day reports SelectionItem.IsSelected");
            auto selection = Pattern<ISelectionProvider>(root.Get(), UIA_SelectionPatternId);
            SAFEARRAY* chosen = nullptr;
            const bool one = selection && SUCCEEDED(selection->GetSelection(&chosen)) && chosen && chosen->rgsabound[0].cElements == 1;
            if (chosen) SafeArrayDestroy(chosen);
            Check(one, "the calendar root reports exactly one selected day");
            host.log.clear();
            auto tenthSelect = Pattern<ISelectionItemProvider>(tenthCell.Get(), UIA_SelectionItemPatternId);
            if (tenthSelect) tenthSelect->Select();
            Pump();
            Check(calendar.Selected() == CalendarDate{ 2028, 2, 10 } && host.Count(700, CalendarDateChanged) == 1, "automation Select chooses the day and notifies");
            ComPtr<IRawElementProviderSimple> blank00;
            grid->GetItem(0, 0, blank00.GetAddressOf());
            auto blankItem = Pattern<ISelectionItemProvider>(blank00.Get(), UIA_SelectionItemPatternId);
            Check(blankItem && blankItem->Select() == UIA_E_ELEMENTNOTENABLED, "selecting a blank cell is refused");
        }

        // Offscreen renders of the grid: light and dark, 96 and 144 dpi; the selection and the indicator are visible.
        calendar.SetSelectedDate({ 2028, 2, 29 });
        calendar.SetWeekendHighlight(true);
        const CalendarPaintModel model = calendar.PaintModel();
        CalendarPaintModel plain = model;
        plain.selectedDay = 0;
        plain.contents.assign(32, CalendarDayContent{});
        plain.todayDay = 0;
        bool drawn = true;
        size_t themeDifference = 0;
        for (const UINT dpi : { 96u, 144u })
        {
            const auto light = Render(dpi, 308, 320, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawCalendarGrid(t, p, model, InputState{ true, false, false, true, false }); });
            const auto dark = Render(dpi, 308, 320, Appearance::Dark, [&](ID2D1RenderTarget* t, const Palette& p) { DrawCalendarGrid(t, p, model, InputState{ true, false, false, true, false }); });
            const auto bare = Render(dpi, 308, 320, Appearance::Light, [&](ID2D1RenderTarget* t, const Palette& p) { DrawCalendarGrid(t, p, plain, InputState{ true, false, false, true, false }); });
            if (Ink(light) < 1500 || Ink(dark) < 1500) drawn = false;
            themeDifference += Difference(light, dark);
            Check(Difference(light, bare) > 300, "the selected day, today marker and day content change the drawing");
        }
        Check(drawn && themeDifference > 10000, "the calendar grid renders non-blank in light and dark at 96 and 144 dpi");
        Check(Ink(PaintInput(gridWindow)) > 1500 && Ink(PaintInput(calendar.PreviousButton())) > 20 && Ink(PaintInput(calendar.MonthDropdown())) > 200, "the grid, the navigation button and the dropdown paint through their windows");
        calendar.SetAppearance(Appearance::Dark);
        Check(gridWindow.GetAppearance() == Appearance::Dark && calendar.MonthDropdown().GetAppearance() == Appearance::Dark && calendar.NextButton().GetAppearance() == Appearance::Dark, "the appearance reaches every part of the calendar");
        calendar.DestroyWindow();
    }

    host.DestroyWindow();
    SetPickerTestHooks(nullptr);
    SetThreadDesktop(original);
    CloseDesktop(desktop);
    std::printf("%s\n", failures ? "Picker verification FAILED" : "Picker verification done");
    return failures ? 1 : 0;
}
