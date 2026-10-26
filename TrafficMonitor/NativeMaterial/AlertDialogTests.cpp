#include "../stdafx.h"
#include "AlertDialog.h"
#include "../NativePreferences/Text.h"
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

// The alert reads the shared appearance and language preference. This program links no preference
// storage, so it supplies the snapshot the dialog reads.
namespace NativePreferences
{
    namespace { Snapshot g_snapshot; }
    void SetTestSnapshot(bool dark, Language language) { g_snapshot.dark = dark; g_snapshot.values.language = language; }
    Snapshot Current() { return g_snapshot; }
}

namespace
{
    CWinApp application;
    int failures{};
    int systemDialogs{};
    void Check(bool value, const std::string& name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name.c_str()); if (!value) ++failures; }

    struct Step { UINT message; WPARAM wparam; LPARAM lparam; int button{ -1 }; };
    Step Key(WPARAM key, bool repeat = false) { return { WM_KEYDOWN, key, repeat ? 0x40000001 : 1 }; }

    struct Case
    {
        std::string name;
        UINT type{};
        std::wstring text{ L"Something happened." };
        std::wstring caption;
        bool noOwner{};
        std::vector<int> ids;
        int focusIndex{};
        NativeMaterial::AlertIcon icon{ NativeMaterial::AlertIcon::None };
        std::vector<Step> steps;
        int expect{};
        std::function<void(const NativeMaterial::AlertSnapshot&)> inspect;
        std::function<void(const NativeMaterial::AlertSnapshot&)> afterSteps;
    };
    Case* current{};
    HWND dialogWindow{};
    HWND ownerWindow{};
    bool ownerWasDisabled{};
    NativeMaterial::AlertSnapshot lateSnapshot;

    void CALLBACK AfterSteps(HWND window, UINT, UINT_PTR id, DWORD)
    {
        KillTimer(window, id);
        NativeMaterial::AlertSnapshot snapshot;
        if (NativeMaterial::InspectAlert(window, snapshot) && current && current->afterSteps) current->afterSteps(snapshot);
        PostMessage(window, WM_KEYDOWN, VK_ESCAPE, 1);
    }

    void Shown(HWND dialog, void*)
    {
        Case& c = *current;
        dialogWindow = dialog;
        NativeMaterial::AlertSnapshot snapshot;
        Check(NativeMaterial::InspectAlert(dialog, snapshot), c.name + ": dialog inspectable");
        Check(snapshot.results == c.ids, c.name + ": buttons match the MB_* set");
        Check(snapshot.focusedIndex == c.focusIndex, c.name + ": initial logical focus on the expected action");
        Check(snapshot.systemFocusIndex == c.focusIndex, c.name + ": initial keyboard focus owned by the expected action window");
        Check(snapshot.icon == c.icon, c.name + ": icon matches MB_ICON");
        Check(!snapshot.headline.IsEmpty(), c.name + ": headline present");
        Check(!c.noOwner ? !IsWindowEnabled(ownerWindow) : true, c.name + ": owner disabled while open");
        Check(snapshot.layout.size.cx > 0 && snapshot.layout.size.cy > 0, c.name + ": layout has a size");
        char buffer[64]{};
        GetClassNameA(dialog, buffer, 64);
        Check(std::string(buffer) != "#32770", c.name + ": dialog is not a system dialog class");
        if (c.inspect) c.inspect(snapshot);
        for (const Step& step : c.steps)
        {
            HWND target = step.button >= 0 ? snapshot.buttons[step.button] : dialog;
            PostMessage(target, step.message, step.wparam, step.lparam);
        }
        if (c.afterSteps) SetTimer(dialog, 77, 20, AfterSteps);
    }

    LRESULT CALLBACK Cbt(int code, WPARAM wparam, LPARAM lparam)
    {
        if (code == HCBT_CREATEWND)
        {
            const auto* create = reinterpret_cast<CBT_CREATEWND*>(lparam);
            const wchar_t* name = create->lpcs->lpszClass;
            if ((reinterpret_cast<ULONG_PTR>(name) == 0x8002) || (reinterpret_cast<ULONG_PTR>(name) > 0xFFFF && std::wstring(name) == L"#32770")) ++systemDialogs;
        }
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    void Run(Case& c, CButton& focusButton)
    {
        current = &c;
        dialogWindow = nullptr;
        focusButton.SetFocus();
        const int before = systemDialogs;
        const HWND hadFocus = ::GetFocus();
        const int result = NativeMaterial::Alert(c.noOwner ? nullptr : ownerWindow, CString(c.text.c_str()), CString(c.caption.c_str()), c.type);
        Check(result == c.expect, c.name + ": returns " + std::to_string(c.expect) + " (got " + std::to_string(result) + ")");
        Check(systemDialogs == before, c.name + ": no system message box window created");
        Check(!IsWindow(dialogWindow), c.name + ": dialog destroyed");
        if (!c.noOwner) Check(IsWindowEnabled(ownerWindow) != FALSE, c.name + ": owner re-enabled");
        Check(::GetFocus() == hadFocus, c.name + ": focus returns to the previously focused control");
    }

    void Sample(const std::function<void(HDC)>& paint, int width, int height, std::vector<COLORREF>& pixels)
    {
        HDC screen = GetDC(nullptr);
        HDC dc = CreateCompatibleDC(screen);
        HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
        HGDIOBJ old = SelectObject(dc, bitmap);
        paint(dc);
        pixels.assign(static_cast<size_t>(width) * height, 0);
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) pixels[static_cast<size_t>(y) * width + x] = GetPixel(dc, x, y);
        SelectObject(dc, old);
        DeleteObject(bitmap);
        DeleteDC(dc);
        ReleaseDC(nullptr, screen);
    }
}

int wmain()
{
    // Private desktop: the dialogs below are real top-level windows and must never reach the user's screen.
    HDESK original = GetThreadDesktop(GetCurrentThreadId());
    const std::wstring desktopName = L"TrafficMonitorAlertTest" + std::to_wstring(GetCurrentProcessId());
    HDESK desktop = CreateDesktopW(desktopName.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    Check(desktop != nullptr && SetThreadDesktop(desktop), "tests run on a private desktop");
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    SetWindowsHookExW(WH_CBT, Cbt, nullptr, GetCurrentThreadId());
    NativeMaterial::AlertTestHooks hooks;
    hooks.shown = Shown;
    NativeMaterial::SetAlertTestHooks(&hooks);

    CWnd owner;
    Check(owner.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 700, 500, nullptr, 0) != FALSE, "visible owner created");
    ownerWindow = owner.GetSafeHwnd();
    CButton focusButton;
    focusButton.Create(L"Before", WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(10, 10, 110, 40), &owner, 900);
    owner.SetActiveWindow();

    using namespace NativeMaterial;
    const int OK = IDOK, CANCEL = IDCANCEL, YES = IDYES, NO = IDNO, RETRY = IDRETRY;
    std::vector<Case> cases;
    auto add = [&](Case c) { cases.push_back(std::move(c)); };
    add({ "MB_OK Enter", MB_OK | MB_ICONINFORMATION, L"Done.", L"", false, { OK }, 0, AlertIcon::Information, { Key(VK_RETURN) }, IDOK });
    add({ "MB_OK Esc", MB_OK | MB_ICONERROR, L"Failed.", L"", false, { OK }, 0, AlertIcon::Error, { Key(VK_ESCAPE) }, IDOK });
    add({ "MB_OKCANCEL Enter", MB_OKCANCEL | MB_ICONWARNING, L"Sure?", L"", false, { OK, CANCEL }, 0, AlertIcon::Warning, { Key(VK_RETURN) }, IDOK });
    add({ "MB_OKCANCEL Esc", MB_OKCANCEL, L"Sure?", L"", false, { OK, CANCEL }, 0, AlertIcon::None, { Key(VK_ESCAPE) }, IDCANCEL });
    add({ "MB_OKCANCEL|MB_DEFBUTTON2 focuses Cancel", MB_OKCANCEL | MB_DEFBUTTON2, L"Sure?", L"", false, { OK, CANCEL }, 1, AlertIcon::None, { Key(VK_RETURN) }, IDCANCEL });
    add({ "MB_YESNO Esc returns No", MB_YESNO | MB_ICONQUESTION, L"Save?", L"", false, { YES, NO }, 0, AlertIcon::Question, { Key(VK_ESCAPE) }, IDNO });
    add({ "MB_YESNO|MB_DEFBUTTON2 focuses No", MB_YESNO | MB_DEFBUTTON2, L"Save?", L"", false, { YES, NO }, 1, AlertIcon::None, { Key(VK_RETURN) }, IDNO });
    add({ "MB_YESNO close returns No", MB_YESNO, L"Save?", L"", false, { YES, NO }, 0, AlertIcon::None, { { WM_CLOSE, 0, 0 } }, IDNO });
    add({ "MB_YESNOCANCEL Tab then Enter", MB_YESNOCANCEL, L"Save?", L"", false, { YES, NO, CANCEL }, 0, AlertIcon::None, { Key(VK_TAB), Key(VK_RETURN) }, IDNO });
    add({ "MB_YESNOCANCEL Tab wraps", MB_YESNOCANCEL, L"Save?", L"", false, { YES, NO, CANCEL }, 0, AlertIcon::None, { Key(VK_TAB), Key(VK_TAB), Key(VK_TAB), Key(VK_RETURN) }, IDYES });
    add({ "MB_YESNOCANCEL backwards wraps to Cancel", MB_YESNOCANCEL, L"Save?", L"", false, { YES, NO, CANCEL }, 0, AlertIcon::None, { Key(VK_LEFT), Key(VK_RETURN) }, IDCANCEL });
    add({ "MB_YESNOCANCEL|MB_DEFBUTTON3 focuses Cancel", MB_YESNOCANCEL | MB_DEFBUTTON3, L"Save?", L"", false, { YES, NO, CANCEL }, 2, AlertIcon::None, { Key(VK_RETURN) }, IDCANCEL });
    add({ "MB_YESNOCANCEL Esc returns Cancel", MB_YESNOCANCEL, L"Save?", L"", false, { YES, NO, CANCEL }, 0, AlertIcon::None, { Key(VK_ESCAPE) }, IDCANCEL });
    add({ "MB_RETRYCANCEL Enter", MB_RETRYCANCEL | MB_ICONERROR, L"Again?", L"", false, { RETRY, CANCEL }, 0, AlertIcon::Error, { Key(VK_RETURN) }, IDRETRY });
    add({ "MB_RETRYCANCEL Esc", MB_RETRYCANCEL, L"Again?", L"", false, { RETRY, CANCEL }, 0, AlertIcon::None, { Key(VK_ESCAPE) }, IDCANCEL });
    add({ "held Enter autorepeat does not dismiss", MB_OKCANCEL, L"Sure?", L"", false, { OK, CANCEL }, 0, AlertIcon::None, { Key(VK_RETURN, true), Key(VK_ESCAPE) }, IDCANCEL });
    {
        Case click{ "button click returns its action", MB_YESNO, L"Save?", L"", false, { YES, NO }, 0, AlertIcon::None, { { BM_CLICK, 0, 0, 1 } }, IDNO };
        add(click);
    }
    add({ "no owner", MB_OKCANCEL, L"Startup failed.", L"TrafficMonitor", true, { OK, CANCEL }, 0, AlertIcon::None, { Key(VK_ESCAPE) }, IDCANCEL,
        [](const AlertSnapshot& s)
        {
            HMONITOR monitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO info{ sizeof(info) };
            GetMonitorInfoW(monitor, &info);
            Check(s.layout.size.cx <= info.rcWork.right - info.rcWork.left, "no owner: dialog fits the primary monitor");
        } });
    add({ "long text wraps inside the dialog width", MB_OK | MB_ICONWARNING,
        std::wstring(L"The monitoring workspace could not be created because the stored layout is damaged. ") +
        L"Existing monitoring remains active and no settings were changed. " +
        L"https://example.com/a/very/long/unbroken/address/that/has/no/spaces/at/all/and/keeps/going/and/going/until/it/must/be/broken/by/the/wrapper",
        L"Long message", false, { OK }, 0, AlertIcon::Warning, { Key(VK_RETURN) }, IDOK,
        [](const AlertSnapshot& s)
        {
            Check(s.layout.bodyLines > 2, "long text: wraps onto several lines");
            Check(s.layout.size.cx <= MulDiv(560, s.dpi, 96), "long text: dialog stays within 560 dp");
            Check(s.layout.bodyTextWidth <= s.layout.bodyView.right - s.layout.bodyView.left, "long text: no line wider than the text area");
            Check(!s.layout.scrolls, "long text: moderate text does not scroll");
            Check(s.layout.bodyView.bottom - s.layout.bodyView.top >= s.layout.bodyContentHeight, "long text: whole text visible");
        } });
    {
        std::wstring huge;
        for (int i = 0; i < 300; ++i) huge += L"Line " + std::to_wstring(i) + L" of a very long report that must scroll.\n";
        Case scroll{ "very long text scrolls", MB_OK, huge, L"Report", false, { OK }, 0, AlertIcon::None, { Key(VK_END) }, IDOK };
        scroll.inspect = [](const AlertSnapshot& s)
        {
            MONITORINFO info{ sizeof(info) };
            GetMonitorInfoW(MonitorFromWindow(ownerWindow, MONITOR_DEFAULTTONEAREST), &info);
            Check(s.layout.scrolls, "very long text: scrolls");
            Check(s.layout.size.cy <= info.rcWork.bottom - info.rcWork.top, "very long text: dialog fits the work area");
            Check(s.layout.bodyView.bottom - s.layout.bodyView.top < s.layout.bodyContentHeight, "very long text: viewport smaller than content");
        };
        scroll.afterSteps = [](const AlertSnapshot& s)
        {
            Check(s.scroll == s.layout.bodyContentHeight - (s.layout.bodyView.bottom - s.layout.bodyView.top), "very long text: End key scrolls to the last line");
        };
        add(scroll);
    }
    add({ "forced GDI fallback still shows the same dialog", MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, L"Save?", L"", false, { YES, NO }, 1, AlertIcon::Question, { Key(VK_RETURN) }, IDNO,
        [](const AlertSnapshot& s) { Check(s.gdi, "GDI fallback: selected and reported"); } });

    for (Case& c : cases)
    {
        const bool gdi = c.name.rfind("forced GDI", 0) == 0;
        hooks.forceGdi = gdi;
        Run(c, focusButton);
    }
    hooks.forceGdi = false;

    // Destructive confirmation: the safest action is focused and the destructive action uses the error color variant.
    {
        AlertSpec spec;
        spec.owner = ownerWindow;
        spec.headline = L"Delete profile?";
        spec.body = L"This cannot be undone.";
        spec.icon = AlertIcon::Warning;
        spec.actions = { { 1001, L"Delete", true }, { IDCANCEL, L"Cancel", false } };
        spec.defaultIndex = 1;
        spec.dismissResult = IDCANCEL;
        Case c{ "destructive confirmation", 0, L"", L"", false, { 1001, IDCANCEL }, 1, AlertIcon::Warning, { Key(VK_RETURN) }, IDCANCEL };
        c.inspect = [](const AlertSnapshot& s)
        {
            Check(s.variants[0] == static_cast<int>(ButtonVariant::Destructive), "destructive: action uses the error variant");
            Check(s.variants[1] == static_cast<int>(ButtonVariant::Primary), "destructive: default action is filled");
        };
        current = &c;
        focusButton.SetFocus();
        const int result = ShowAlert(spec);
        Check(result == IDCANCEL, "destructive: Enter on the safest action returns Cancel");
        current = nullptr;
    }

    // Light and dark surfaces render different colors and the text is actually drawn.
    {
        COLORREF surface[2]{};
        for (int dark = 0; dark < 2; ++dark)
        {
            NativePreferences::SetTestSnapshot(dark != 0, NativePreferences::Language::English);
            Case c{ dark ? "dark render" : "light render", MB_OK | MB_ICONINFORMATION, L"Rendered text sample", L"Headline", false, { OK }, 0, AlertIcon::Information, { Key(VK_RETURN) }, IDOK };
            c.inspect = [&surface, dark](const AlertSnapshot& s)
            {
                const int width = s.layout.size.cx, height = s.layout.size.cy;
                std::vector<COLORREF> pixels;
                Sample([&](HDC dc) { SendMessage(dialogWindow, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT); }, width, height, pixels);
                surface[dark] = pixels[static_cast<size_t>(height / 2) * width + 3];
                const COLORREF background = surface[dark];
                int different = 0;
                const RECT& head = s.layout.headline;
                for (int y = head.top; y < head.bottom; ++y) for (int x = head.left; x < head.right; ++x) if (pixels[static_cast<size_t>(y) * width + x] != background) ++different;
                Check(different > 20, std::string(dark ? "dark" : "light") + " render: headline text pixels drawn");
            };
            current = &c;
            focusButton.SetFocus();
            Alert(ownerWindow, CString(c.text.c_str()), CString(c.caption.c_str()), c.type);
        }
        NativePreferences::SetTestSnapshot(false, NativePreferences::Language::English);
        Check(surface[0] != surface[1], "light and dark appearance use different dialog surfaces");
    }

    // Per-monitor DPI geometry scales with the display.
    {
        AlertSpec spec;
        spec.headline = L"Scale";
        spec.body = L"A sentence that is long enough to need a little more than the minimum dialog width to show.";
        spec.actions = { { IDOK, L"OK", false } };
        const SIZE work{ 3840, 2160 };
        const auto a = MeasureAlert(spec, 96, work, false), b = MeasureAlert(spec, 192, work, false);
        Check(std::abs(b.size.cx - 2 * a.size.cx) <= 3 && std::abs(b.size.cy - 2 * a.size.cy) <= 6, "layout scales with DPI (96 vs 192)");
        const auto narrow = MeasureAlert(spec, 96, { 320, 480 }, false);
        Check(narrow.size.cx <= 320, "layout fits a narrow work area");
    }

    NativeMaterial::SetAlertTestHooks(nullptr);
    owner.DestroyWindow();
    SetThreadDesktop(original);
    CloseDesktop(desktop);
    std::printf("RESULT failures=%d\n", failures);
    return failures ? 1 : 0;
}
