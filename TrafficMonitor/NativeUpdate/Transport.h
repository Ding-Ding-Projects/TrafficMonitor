#pragma once
#include <windows.h>
#include <atomic>
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>

namespace NativeUpdate::Detail
{
    struct TransportError : std::runtime_error
    {
        DWORD code, httpStatus;
        TransportError(const char* operation,DWORD result,DWORD status=0)
            :std::runtime_error(operation),code(result),httpStatus(status){}
    };
    std::wstring Wide(const std::string& value);
    std::vector<unsigned char> Fetch(const std::wstring& url, size_t limit,
        const std::atomic<bool>& cancel, ULONGLONG deadline);
    std::string Digest(const std::vector<unsigned char>& bytes, bool sha1=false);
    std::vector<unsigned char> Read(const std::filesystem::path& path, size_t limit);
    void Write(const std::filesystem::path& path, const std::vector<unsigned char>& bytes);
    bool PlainDirectory(const std::filesystem::path& path);
}
