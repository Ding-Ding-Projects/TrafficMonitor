#pragma once
#include <windows.h>
#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

namespace NativeUpdate::Detail
{
    std::wstring Wide(const std::string& value);
    std::vector<unsigned char> Fetch(const std::wstring& url, size_t limit,
        const std::atomic<bool>& cancel, ULONGLONG deadline);
    std::string Digest(const std::vector<unsigned char>& bytes, bool sha1=false);
    std::vector<unsigned char> Read(const std::filesystem::path& path, size_t limit);
    void Write(const std::filesystem::path& path, const std::vector<unsigned char>& bytes);
    bool PlainDirectory(const std::filesystem::path& path);
}
