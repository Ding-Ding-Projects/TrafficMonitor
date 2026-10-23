// Build locally with MSVC: cl /EHsc /std:c++17 tests\lifecycle.cpp advapi32.lib shell32.lib
#include "../TrafficMonitor/InstallerLifecycle.h"
#include <cassert>
#include <iostream>

static int shortcutCalls = 0;
static DWORD VerifyShortcutRequest(const std::wstring& event, const std::wstring&)
{
    if (event != L"--squirrel-obsolete")
        ++shortcutCalls;
    return ERROR_SUCCESS;
}

int wmain()
{
    using namespace InstallerLifecycle;
    const std::wstring sandbox = L"Software\\TrafficMonitorLifecycleTest-" + std::to_wstring(GetCurrentProcessId());
    HKEY isolated{};
    assert(RegCreateKeyExW(HKEY_CURRENT_USER, sandbox.c_str(), 0, nullptr, 0,
        KEY_ALL_ACCESS, nullptr, &isolated, nullptr) == ERROR_SUCCESS);
    assert(RegOverridePredefKey(HKEY_CURRENT_USER, isolated) == ERROR_SUCCESS);

    wchar_t temporary[MAX_PATH]{};
    assert(GetTempPathW(MAX_PATH, temporary));
    const std::wstring root = std::wstring(temporary) + L"TrafficMonitor lifecycle " + std::to_wstring(GetCurrentProcessId()) + L"\\";
    const auto directory = root + L"app-1.2.3\\";
    assert(CreateDirectoryW(root.c_str(), nullptr));
    assert(CreateDirectoryW(directory.c_str(), nullptr));
    const auto marker = directory + L"installed-mode.ini";
    assert(!IsManaged(directory));
    assert(WritePrivateProfileStringW(L"installation", L"managed", L"true", marker.c_str()));
    assert(IsManaged(directory));
    const auto command = L"\"" + root + L"Update.exe\" --processStart TrafficMonitor.exe";
    assert(StartupCommand(directory) == command);
    int exitCode = -1;
    assert(!HandleEvent(L"--ordinary", exitCode, directory, VerifyShortcutRequest));
    assert(HandleEvent(L"--squirrel-install", exitCode, directory, VerifyShortcutRequest) && exitCode == 0);
    std::wstring value;
    assert(ReadStartup(value) && value == command);
    assert(SetStartup(false, false, directory));
    assert(HandleEvent(L"--squirrel-updated", exitCode, directory, VerifyShortcutRequest) && exitCode == 0);
    assert(!ReadStartup(value));
    assert(HandleEvent(L"--squirrel-obsolete", exitCode, directory, VerifyShortcutRequest) && exitCode == 0);
    assert(!ReadStartup(value));
    assert(SetStartup(true, false, directory));
    assert(HandleEvent(L"--squirrel-updated", exitCode, directory, VerifyShortcutRequest) && exitCode == 0);
    assert(ReadStartup(value) && value == command);
    assert(HandleEvent(L"--squirrel-uninstall", exitCode, directory, VerifyShortcutRequest) && exitCode == 0);
    assert(!ReadStartup(value));
    assert(SetStartup(true, false, directory));
    assert(SetStartup(false, false, root + L"app-other\\")); // Same install root owns the stable command.
    assert(SetStartup(true, false, directory));
    assert(SetStartup(false, false, root + L"other\\app-1\\")); // Another root must not remove it.
    assert(ReadStartup(value) && value == command);
    assert(!SetStartup(true, false, root + L"other\\app-1\\"));

    assert(shortcutCalls == 4);
    assert(RegOverridePredefKey(HKEY_CURRENT_USER, nullptr) == ERROR_SUCCESS);
    RegCloseKey(isolated);
    assert(RegDeleteTreeW(HKEY_CURRENT_USER, sandbox.c_str()) == ERROR_SUCCESS);
    DeleteFileW(marker.c_str());
    RemoveDirectoryW(directory.c_str());
    RemoveDirectoryW(root.c_str());
    std::cout << "PASS: lifecycle callbacks, quoted launcher, update choice, ownership and portable marker\n";
}
