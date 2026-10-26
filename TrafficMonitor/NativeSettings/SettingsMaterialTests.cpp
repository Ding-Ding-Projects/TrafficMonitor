// Hidden-window verification that the native Settings editor and its embedded Interface preferences editor
// contain Material components only. The real RowControls (one settings row) and the real
// NativePreferences::Editor are created in a never-shown parent, every descendant window class is audited
// and the draft flow (apply, revert, refuse) is driven through the new controls with window messages.
#include "../stdafx.h"
#include "RowControls.h"
#include "../NativeMaterial/SettingsControls.h"
#include "../NativePreferences/Editor.h"
#include "../NativeMenus/SearchableMenu.h"
#include <cstdio>
#include <functional>

// The text field reaches the application searchable menu for its context menu; the focused test has none.
namespace NativeMenus { UINT Track(CMenu*, UINT, int, int, CWnd*, NativeRegex::Options*) { return 0; } }

namespace
{
    using namespace NativeMaterial;
    using namespace NativeSettings;
    CWinApp application;
    int failures{}, passes{};
    void Check(bool value, const char* name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name); value ? ++passes : ++failures; }

    // Routes every WM_COMMAND to a handler, like the settings editor does for its rows.
    class Host : public CWnd
    {
    public:
        std::function<void(UINT id, UINT code)> handler;
        int commands{};
    protected:
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
        {
            if (message == WM_COMMAND) { ++commands; if (handler) handler(LOWORD(w), HIWORD(w)); }
            return CWnd::WindowProc(message, w, l);
        }
    };

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
    void ClickSegment(HWND window, int index, int count)
    {
        CRect client;
        ::GetClientRect(window, client);
        const float scale = GetDpiForWindow(window) / 96.f;
        const auto rects = SegmentRects(client.Width() / scale, client.Height() / scale, count);
        const auto& r = rects[index];
        Click(window, static_cast<int>((r.left + r.right) / 2 * scale), static_cast<int>((r.top + r.bottom) / 2 * scale));
    }
    void Key(HWND window, UINT key) { SendMessage(window, WM_KEYDOWN, key, 0); SendMessage(window, WM_KEYUP, key, 0); }
    void Chars(HWND window, const wchar_t* text) { for (; *text; ++text) SendMessage(window, WM_CHAR, *text, 0); }
    void TypeNumber(HWND edit, const wchar_t* text)
    {
        SendMessage(edit, EM_SETSEL, 0, -1);
        Chars(edit, text);
    }
    CString ClassOf(HWND window) { wchar_t name[128]{}; GetClassNameW(window, name, 128); return name; }
    int CountClass(HWND root, const wchar_t* className)
    {
        struct Context { int count{}; const wchar_t* name{}; } context{ 0, className };
        EnumChildWindows(root, [](HWND child, LPARAM l) -> BOOL
        {
            auto* c = reinterpret_cast<Context*>(l);
            if (ClassOf(child).CompareNoCase(c->name) == 0) ++c->count;
            return TRUE;
        }, reinterpret_cast<LPARAM>(&context));
        return context.count;
    }
    HWND FindById(HWND root, UINT id)
    {
        struct Context { UINT id; HWND found{}; } context{ id, nullptr };
        EnumChildWindows(root, [](HWND child, LPARAM l) -> BOOL
        {
            auto* c = reinterpret_cast<Context*>(l);
            if (static_cast<UINT>(GetDlgCtrlID(child)) == c->id && !c->found) c->found = child;
            return TRUE;
        }, reinterpret_cast<LPARAM>(&context));
        return context.found;
    }
    template<class T> T* As(HWND window) { return dynamic_cast<T*>(CWnd::FromHandlePermanent(window)); }

    // One settings row owns its controls and a draft; the host plays the settings editor.
    struct Draft { bool toggle{}; int number{}; int unit{}; RowChange last{ RowChange::None }; };
    struct Fixture
    {
        Host host;
        RowControls row;
        Draft draft;
        UINT firstId{ 9000 };
        bool Open(const RowShape& shape)
        {
            if (!host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Row", WS_POPUP, 0, 0, 700, 400, nullptr, 0)) return false;
            host.handler = [this](UINT id, UINT code) { draft.last = row.Receive(id - firstId, code, &draft.toggle, &draft.number, &draft.unit); };
            return row.Create(&host, firstId, shape, L"Setting name", L"Details") != FALSE;
        }
    };

    void AuditRow(const char* name, const RowShape& shape, size_t expectedStatics)
    {
        Fixture f;
        const std::string label(name);
        Check(f.Open(shape), (label + ": row controls created").c_str());
        const auto stock = FindStockControls(f.host.GetSafeHwnd());
        std::string found;
        for (const auto& s : stock) { found += CStringA(s.className); found += ' '; }
        Check(stock.empty(), (label + ": no stock Button/Edit/ComboBox/ListBox/common control below the row (found: " + found + ")").c_str());
        Check(f.row.toggle.GetSafeHwnd() != nullptr == shape.toggle, (label + ": a Switch exists exactly for on/off settings").c_str());
        if (shape.toggle) Check(!IsStockControlClass(ClassOf(f.row.toggle.GetSafeHwnd())), (label + ": the on/off control is not a BUTTON window").c_str());
        Check(f.row.number.GetSafeHwnd() != nullptr == shape.number, (label + ": a NumberStepper exists exactly for numeric settings").c_str());
        Check(CountClass(f.host.GetSafeHwnd(), L"Edit") == (shape.number ? 1 : 0), (label + ": the only EDIT is the entry field owned by the NumberStepper").c_str());
        Check(f.row.unit.GetSafeHwnd() != nullptr == shape.trafficUnits, (label + ": a SegmentedButton exists exactly for the MB/GB unit").c_str());
        Check(CountClass(f.host.GetSafeHwnd(), L"Static") == static_cast<int>(expectedStatics), (label + ": remaining windows are read-only STATIC text").c_str());
    }
}

int wmain()
{
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 2;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // ------------------------------------------------------------------ rows
    RowShape toggleOnly; toggleOnly.toggle = true;
    RowShape numberOnly; numberOnly.number = true; numberOnly.minimum = 200; numberOnly.maximum = 30000; numberOnly.step = 100;
    RowShape threshold; threshold.toggle = true; threshold.number = true; threshold.minimum = 1; threshold.maximum = 100; threshold.notification = true;
    RowShape traffic; traffic.toggle = true; traffic.number = true; traffic.trafficUnits = true; traffic.minimum = 1; traffic.maximum = 32767; traffic.notification = true;
    // Statics: description and defaults (plus the name for a number-only row, plus the unavailable line for notifications).
    AuditRow("toggle row", toggleOnly, 2);
    AuditRow("number row", numberOnly, 3);
    AuditRow("threshold row", threshold, 3);
    AuditRow("traffic row", traffic, 3);

    // The audit itself must see a stray stock control, otherwise "empty" proves nothing.
    {
        Host host;
        host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Negative", WS_POPUP, 0, 0, 300, 300, nullptr, 0);
        CButton stray; stray.Create(L"stray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 80, 30), &host, 1);
        CEdit loose; loose.Create(WS_CHILD | WS_VISIBLE, CRect(0, 40, 80, 70), &host, 2);
        CComboBox combo; combo.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, CRect(0, 80, 80, 200), &host, 3);
        const auto found = FindStockControls(host.GetSafeHwnd());
        Check(found.size() == 3, "audit reports a stray BUTTON, a loose EDIT and a COMBOBOX");
        const auto skipped = FindStockControls(host.GetSafeHwnd(), { stray.GetSafeHwnd(), loose.GetSafeHwnd(), combo.GetSafeHwnd() });
        Check(skipped.empty(), "audit skips windows the owner explicitly allows");
        Check(!IsStockControlClass(L"Static") && IsStockControlClass(L"Button") && IsStockControlClass(L"msctls_updown32") && IsStockControlClass(L"SysListView32"), "class rule: STATIC allowed, BUTTON and common controls stock");
    }

    // --------------------------------------------------- draft flow through the new controls
    {
        Fixture f;
        Check(f.Open(traffic), "traffic row opened for the draft flow");
        f.draft = { false, 200, 0 };
        // Revert / load: the controls only display the draft.
        f.row.ShowToggle(f.draft.toggle, true); f.row.ShowNumber(f.draft.number, true); f.row.ShowTrafficUnit(f.draft.unit, true);
        Check(!f.row.ToggleValue() && f.row.NumberValue() == 200 && f.row.TrafficUnitValue() == 0, "load shows the draft in the Switch, the stepper and the unit segments");
        const int before = f.host.commands;
        f.row.ShowToggle(true, true); f.row.ShowNumber(300, true); f.row.ShowTrafficUnit(1, true);
        Check(f.host.commands == before, "showing a draft never notifies the editor");
        f.row.ShowToggle(false, true); f.row.ShowNumber(200, true); f.row.ShowTrafficUnit(0, true);

        ClickCenter(f.row.toggle);
        Check(f.draft.toggle && f.draft.last == RowChange::Toggle, "clicking the Switch moves it into the draft");
        Key(f.row.toggle.GetSafeHwnd(), VK_SPACE);
        Check(!f.draft.toggle && f.draft.last == RowChange::Toggle, "Space on the Switch toggles it back and the draft follows");
        ClickCenter(f.row.toggle);

        const HWND edit = f.row.number.EditWindow();
        TypeNumber(edit, L"350");
        Key(edit, VK_RETURN);
        Check(f.draft.number == 350 && f.draft.last == RowChange::Number, "Enter commits a typed number into the draft");
        Key(edit, VK_UP);
        Check(f.draft.number == 351, "Up arrow steps the stepper and the draft follows");
        TypeNumber(edit, L"99999");
        Key(edit, VK_RETURN);
        Check(f.draft.number == 351 && !f.row.number.GetValidationMessage().IsEmpty(), "a number above the maximum is refused with a message and leaves the draft");
        Check(!f.row.CommitNumber(), "apply refuses text outside the range");
        Key(edit, VK_ESCAPE);

        ClickSegment(f.row.unit.GetSafeHwnd(), 1, 2);
        Check(f.draft.unit == 1 && f.draft.last == RowChange::Unit, "choosing GB moves the unit into the draft");
        Key(f.row.unit.GetSafeHwnd(), VK_LEFT);
        Check(f.draft.unit == 0, "Left arrow returns to MB");
        f.row.ShowTrafficUnit(7, true);
        Check(f.row.TrafficUnitValue() == -1, "an invalid stored unit selects no segment");

        // Revert: the editor reloads the applied values into the same controls.
        f.row.ShowToggle(false, true); f.row.ShowNumber(200, true); f.row.ShowTrafficUnit(0, true);
        Check(!f.row.ToggleValue() && f.row.NumberValue() == 200 && f.row.TrafficUnitValue() == 0, "revert restores applied values into every control");

        // Unavailable capability: stored values stay visible, nothing can be edited or notified.
        f.row.ShowToggle(true, false); f.row.ShowNumber(80, false); f.row.ShowTrafficUnit(1, false);
        const int quiet = f.host.commands;
        ClickCenter(f.row.toggle);
        Check(!f.row.toggle.IsWindowEnabled() && !f.row.number.IsWindowEnabled() && f.row.ToggleValue() && f.row.NumberValue() == 80 && f.host.commands == quiet, "unavailable row keeps its values and ignores input");
    }
    {
        Fixture f;
        Check(f.Open(numberOnly), "interval row opened for the step rules");
        f.draft = { false, 1000, 0 };
        f.row.ShowNumber(1000, true);
        const HWND edit = f.row.number.EditWindow();
        TypeNumber(edit, L"250");
        Key(edit, VK_RETURN);
        Check(f.draft.last == RowChange::NumberRefused && f.draft.number == 1000 && f.row.NumberValue() == 1000, "a value off the 100 step is refused and the control returns to the draft value");
        TypeNumber(edit, L"250");
        Check(!f.row.CommitNumber(), "apply refuses typed text that is off the step");
        TypeNumber(edit, L"400");
        Check(f.row.CommitNumber() && f.draft.number == 400, "apply commits typed text on the step into the draft");
        Key(edit, VK_NEXT);
        Check(f.draft.number == 200 && f.row.NumberValue() == 200, "PageDown clamps at the minimum and the draft follows");
    }

    // ------------------------------------------------ the embedded Interface preferences editor
    {
        Host host;
        host.CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"Interface", WS_POPUP, 0, 0, 900, 900, nullptr, 0);
        NativePreferences::Editor editor;
        host.handler = [&](UINT id, UINT code) { editor.Command(id, code); };
        Check(editor.Create(&host), "interface preferences editor created");
        editor.SetFilter(true, true, true);
        editor.Layout(0, 0, 800, 96, true);
        const auto stock = FindStockControls(host.GetSafeHwnd());
        std::string found;
        for (const auto& s : stock) { found += CStringA(s.className); found += ' '; }
        Check(stock.empty(), ("interface editor: no stock control below it (found: " + found + ")").c_str());
        Check(CountClass(host.GetSafeHwnd(), L"Edit") == 0, "interface editor: no EDIT window at all");
        const HWND language = FindById(host.GetSafeHwnd(), NativePreferences::Editor::LanguageChoice);
        const HWND appearance = FindById(host.GetSafeHwnd(), NativePreferences::Editor::AppearanceChoice);
        const HWND sounds = FindById(host.GetSafeHwnd(), NativePreferences::Editor::SoundChoice);
        Check(As<SegmentedButton>(language) && As<SegmentedButton>(appearance) && As<SegmentedButton>(sounds), "language, appearance and action sounds are SegmentedButtons");
        if (As<SegmentedButton>(language) && As<SegmentedButton>(appearance) && As<SegmentedButton>(sounds))
        {
            Check(As<SegmentedButton>(language)->GetSegmentCount() == 3 && As<SegmentedButton>(appearance)->GetSegmentCount() == 3 && As<SegmentedButton>(sounds)->GetSegmentCount() == 3, "each choice offers its three options");
            const auto start = editor.draft.values;
            ClickSegment(appearance, 2, 3);
            Check(static_cast<int>(editor.draft.values.appearance) == 2 && editor.draft.dirty, "choosing the third appearance segment changes the draft and marks it unsaved");
            ClickSegment(sounds, 2, 3);
            Check(editor.draft.values.actionSounds == NativePreferences::ActionSounds::All, "choosing All action sounds changes the draft");
            ClickSegment(language, 1, 3);
            Check(editor.draft.values.language == NativePreferences::Language::Cantonese, "choosing the second language segment changes the draft");
            Check(As<SegmentedButton>(language)->GetSelected() == 1 && As<SegmentedButton>(appearance)->GetSelected() == 2 && As<SegmentedButton>(sounds)->GetSelected() == 2, "the segments display the draft after refresh");
            // Revert: Use saved returns the draft to the accepted values and the controls follow.
            editor.Command(NativePreferences::Editor::RevertChoice, BN_CLICKED);
            Check(editor.draft.values == start && As<SegmentedButton>(appearance)->GetSelected() == static_cast<int>(start.appearance) && !editor.draft.dirty, "Use saved restores the accepted values into every segmented control");
            // Reset: defaults go back into the draft.
            ClickSegment(appearance, 1, 3);
            editor.Command(NativePreferences::Editor::ResetChoice, BN_CLICKED);
            Check(editor.draft.values.appearance == NativePreferences::Defaults().appearance, "Reset puts the default appearance back into the draft");
            // A click notification must not be mistaken for a changed selection.
            const auto frozen = editor.draft.values;
            editor.Command(NativePreferences::Editor::AppearanceChoice, BN_CLICKED);
            Check(editor.draft.values == frozen, "a plain click notification on a choice changes nothing");
        }
    }

    std::printf("Settings Material checks: %d passed, %d failed\n", passes, failures);
    return failures ? 1 : 0;
}
