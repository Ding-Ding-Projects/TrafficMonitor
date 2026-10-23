#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

// The package marker is deliberately separate from portable global settings.
namespace InstallerLifecycle
{
    inline std::wstring ModuleDirectory()
    {
        std::vector<wchar_t> path(32768);
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0 || length >= path.size())
            return {};
        const std::wstring module(path.data(), length);
        const auto slash = module.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"" : module.substr(0, slash + 1);
    }

    inline bool IsManaged(const std::wstring& directory = ModuleDirectory())
    {
        wchar_t managed[16]{};
        if (directory.empty())
            return false;
        GetPrivateProfileStringW(L"installation", L"managed", L"", managed, 16,
            (directory + L"installed-mode.ini").c_str());
        return _wcsicmp(managed, L"true") == 0;
    }

    inline std::wstring StartupCommand(const std::wstring& directory = ModuleDirectory())
    {
        if (directory.empty())
            return {};
        const auto parent = directory.find_last_of(L"\\/", directory.size() - 2);
        if (parent == std::wstring::npos)
            return {};
        return L"\"" + directory.substr(0, parent + 1) + L"Update.exe\" --processStart TrafficMonitor.exe";
    }

    inline LONG ReadStartupStatus(std::wstring& value)
    {
        HKEY key{};
        LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_QUERY_VALUE, &key);
        if (result != ERROR_SUCCESS)
            return result;
        DWORD type{}, bytes{};
        result = RegQueryValueExW(key, L"TrafficMonitor", nullptr, &type, nullptr, &bytes);
        if (result != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(wchar_t) || bytes > 65536)
        {
            RegCloseKey(key);
            return result == ERROR_SUCCESS ? ERROR_INVALID_DATA : result;
        }
        std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, 0);
        result = RegQueryValueExW(key, L"TrafficMonitor", nullptr, &type,
            reinterpret_cast<BYTE*>(buffer.data()), &bytes);
        RegCloseKey(key);
        if (result != ERROR_SUCCESS || type != REG_SZ)
            return result == ERROR_SUCCESS ? ERROR_INVALID_DATA : result;
        value.assign(buffer.data());
        return ERROR_SUCCESS;
    }

    inline bool ReadStartup(std::wstring& value)
    {
        return ReadStartupStatus(value) == ERROR_SUCCESS;
    }

    inline bool SetStartup(bool enabled, bool onlyIfAbsent = false,
        const std::wstring& directory = ModuleDirectory())
    {
        const auto command = StartupCommand(directory);
        if (command.empty())
            return false;
        std::wstring existing;
        const LONG readResult = ReadStartupStatus(existing);
        const bool present = readResult == ERROR_SUCCESS;
        if (!present && readResult != ERROR_FILE_NOT_FOUND && readResult != ERROR_PATH_NOT_FOUND)
            return false;
        // Never replace or remove a different installation's entry.
        if (present && existing != command)
            return !enabled;
        if (onlyIfAbsent && present)
            return true;
        if (!enabled && !present)
            return true;
        HKEY key{};
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
            return false;
        const LONG result = enabled ? RegSetValueExW(key, L"TrafficMonitor", 0, REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()), static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)))
            : RegDeleteValueW(key, L"TrafficMonitor");
        RegCloseKey(key);
        return result == ERROR_SUCCESS || (!enabled && result == ERROR_FILE_NOT_FOUND);
    }

    inline bool HandleEvent(const std::wstring& event, int& exitCode,
        const std::wstring& directory = ModuleDirectory())
    {
        if (event != L"--squirrel-install" && event != L"--squirrel-updated"
            && event != L"--squirrel-obsolete" && event != L"--squirrel-uninstall")
            return false;
        exitCode = 0;
        if (!IsManaged(directory))
            exitCode = ERROR_INVALID_DATA;
        else if (event == L"--squirrel-install" && !SetStartup(true, true, directory))
            exitCode = ERROR_ACCESS_DENIED;
        else if (event == L"--squirrel-uninstall" && !SetStartup(false, false, directory))
            exitCode = ERROR_ACCESS_DENIED;
        // Update and obsolete callbacks preserve the user's startup choice.
        return true;
    }

    // Call before any normal initialization or the interactive-instance mutex.
    inline bool HandleCallback(int& exitCode)
    {
        int count{};
        LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!arguments)
            return false;
        const std::wstring event = count > 1 ? arguments[1] : L"";
        LocalFree(arguments);
        return HandleEvent(event, exitCode);
    }
}
