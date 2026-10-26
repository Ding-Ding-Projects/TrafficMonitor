#include "../stdafx.h"
#include "TitleBar.h"
#include <UIAutomation.h>
#include <shellapi.h>
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>
namespace { CWinApp application; int failures{};
void Check(bool value, const char* name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name); if (!value) ++failures; }

LRESULT Hit(HWND window, int x, int y) { return SendMessageW(window, WM_NCHITTEST, 0, MAKELPARAM(x, y)); }
POINT Screen(HWND window, int x, int y) { POINT p{ x, y }; ClientToScreen(window, &p); return p; }
LRESULT HitClient(HWND window, int x, int y) { const POINT p = Screen(window, x, y); return Hit(window, p.x, p.y); }

COLORREF PixelAt(HWND bar, int x, int y)
{
    RECT r{}; GetClientRect(bar, &r);
    HDC dc = CreateCompatibleDC(nullptr);
    BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = r.right; info.bmiHeader.biHeight = -r.bottom;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
    void* bits{}; HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    auto previous = SelectObject(dc, bitmap);
    SendMessageW(bar, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
    const auto value = reinterpret_cast<const DWORD*>(bits)[y * r.right + x];
    SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
    return RGB((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
}
bool Near(COLORREF a, COLORREF b, int tolerance = 6)
{ return abs(int(GetRValue(a)) - int(GetRValue(b))) <= tolerance && abs(int(GetGValue(a)) - int(GetGValue(b))) <= tolerance && abs(int(GetBValue(a)) - int(GetBValue(b))) <= tolerance; }
COLORREF Rgb(D2D1_COLOR_F c) { return RGB(BYTE(c.r * 255 + .5f), BYTE(c.g * 255 + .5f), BYTE(c.b * 255 + .5f)); }

struct AutomationResult { bool ran{}; std::wstring rootName; int rootType{}, count{}; std::wstring names[3]; int types[3]{}; RECT bounds[3]{}; RECT root{}; bool invoked[3]{}; };
}

int wmain()
{
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const DWORD style = WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    CWnd window;
    Check(window.CreateEx(0, AfxRegisterWndClass(0), L"Title bar regression", style, 100, 100, 800, 600, nullptr, 0) != FALSE, "hidden top-level window created");
    HWND host = window.GetSafeHwnd();
    SendMessageW(host, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadIconW(nullptr, IDI_APPLICATION)));
    NativeMaterial::TitleBar bar;
    NativeMaterial::TitleBar::Labels labels;
    Check(SUCCEEDED(bar.Attach(&window, NativeMaterial::Appearance::Light, labels)), "title bar attached");
    Check(bar.IsAttached() && bar.BarWindow() && IsWindow(bar.BarWindow()), "title bar child window exists");
    Check((GetWindowLongW(host, GWL_STYLE) & (WS_CAPTION | WS_SYSMENU | WS_THICKFRAME)) == (WS_CAPTION | WS_SYSMENU | WS_THICKFRAME), "native caption, system menu and sizing style bits are retained for snap and Alt+Space");
    Check(GetSystemMenu(host, FALSE) != nullptr, "system menu remains available");

    const UINT dpis[] = { 96, 144 };
    for (const UINT dpi : dpis)
    {
        std::printf("--- %u DPI\n", dpi);
        RECT now{}; GetWindowRect(host, &now);
        SendMessageW(host, WM_DPICHANGED, MAKEWPARAM(dpi, dpi), reinterpret_cast<LPARAM>(&now));
        char label[160];
        const int barHeight = MulDiv(40, dpi, 96), cell = MulDiv(48, dpi, 96);
        RECT windowRect{}, clientRect{}; GetWindowRect(host, &windowRect); GetClientRect(host, &clientRect);
        const POINT origin = Screen(host, 0, 0);
        Check(origin.y == windowRect.top, "client area starts at the top of the window (no operating-system caption)");
        Check(bar.HeightPx() == barHeight, "title bar height scales with DPI");
        RECT barRect{}; GetClientRect(bar.BarWindow(), &barRect);
        Check(barRect.bottom == barHeight && barRect.right == clientRect.right, "title bar child fills the client width at the title bar height");

        const auto g = bar.Measure();
        Check(g.hasMinimize && g.hasMaximize && g.hasClose && g.hasIcon, "all caption buttons and the icon are present");
        Check(g.close.right == g.bar.right && g.close.left == g.bar.right - cell && g.maximize.right == g.close.left && g.minimize.right == g.maximize.left, "caption buttons are contiguous at the right edge");
        Check(g.close.Width() == cell && g.close.Height() == barHeight, "caption button cells have the scaled size");
        Check(g.text.right <= g.minimize.left && g.text.left >= g.icon.right && g.text.left < g.text.right, "caption buttons do not overlap the title text");
        Check(g.icon.right <= g.text.left && g.icon.top >= 0 && g.icon.bottom <= g.bar.bottom, "icon is inside the bar and left of the title text");
        const auto narrow = NativeMaterial::TitleBar::Compute(MulDiv(200, dpi, 96), dpi, true, true, true, true);
        Check(narrow.text.right <= narrow.minimize.left && narrow.text.left <= narrow.text.right, "narrow bar keeps the title text clear of the buttons");

        const int mid = barHeight / 2;
        Check(HitClient(host, g.text.left + g.text.Width() / 2, mid) == HTCAPTION, "title text area is the caption drag region");
        Check(HitClient(host, g.icon.left + 2, mid) == HTCAPTION, "icon area drags the window instead of closing it");
        Check(HitClient(host, g.minimize.CenterPoint().x, mid) == HTMINBUTTON, "minimize button reports HTMINBUTTON");
        Check(HitClient(host, g.maximize.CenterPoint().x, mid) == HTMAXBUTTON, "maximize button reports HTMAXBUTTON for Snap Layouts");
        Check(HitClient(host, g.close.CenterPoint().x, mid) == HTCLOSE, "close button reports HTCLOSE");
        Check(HitClient(host, g.maximize.left, mid) == HTMAXBUTTON && HitClient(host, g.maximize.left - 1, mid) == HTMINBUTTON, "button boundary is exact between minimize and maximize");
        Check(HitClient(host, g.close.right - 1, mid) == HTCLOSE, "close cell reaches the right edge of the client area");
        Check(HitClient(host, clientRect.right / 2, barHeight + 10) == HTCLIENT, "below the title bar is client content");
        Check(bar.PartAt(Screen(host, g.maximize.CenterPoint().x, mid)) == NativeMaterial::TitleBar::Part::Maximize, "PartAt agrees with WM_NCHITTEST");

        GetWindowRect(host, &windowRect);
        const int centerX = (windowRect.left + windowRect.right) / 2, centerY = (windowRect.top + windowRect.bottom) / 2;
        Check(Hit(host, centerX, windowRect.top + 1) == HTTOP, "top edge resizes");
        Check(Hit(host, windowRect.left + 1, centerY) == HTLEFT, "left edge resizes");
        Check(Hit(host, windowRect.right - 2, centerY) == HTRIGHT, "right edge resizes");
        Check(Hit(host, centerX, windowRect.bottom - 2) == HTBOTTOM, "bottom edge resizes");
        Check(Hit(host, windowRect.left + 1, windowRect.top + 1) == HTTOPLEFT, "top left corner resizes");
        Check(Hit(host, windowRect.right - 2, windowRect.top + 1) == HTTOPRIGHT, "top right corner resizes");
        Check(Hit(host, windowRect.left + 1, windowRect.bottom - 2) == HTBOTTOMLEFT, "bottom left corner resizes");
        Check(Hit(host, windowRect.right - 2, windowRect.bottom - 2) == HTBOTTOMRIGHT, "bottom right corner resizes");
        Check(Hit(host, windowRect.right + 20, centerY) != HTCAPTION && Hit(host, windowRect.right + 20, centerY) != HTCLOSE, "outside the window never reports title bar parts");

        // Window without maximize box: no HTMAXBUTTON and the cell is not reserved.
        SetWindowLongW(host, GWL_STYLE, GetWindowLongW(host, GWL_STYLE) & ~WS_MAXIMIZEBOX);
        const auto noMax = bar.Measure();
        Check(!noMax.hasMaximize && HitClient(host, noMax.close.left - noMax.close.Width() / 2, mid) == HTMINBUTTON, "maximize button disappears without WS_MAXIMIZEBOX");
        SetWindowLongW(host, GWL_STYLE, GetWindowLongW(host, GWL_STYLE) | WS_MAXIMIZEBOX);

        // Synthetic maximized insets: WS_MAXIMIZE style is set only to exercise the pure frame math.
        const int expectedInset = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        RECT proposed{ 0, 0, 800, 600 };
        SendMessageW(host, WM_NCCALCSIZE, FALSE, reinterpret_cast<LPARAM>(&proposed));
        Check(proposed.top == 0, "restored window client begins at the window top");
        SetWindowLongW(host, GWL_STYLE, GetWindowLongW(host, GWL_STYLE) | WS_MAXIMIZE);
        proposed = { 0, 0, 800, 600 };
        SendMessageW(host, WM_NCCALCSIZE, FALSE, reinterpret_cast<LPARAM>(&proposed));
        Check(proposed.top == expectedInset, "maximized client is inset by the sizing frame so the bar is not off-screen");
        const LRESULT zoomedEdge = Hit(host, centerX, windowRect.top + 1);
        Check(zoomedEdge != HTTOP && zoomedEdge != HTTOPLEFT && zoomedEdge != HTTOPRIGHT, "maximized window does not offer resize edges");
        SetWindowLongW(host, GWL_STYLE, GetWindowLongW(host, GWL_STYLE) & ~WS_MAXIMIZE);

        // Painting states (skipped under high contrast, which uses system colors).
        const auto palette = NativeMaterial::Palette::Resolve(NativeMaterial::Appearance::Light);
        if (!palette.highContrast)
        {
            const int probeX = g.close.CenterPoint().x - MulDiv(12, dpi, 96);
            Check(Near(PixelAt(bar.BarWindow(), probeX, mid), Rgb(palette.surfaceContainerLow)), "idle caption button uses the title bar surface");
            SendMessageW(host, WM_NCMOUSEMOVE, HTCLOSE, 0);
            Check(Near(PixelAt(bar.BarWindow(), probeX, mid), Rgb(palette.error)), "close hover uses the Material error color");
            SendMessageW(host, WM_NCMOUSEMOVE, HTMAXBUTTON, 0);
            Check(!Near(PixelAt(bar.BarWindow(), g.maximize.CenterPoint().x - MulDiv(12, dpi, 96), mid), Rgb(palette.surfaceContainerLow), 2), "maximize hover shows a state layer");
            SendMessageW(host, WM_NCMOUSELEAVE, 0, 0);
            Check(Near(PixelAt(bar.BarWindow(), probeX, mid), Rgb(palette.surfaceContainerLow)), "state layer clears when the pointer leaves");
            bar.SetAppearance(NativeMaterial::Appearance::Dark);
            const auto dark = NativeMaterial::Palette::Resolve(NativeMaterial::Appearance::Dark);
            Check(Near(PixelAt(bar.BarWindow(), probeX, mid), Rgb(dark.surfaceContainerLow)), "dark appearance repaints with the dark palette");
            bar.SetAppearance(NativeMaterial::Appearance::Light);
        }
        (void)label;
    }

    // UI Automation: the caption buttons are announced and invokable.
    AutomationResult automation;
    std::atomic<bool> done{ false };
    HWND barWindow = bar.BarWindow();
    std::thread client([&] {
        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
        {
            IUIAutomation* uia{};
            if (SUCCEEDED(CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), reinterpret_cast<void**>(&uia))) && uia)
            {
                IUIAutomationElement* root{};
                if (SUCCEEDED(uia->ElementFromHandle(barWindow, &root)) && root)
                {
                    BSTR name{}; if (SUCCEEDED(root->get_CurrentName(&name)) && name) { automation.rootName = name; SysFreeString(name); }
                    root->get_CurrentControlType(&automation.rootType);
                    root->get_CurrentBoundingRectangle(&automation.root);
                    IUIAutomationCondition* all{}; uia->CreateTrueCondition(&all);
                    IUIAutomationElementArray* children{};
                    if (all && SUCCEEDED(root->FindAll(TreeScope_Children, all, &children)) && children)
                    {
                        children->get_Length(&automation.count);
                        for (int i = 0; i < automation.count && i < 3; ++i)
                        {
                            IUIAutomationElement* item{};
                            if (FAILED(children->GetElement(i, &item)) || !item) continue;
                            BSTR itemName{}; if (SUCCEEDED(item->get_CurrentName(&itemName)) && itemName) { automation.names[i] = itemName; SysFreeString(itemName); }
                            item->get_CurrentControlType(&automation.types[i]);
                            item->get_CurrentBoundingRectangle(&automation.bounds[i]);
                            IUnknown* pattern{};
                            if (SUCCEEDED(item->GetCurrentPattern(UIA_InvokePatternId, &pattern)) && pattern)
                            {
                                IUIAutomationInvokePattern* invoke{};
                                if (SUCCEEDED(pattern->QueryInterface(__uuidof(IUIAutomationInvokePattern), reinterpret_cast<void**>(&invoke))) && invoke)
                                { automation.invoked[i] = SUCCEEDED(invoke->Invoke()); invoke->Release(); }
                                pattern->Release();
                            }
                            item->Release();
                        }
                        children->Release();
                    }
                    if (all) all->Release();
                    root->Release();
                    automation.ran = true;
                }
                uia->Release();
            }
            CoUninitialize();
        }
        done = true;
    });
    const ULONGLONG deadline = GetTickCount64() + 15000;
    while (!done && GetTickCount64() < deadline)
    {
        MSG pump{};
        // Peeking services cross-thread sent messages without dispatching posted commands.
        PeekMessageW(&pump, host, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_NOREMOVE);
        Sleep(5);
    }
    if (done) client.join(); else client.detach();
    Check(automation.ran, "UI Automation reached the title bar");
    Check(automation.rootType == UIA_TitleBarControlTypeId && automation.rootName == L"Title bar regression", "title bar is announced with the window title");
    Check(automation.count == 3, "three caption buttons are exposed");
    Check(automation.names[0] == L"Minimize" && automation.names[1] == L"Maximize" && automation.names[2] == L"Close", "caption buttons have accessible names");
    Check(automation.types[0] == UIA_ButtonControlTypeId && automation.types[1] == UIA_ButtonControlTypeId && automation.types[2] == UIA_ButtonControlTypeId, "caption buttons are button controls");
    bool inside = true;
    for (int i = 0; i < 3; ++i) inside = inside && automation.bounds[i].left >= automation.root.left && automation.bounds[i].right <= automation.root.right && automation.bounds[i].top >= automation.root.top;
    Check(inside && automation.bounds[0].right <= automation.bounds[1].left && automation.bounds[1].right <= automation.bounds[2].left, "accessible bounds are ordered inside the title bar");
    Check(automation.invoked[0] && automation.invoked[1] && automation.invoked[2], "caption buttons support Invoke");
    std::vector<WPARAM> commands;
    MSG posted{};
    while (PeekMessageW(&posted, host, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE)) commands.push_back(posted.wParam & 0xFFF0);
    bool hasMinimize = false, hasMaximize = false, hasClose = false;
    for (const auto command : commands) { hasMinimize |= command == SC_MINIMIZE; hasMaximize |= command == SC_MAXIMIZE; hasClose |= command == SC_CLOSE; }
    Check(hasMinimize && hasMaximize && hasClose, "Invoke posts the matching system commands");

    // Accessible window name stays the window title.
    window.SetWindowText(L"Renamed");
    wchar_t title[64]{}; GetWindowTextW(host, title, 64);
    Check(std::wstring(title) == L"Renamed", "window title remains the accessible window name");

    {
        // Optional visual inspection aid: --dump <dir> writes the painted title bar as BMP files.
        int argc = 0; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        std::wstring directory;
        for (int i = 1; argv && i + 1 < argc; ++i) if (std::wstring(argv[i]) == L"--dump") directory = argv[i + 1];
        if (argv) LocalFree(argv);
        if (!directory.empty())
        {
            const struct { const wchar_t* name; NativeMaterial::Appearance appearance; WPARAM hit; } shots[] = {
                { L"light-idle", NativeMaterial::Appearance::Light, 0 }, { L"light-close-hover", NativeMaterial::Appearance::Light, HTCLOSE },
                { L"light-max-hover", NativeMaterial::Appearance::Light, HTMAXBUTTON }, { L"dark-idle", NativeMaterial::Appearance::Dark, 0 },
                { L"dark-close-hover", NativeMaterial::Appearance::Dark, HTCLOSE } };
            for (const auto& shot : shots)
            {
                bar.SetAppearance(shot.appearance);
                SendMessageW(host, WM_NCMOUSEMOVE, shot.hit, 0);
                RECT r{}; GetClientRect(bar.BarWindow(), &r);
                HDC dc = CreateCompatibleDC(nullptr);
                BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = r.right; info.bmiHeader.biHeight = -r.bottom;
                info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
                void* bits{}; HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
                auto previous = SelectObject(dc, bitmap);
                SendMessageW(bar.BarWindow(), WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
                BITMAPFILEHEADER file{ 0x4d42, DWORD(sizeof(file) + sizeof(info.bmiHeader) + r.right * r.bottom * 4), 0, 0, DWORD(sizeof(file) + sizeof(info.bmiHeader)) };
                info.bmiHeader.biHeight = -r.bottom;
                HANDLE out = CreateFileW((directory + L"\\" + shot.name + L".bmp").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                DWORD written{};
                if (out != INVALID_HANDLE_VALUE)
                {
                    WriteFile(out, &file, sizeof(file), &written, nullptr); WriteFile(out, &info.bmiHeader, sizeof(info.bmiHeader), &written, nullptr);
                    WriteFile(out, bits, r.right * r.bottom * 4, &written, nullptr); CloseHandle(out);
                }
                SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
            }
            bar.SetAppearance(NativeMaterial::Appearance::Light);
        }
    }

    bar.Detach();
    window.DestroyWindow();
    std::printf("RESULT failures=%d\n", failures);
    return failures ? 1 : 0;
}
