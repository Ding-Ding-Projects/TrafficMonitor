#pragma once
#include "Preferences.h"
namespace NativePreferences::Storage
{
    struct ReadResult { Values values; std::wstring fingerprint; bool missing{}, valid{}, protectedFile{}; DWORD error{}; };
    ReadResult Read(const std::wstring& path);
    Result Write(const std::wstring& path, Values value, const std::wstring& expected, ReadResult& stored);
}
