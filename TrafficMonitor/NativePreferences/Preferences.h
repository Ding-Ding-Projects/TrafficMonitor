#pragma once
#include <Windows.h>
#include <string>
#include <cstdint>
namespace NativePreferences
{
    enum class Language { English, Cantonese, Bilingual };
    enum class Appearance { System, Light, Dark };
    struct Values { Language language{Language::English}; Appearance appearance{Appearance::System}; };
    inline bool operator==(Values a, Values b) { return a.language==b.language && a.appearance==b.appearance; }
    inline bool operator!=(Values a, Values b) { return !(a==b); }
    inline bool Valid(Values value) { return static_cast<unsigned>(value.language)<3 && static_cast<unsigned>(value.appearance)<3; }
    struct Base { std::uint64_t revision{}; std::wstring fingerprint; Values values; };
    inline bool Matches(const Base& a,const Base& b) { return a.revision==b.revision && a.fingerprint==b.fingerprint && a.values==b.values; }
    enum class Origin { Defaults, Migrated, Loaded, Applied, Uncertain };
    struct Snapshot { Values values; Base base; bool dark{}, protectedFile{}, persisted{}; DWORD error{};Origin origin{Origin::Defaults}; };
    enum class Result { Saved, Conflict, Invalid, Protected, WriteFailed, Uncertain };
    // Only the application UI thread owns this service. No callbacks or cross-process notifications.
    void Initialize(const std::wstring& directory, LANGID legacyLanguage);
    Snapshot Current();
    Values Defaults();
    Result TryApply(Values candidate, const Base& expected);
    bool Reload();
    void RefreshSystemAppearance();
    constexpr UINT ChangedMessage = WM_APP + 0x53A;
}
