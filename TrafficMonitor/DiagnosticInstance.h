#pragma once
// Diagnostic-only single-instance isolation.
// A portable copy launched for evidence capture (for example on a hidden desktop) must not share the
// session-wide single-instance mutex with the person's installed copy, or it would take the
// "already running" path and could message that real copy. The isolation is opt-in: a regular file
// named <full exe path> + MarkerSuffix beside the executable, and only in portable (not managed) mode.
// Without the marker, or in installed mode, the normal mutex name is returned unchanged.
#include <cstdint>
#include <string>

namespace DiagnosticInstance
{
    inline constexpr wchar_t MarkerSuffix[] = L".diagnostic-instance";

    // Marker attributes are acceptable only for an existing regular file (no directory, no reparse point).
    inline bool IsRegularFileAttributes(unsigned long attributes)
    {
        constexpr unsigned long invalid = 0xFFFFFFFFul, directory = 0x10ul, reparsePoint = 0x400ul;
        return attributes != invalid && (attributes & (directory | reparsePoint)) == 0;
    }

    // Stable 64-bit FNV-1a over the UTF-16 code units of the path, as 16 lowercase hex digits.
    inline std::wstring PathHash(const std::wstring& path)
    {
        std::uint64_t hash = 14695981039346656037ull;
        for (wchar_t c : path)
        {
            hash ^= static_cast<std::uint16_t>(c);
            hash *= 1099511628211ull;
        }
        std::wstring text;
        for (int shift = 60; shift >= 0; shift -= 4)
            text.push_back(L"0123456789abcdef"[(hash >> shift) & 0xF]);
        return text;
    }

    // Isolation applies only to a regular-file marker in portable mode.
    inline bool IsIsolated(bool markerIsRegularFile, bool installedMode)
    {
        return markerIsRegularFile && !installedMode;
    }

    inline std::wstring MutexName(const std::wstring& normalName, const std::wstring& exePath, bool markerIsRegularFile, bool installedMode)
    {
        if (!IsIsolated(markerIsRegularFile, installedMode))
            return normalName;
        return normalName + L"-diag-" + PathHash(exePath);
    }
}