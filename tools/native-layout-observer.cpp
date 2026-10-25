// Structural observations only. This output is not a native layout/promotion receipt.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "../TrafficMonitor/NativeMaterial/LayoutObservationProtocol.h"
#include <aclapi.h>
#include <cmath>
#include <set>
#include <map>
#include <algorithm>
#include <bcrypt.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cwctype>
#include <sddl.h>
#include <string>
#include <vector>

namespace
{
struct Failure
{
    const char *phase;
    DWORD error;
};
void Need(bool ok, const char *phase)
{
    if (!ok)
        throw Failure{phase, GetLastError()};
}
struct Handle
{
    HANDLE h{INVALID_HANDLE_VALUE};
    ~Handle()
    {
        if (h && h != INVALID_HANDLE_VALUE)
            CloseHandle(h);
    }
};
uint64_t Number(const wchar_t *value)
{
    Need(value && *value, "number");
    for (auto p = value; *p; ++p)
        Need(*p >= L'0' && *p <= L'9', "number");
    errno = 0;
    auto n = _wcstoui64(value, nullptr, 10);
    Need(errno != ERANGE, "number-overflow");
    return n;
}
std::wstring Full(const std::wstring &value)
{
    wchar_t b[32768];
    DWORD n = GetFullPathNameW(value.c_str(), _countof(b), b, nullptr);
    Need(n && n < _countof(b), "full-path");
    return {b, n};
}
std::wstring Self()
{
    wchar_t b[32768];
    DWORD n = GetModuleFileNameW(nullptr, b, _countof(b));
    Need(n && n < _countof(b), "self-path");
    return {b, n};
}
std::string Json(const std::wstring &value)
{
    int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    Need(n > 0 || value.empty(), "utf8");
    std::string raw(n, ' ');
    if (n)
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), &raw[0], n, nullptr, nullptr);
    std::string s = "\"";
    for (unsigned char c : raw)
    {
        if (c == '"' || c == '\\')
        {
            s += '\\';
            s += char(c);
        }
        else if (c < 32)
        {
            char hex[7];
            sprintf_s(hex, "\\u%04x", c);
            s += hex;
        }
        else
            s += char(c);
    }
    return s + '"';
}
std::string Rect(const RECT &r)
{
    return "{\"left\":" + std::to_string(r.left) + ",\"top\":" + std::to_string(r.top) +
           ",\"right\":" + std::to_string(r.right) + ",\"bottom\":" + std::to_string(r.bottom) +
           "}";
}
std::string Id(HWND h)
{
    char b[24];
    sprintf_s(b, "\"0x%llx\"", reinterpret_cast<uint64_t>(h));
    return b;
}
std::wstring Hash(const std::wstring &path)
{
    Handle file;
    file.h = CreateFileW(path.c_str(), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                         OPEN_EXISTING, 0, nullptr);
    Need(file.h != INVALID_HANDLE_VALUE, "hash-open");
    BCRYPT_ALG_HANDLE a = nullptr;
    BCRYPT_HASH_HANDLE h = nullptr;
    Need(BCryptOpenAlgorithmProvider(&a, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0,
         "hash-algorithm");
    std::wstring out;
    try
    {
        Need(BCryptCreateHash(a, &h, nullptr, 0, nullptr, 0, 0) >= 0, "hash-create");
        BYTE bytes[65536], digest[32];
        DWORD n;
        for (;;)
        {
            Need(ReadFile(file.h, bytes, sizeof(bytes), &n, nullptr) != FALSE, "hash-read");
            if (!n)
                break;
            Need(BCryptHashData(h, bytes, n, 0) >= 0, "hash-data");
        }
        Need(BCryptFinishHash(h, digest, 32, 0) >= 0, "hash-finish");
        for (auto b : digest)
        {
            out += L"0123456789abcdef"[b >> 4];
            out += L"0123456789abcdef"[b & 15];
        }
    }
    catch (...)
    {
        if (h)
            BCryptDestroyHash(h);
        BCryptCloseAlgorithmProvider(a, 0);
        throw;
    }
    BCryptDestroyHash(h);
    BCryptCloseAlgorithmProvider(a, 0);
    return out;
}
uint64_t Created(HANDLE p)
{
    FILETIME c, e, k, u;
    Need(GetProcessTimes(p, &c, &e, &k, &u) != FALSE, "creation-read");
    return (uint64_t(c.dwHighDateTime) << 32) | c.dwLowDateTime;
}
std::wstring DesktopName(HDESK h)
{
    wchar_t b[512];
    DWORD n;
    Need(GetUserObjectInformationW(h, UOI_NAME, b, sizeof(b), &n) != FALSE, "desktop-name");
    return b;
}
void NoReparse(std::wstring directory)
{
    for (;;)
    {
        const DWORD a = GetFileAttributesW(directory.c_str());
        Need(a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) &&
                 !(a & FILE_ATTRIBUTE_REPARSE_POINT),
             "output-directory");
        auto slash = directory.find_last_of(L"\\/");
        if (slash == std::wstring::npos || slash <= 2)
            break;
        directory.resize(slash);
    }
}
std::wstring OutputPath(const std::wstring &name)
{
    Need(name.size() > 6 && name.size() <= 128 && name.substr(name.size() - 6) == L".jsonl",
         "output-name");
    for (size_t i = 0; i < name.size() - 6; ++i)
        Need(iswalnum(name[i]) || name[i] == L'-' || name[i] == L'_', "output-name");
    const auto exe = Self();
    const auto dir = exe.substr(0, exe.find_last_of(L"\\/"));
    Need(dir.size() > 3 && dir[1] == L':', "output-local-drive");
    NoReparse(dir);
    return dir + L"\\" + name;
}
struct Writer
{
    Handle file;
    ULONGLONG started{GetTickCount64()};
    void Open(const std::wstring &name)
    {
        const auto path = OutputPath(name);
        Handle token;
        Need(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.h) != FALSE, "output-token");
        DWORD size = 0;
        GetTokenInformation(token.h, TokenUser, nullptr, 0, &size);
        std::vector<BYTE> data(size);
        Need(GetTokenInformation(token.h, TokenUser, data.data(), size, &size) != FALSE,
             "output-owner");
        LPWSTR sid = nullptr;
        Need(ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(data.data())->User.Sid, &sid) !=
                 FALSE,
             "output-sid");
        std::wstring sddl = L"D:P(A;;GA;;;" + std::wstring(sid) + L")";
        LocalFree(sid);
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        Need(ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1,
                                                                  &descriptor, nullptr) != FALSE,
             "output-security");
        SECURITY_ATTRIBUTES sa{sizeof(sa), descriptor, FALSE};
        file.h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        const DWORD error = GetLastError();
        LocalFree(descriptor);
        SetLastError(error);
        Need(file.h != INVALID_HANDLE_VALUE, "output-create-new");
    }
    void Write(const std::string &payload)
    {
        std::string line = payload + "\n";
        DWORD n = 0;
        Need(line.size() < 1024 * 1024 &&
                 WriteFile(file.h, line.data(), static_cast<DWORD>(line.size()), &n, nullptr) &&
                 n == line.size(),
             "output-write");
        FlushFileBuffers(file.h);
    }
    void Phase(const char *name)
    {
        Write("{\"phase\":\"" + std::string(name) +
              "\",\"elapsedMs\":" + std::to_string(GetTickCount64() - started) + "}");
    }
};
struct Target
{
    DWORD pid{}, thread{};
    HWND root{};
    HDESK desktop{};
    std::wstring path, hash;
    uint64_t creation{};
    Handle process;
    struct Match
    {
        HWND root;
        DWORD pid;
        bool found{};
    };
    static BOOL CALLBACK MatchRoot(HWND w, LPARAM l)
    {
        auto &m = *reinterpret_cast<Match *>(l);
        DWORD pid = 0;
        GetWindowThreadProcessId(w, &pid);
        if (w == m.root && pid == m.pid)
            m.found = true;
        return TRUE;
    }
    void Child(HWND w)
    {
        DWORD owner = 0;
        GetWindowThreadProcessId(w, &owner);
        Need(IsWindow(w) && owner == pid && (w == root || IsChild(root, w)) &&
                 GetAncestor(w, GA_ROOT) == root,
             "subtree-identity");
    }
    void RecordIdentity(Writer &out, const char *phase)
    {
        out.Write("{\"phase\":\"" + std::string(phase) + "\",\"pid\":" + std::to_string(pid) +
                  ",\"creationFiletime\":\"" + std::to_string(creation) +
                  "\",\"rootHwnd\":" + Id(root) + ",\"executablePath\":" + Json(path) +
                  ",\"executableSha256\":" + Json(hash) +
                  ",\"desktop\":" + Json(DesktopName(desktop)) + "}");
    }
    void Validate()
    {
        Need(WaitForSingleObject(process.h, 0) == WAIT_TIMEOUT, "process-alive");
        Need(Created(process.h) == creation, "process-creation");
        wchar_t b[32768];
        DWORD n = _countof(b);
        Need(QueryFullProcessImageNameW(process.h, 0, b, &n) != FALSE, "process-path-read");
        Need(_wcsicmp(Full(b).c_str(), path.c_str()) == 0, "process-path");
        Need(_wcsicmp(Hash(path).c_str(), hash.c_str()) == 0, "process-hash");
        DWORD owner = 0;
        thread = GetWindowThreadProcessId(root, &owner);
        Need(IsWindow(root) && owner == pid && GetAncestor(root, GA_ROOT) == root, "root-identity");
        Match match{root, pid, false};
        Need(EnumDesktopWindows(desktop, MatchRoot, reinterpret_cast<LPARAM>(&match)) != FALSE,
             "desktop-enumerate");
        Need(match.found, "desktop-membership");
    }
};
#include "native-layout-render-reader.h"

struct Inventory
{
    std::vector<HWND> values;
    bool overflow{};
    static BOOL CALLBACK Add(HWND w, LPARAM p)
    {
        auto &self = *reinterpret_cast<Inventory *>(p);
        if (self.values.size() >= 2048)
        {
            self.overflow = true;
            return FALSE;
        }
        self.values.push_back(w);
        return TRUE;
    }
};
std::string Scroll(HWND h, int axis)
{
    SCROLLINFO s{sizeof(s), SIF_RANGE | SIF_PAGE | SIF_POS};
    SetLastError(0);
    if (!GetScrollInfo(h, axis, &s))
        return "{\"status\":\"unavailable\",\"error\":" + std::to_string(GetLastError()) + "}";
    const auto maximum = std::max<long long>(
        s.nMin, static_cast<long long>(s.nMax) -
                    std::max<long long>(0, static_cast<long long>(s.nPage) - 1));
    Need(s.nMin <= s.nMax && s.nPos >= s.nMin && s.nPos <= maximum, "scroll-range");
    return "{\"status\":\"observed-standard-api\",\"min\":" + std::to_string(s.nMin) +
           ",\"max\":" + std::to_string(s.nMax) + ",\"page\":" + std::to_string(s.nPage) +
           ",\"position\":" + std::to_string(s.nPos) + "}";
}
void Window(Target &target, HWND h, Writer &output)
{
    target.Child(h);
    RECT outer{}, client{};
    Need(GetWindowRect(h, &outer) && GetClientRect(h, &client), "rectangles");
    POINT origin{0, 0};
    Need(ClientToScreen(h, &origin) != FALSE, "client-origin");
    OffsetRect(&client, origin.x, origin.y);
    wchar_t cls[256];
    Need(GetClassNameW(h, cls, _countof(cls)) > 0, "class-name");
    const auto style = GetWindowLongPtrW(h, GWL_STYLE);
    const HWND parent = h == target.root ? nullptr : GetParent(h);
    if (parent)
        target.Child(parent);
    const UINT dpi = GetDpiForWindow(h);
    Need(dpi > 0, "window-dpi");
    const auto context = GetWindowDpiAwarenessContext(h);
    const int awareness = GetAwarenessFromDpiAwarenessContext(context);
    const bool v2 =
        AreDpiAwarenessContextsEqual(context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE;
    std::string edit = "{\"status\":\"unavailable\",\"reason\":\"not-a-native-EDIT\"}";
    if (_wcsicmp(cls, L"Edit") == 0)
    {
        RECT format{};
        DWORD_PTR result = 0;
        Need(SendMessageTimeoutW(h, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&format),
                                 SMTO_ABORTIFHUNG | SMTO_BLOCK, 500, &result) != 0,
             "edit-format-timeout");
        OffsetRect(&format, origin.x, origin.y);
        edit = "{\"status\":\"observed\",\"method\":\"EM_GETRECT\",\"rect\":" + Rect(format) + "}";
    }
    // Custom rails do not acquire native scroll completeness from a successful API call.
    const bool standard = _wcsicmp(cls, L"Edit") == 0 || _wcsicmp(cls, L"ListBox") == 0 ||
                          _wcsicmp(cls, L"ScrollBar") == 0;
    std::string scroll =
        standard
            ? "{\"horizontal\":" + Scroll(h, SB_HORZ) + ",\"vertical\":" + Scroll(h, SB_VERT) + "}"
            : "{\"status\":\"unavailable\",\"reason\":\"custom-or-unverified-scroll-owner\"}";
    if (_wcsicmp(cls, L"ScrollBar") == 0)
        scroll = "{\"axis\":\"" + std::string((style & SBS_VERT) ? "vertical" : "horizontal") +
                 "\",\"control\":" + Scroll(h, SB_CTL) + "}";
    output.Write("{\"phase\":\"control\",\"hwnd\":" + Id(h) +
                 ",\"parent\":" + (parent ? Id(parent) : "null") +
                 ",\"ownerPid\":" + std::to_string(target.pid) + ",\"className\":" + Json(cls) +
                 ",\"controlId\":" + std::to_string(GetDlgCtrlID(h)) +
                 ",\"visible\":" + (IsWindowVisible(h) ? "true" : "false") +
                 ",\"enabled\":" + (IsWindowEnabled(h) ? "true" : "false") +
                 ",\"style\":" + std::to_string(static_cast<uint64_t>(style)) +
                 ",\"dpi\":" + std::to_string(dpi) + ",\"awareness\":" + std::to_string(awareness) +
                 ",\"perMonitorV2\":" + (v2 ? "true" : "false") + ",\"outer\":" + Rect(outer) +
                 ",\"client\":" + Rect(client) + ",\"clientOrigin\":{\"x\":" +
                 std::to_string(origin.x) + ",\"y\":" + std::to_string(origin.y) +
                 "},\"editFormattingArea\":" + edit + ",\"standardScroll\":" + scroll + "}");
    target.Child(h);
}
void Observe(Target &target, Writer &output, RendererRead::Session *renderer = nullptr)
{
    output.Phase("identity.start");
    target.Validate();
    target.RecordIdentity(output, "identity.start.verified");
    Inventory first;
    EnumChildWindows(target.root, Inventory::Add, reinterpret_cast<LPARAM>(&first));
    Need(!first.overflow, "control-limit");
    Window(target, target.root, output);
    if (renderer)
        renderer->Control(target, target.root, output);
    for (HWND h : first.values)
    {
        Window(target, h, output);
        if (renderer)
            renderer->Control(target, h, output);
    }
    GUITHREADINFO info{sizeof(info)};
    Need(GetGUIThreadInfo(target.thread, &info) != FALSE, "focus-read");
    if (info.hwndFocus)
    {
        target.Child(info.hwndFocus);
        Need(info.hwndFocus == target.root || std::find(first.values.begin(), first.values.end(),
                                                        info.hwndFocus) != first.values.end(),
             "focus-inventory");
    }
    output.Write("{\"phase\":\"focus\",\"hwnd\":" + (info.hwndFocus ? Id(info.hwndFocus) : "null") +
                 ",\"method\":\"GetGUIThreadInfo\",\"status\":\"" +
                 (info.hwndFocus ? std::string("observed") : std::string("no-focused-window")) +
                 "\"}");
    Inventory last;
    EnumChildWindows(target.root, Inventory::Add, reinterpret_cast<LPARAM>(&last));
    Need(!last.overflow, "control-limit-end");
    auto a = first.values, b = last.values;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    Need(a == b, "inventory-changed");
    output.Phase("identity.end");
    target.Validate();
    target.RecordIdentity(output, "identity.end.verified");
    if (renderer)
        renderer->End(target, output);
    output.Write("{\"phase\":\"complete\",\"structuralObservationCompleted\":true,"
                 "\"receiptComplete\":false,\"validV2Receipt\":false,\"validV3Receipt\":false,"
                 "\"unavailable\":[\"renderer-font\",\"renderer-text-glyph-bounds\",\"custom-rail-"
                 "range\",\"paint-sequence\",\"image-binding\"]}");
}
int Worker(wchar_t **a, bool rendererRequested)
{
    Writer output;
    HDESK desk = nullptr;
    DPI_AWARENESS_CONTEXT old = nullptr;
    try
    {
        output.Open(a[6]);
        output.Write(
            "{\"kind\":\"incomplete-native-structural-observation\",\"version\":1,"
            "\"coordinateSpace\":\"physical-screen-pixels\",\"rawControlTextCollected\":false}");
        desk = OpenDesktopW(a[1], 0, FALSE, DESKTOP_READOBJECTS | DESKTOP_ENUMERATE);
        Need(desk != nullptr, "desktop-open");
        Need(SetThreadDesktop(desk) != FALSE, "desktop-attach");
        Need(DesktopName(GetThreadDesktop(GetCurrentThreadId())) == a[1], "observer-desktop");
        old = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        Need(old != nullptr &&
                 AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(),
                                              DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2),
             "observer-physical-dpi");
        Target target;
        auto pid = Number(a[0]);
        Need(pid && pid <= MAXDWORD, "pid");
        target.pid = static_cast<DWORD>(pid);
        target.root = reinterpret_cast<HWND>(Number(a[2]));
        target.path = Full(a[3]);
        target.creation = Number(a[4]);
        target.hash = a[5];
        Need(target.hash.size() == 64, "hash-input");
        for (auto c : target.hash)
            Need(iswxdigit(c) != 0, "hash-input");
        target.desktop = desk;
        target.process.h =
            OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, target.pid);
        Need(target.process.h != nullptr, "process-open");
        RendererRead::Session renderer;
        if (rendererRequested)
        {
            Need(std::wstring(a[8]) == L"--renderer", "renderer-arguments");
            target.Validate();
            renderer.Begin(target, Number(a[9]), Number(a[10]), output);
        }
        Observe(target, output, rendererRequested ? &renderer : nullptr);
        if (old)
            SetThreadDpiAwarenessContext(old);
        CloseDesktop(desk);
        return 0;
    }
    catch (const Failure &f)
    {
        if (output.file.h != INVALID_HANDLE_VALUE)
            try
            {
                output.Write("{\"phase\":\"rejected\",\"at\":\"" + std::string(f.phase) +
                             "\",\"win32\":" + std::to_string(f.error) +
                             ",\"receiptComplete\":false}");
            }
            catch (...)
            {
            }
    }
    catch (...)
    {
    }
    if (old)
        SetThreadDpiAwarenessContext(old);
    if (desk)
        CloseDesktop(desk);
    return 1;
}
std::wstring Quote(const wchar_t *value)
{
    std::wstring out = L"\"";
    size_t slash = 0;
    for (auto p = value; *p; ++p)
    {
        if (*p == L'\\')
        {
            ++slash;
            continue;
        }
        out.append(*p == L'"' ? slash * 2 + 1 : slash, L'\\');
        out += *p;
        slash = 0;
    }
    out.append(slash * 2, L'\\');
    return out + L"\"";
}
int Watch(const std::wstring &command, DWORD timeout)
{
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION child{};
    auto mutableCommand = command;
    const auto exe = Self();
    Need(CreateProcessW(exe.c_str(), &mutableCommand[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &si, &child) != FALSE,
         "helper-start");
    Handle process;
    process.h = child.hProcess;
    Handle thread;
    thread.h = child.hThread;
    DWORD wait = WaitForSingleObject(process.h, timeout);
    if (wait != WAIT_OBJECT_0)
    {
        Need(TerminateProcess(process.h, 124) != FALSE, "helper-terminate");
        Need(WaitForSingleObject(process.h, 2000) == WAIT_OBJECT_0, "helper-exit");
        return 124;
    }
    DWORD code;
    Need(GetExitCodeProcess(process.h, &code) != FALSE, "helper-code");
    return static_cast<int>(code);
}
int SelfTest()
{
    int failed = 0;
    auto expect = [&](const char *name, auto action, const char *phase,
                      const char *alternative = nullptr) {
        bool rejected = false;
        try
        {
            action();
        }
        catch (const Failure &f)
        {
            rejected = std::string(f.phase) == phase ||
                       (alternative && std::string(f.phase) == alternative);
        }
        std::printf("%s %s\n", rejected ? "PASS" : "FAIL", name);
        if (!rejected)
            ++failed;
    };
    expect("output traversal rejected", [] { OutputPath(L"../escape.jsonl"); }, "output-name");
    expect(
        "numeric overflow rejected", [] { Number(L"999999999999999999999999999999"); },
        "number-overflow");
    const std::wstring desktopName = L"NativeLayoutTests-" + std::to_wstring(GetCurrentProcessId());
    HDESK desk = CreateDesktopW(desktopName.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    Need(desk != nullptr, "test-desktop");
    Need(SetThreadDesktop(desk) != FALSE, "test-attach");
    auto dpi = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Need(dpi != nullptr, "test-dpi");
    HWND root = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 400, 300, nullptr, nullptr,
                                GetModuleHandleW(nullptr), nullptr);
    HWND edit =
        CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_MULTILINE | WS_VSCROLL, 10, 10, 200, 100,
                        root, reinterpret_cast<HMENU>(101), GetModuleHandleW(nullptr), nullptr);
    HWND other = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 100, 100, nullptr, nullptr,
                                 GetModuleHandleW(nullptr), nullptr);
    Need(root && edit && other, "test-windows");
    RendererRead::SelfTest(edit);
    Target t;
    t.pid = GetCurrentProcessId();
    t.root = root;
    t.desktop = desk;
    t.path = Self();
    t.hash = Hash(t.path);
    t.process.h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, t.pid);
    t.creation = Created(t.process.h);
    t.Validate();
    std::puts("PASS real hidden desktop identity");
    HDESK otherDesktop = CreateDesktopW((desktopName + L"-other").c_str(), nullptr, nullptr, 0,
                                        GENERIC_ALL, nullptr);
    Need(otherDesktop != nullptr, "test-other-desktop");
    t.desktop = otherDesktop;
    expect(
        "wrong desktop rejected", [&] { t.Validate(); }, "desktop-membership", "desktop-enumerate");
    t.desktop = desk;
    CloseDesktop(otherDesktop);
    ++t.creation;
    expect("reused process identity rejected", [&] { t.Validate(); }, "process-creation");
    --t.creation;
    const auto hash = t.hash;
    t.hash.assign(64, L'0');
    expect("wrong executable hash rejected", [&] { t.Validate(); }, "process-hash");
    t.hash = hash;
    expect("subtree escape rejected", [&] { t.Child(other); }, "subtree-identity");
    ++t.pid;
    expect("foreign owner rejected", [&] { t.Child(edit); }, "subtree-identity");
    --t.pid;
    const std::wstring name = L"selftest-" + std::to_wstring(GetTickCount64()) + L".jsonl";
    Writer out;
    out.Open(name);
    Observe(t, out);
    std::puts("PASS structural observation remains explicitly incomplete");
    expect(
        "existing output rejected",
        [&] {
            Writer duplicate;
            duplicate.Open(name);
        },
        "output-create-new");
    DestroyWindow(root);
    DestroyWindow(other);
    expect("destroyed root rejected", [&] { t.Validate(); }, "root-identity");
    SetThreadDpiAwarenessContext(dpi);
    CloseDesktop(desk);
    const int timeout = Watch(Quote(Self().c_str()) + L" --watchdog-test-child", 100);
    std::printf("%s owned helper timeout\n", timeout == 124 ? "PASS" : "FAIL");
    if (timeout != 124)
        ++failed;
    return failed ? 1 : 0;
}
} // namespace
int wmain(int argc, wchar_t **argv)
{
    try
    {
        if (argc == 2 && std::wstring(argv[1]) == L"--watchdog-test-child")
        {
            Sleep(INFINITE);
            return 1;
        }
        if (argc == 2 && std::wstring(argv[1]) == L"--self-test")
            return SelfTest();
        if ((argc == 10 || argc == 13) && std::wstring(argv[1]) == L"--worker")
            return Worker(argv + 2, argc == 13);
        if (argc != 9 && argc != 12)
        {
            std::fputs(
                "Usage: native-layout-observer PID DESKTOP ROOT_HWND EXE CREATION_FILETIME "
                "SHA256 NEW_BASENAME.jsonl TIMEOUT_SECONDS [--renderer MIN_SEQUENCE MAX_AGE_MS]\n",
                stderr);
            return 2;
        }
        auto seconds = Number(argv[8]);
        Need(seconds >= 5 && seconds <= 30, "timeout-bound");
        OutputPath(argv[7]);
        std::wstring command = Quote(Self().c_str()) + L" --worker";
        for (int i = 1; i < argc; ++i)
            command += L" " + Quote(argv[i]);
        const int result = Watch(command, static_cast<DWORD>(seconds * 1000));
        std::printf(
            "{\"observerExitCode\":%d,\"targetTerminated\":false,\"receiptComplete\":false}\n",
            result);
        return result;
    }
    catch (const Failure &f)
    {
        std::fprintf(stderr, "Observer rejected at %s (%lu).\n", f.phase, f.error);
        return 2;
    }
    catch (...)
    {
        return 2;
    }
}
