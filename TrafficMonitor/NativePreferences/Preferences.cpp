#include "../stdafx.h"
#include "Preferences.h"
#include "SetupState.h"
#include "Storage.h"
#include "Subscription.h"
#include "../IniHelper.h"
namespace NativePreferences
{
    namespace { Snapshot accepted; Values defaults, missingFallback;Origin startupOrigin{Origin::Defaults}; std::wstring path; bool initialized{}; DWORD thread{};
        bool Dark(Values value){if(value.appearance!=Appearance::System)return value.appearance==Appearance::Dark;DWORD light=1,size=sizeof(light);RegGetValueW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"AppsUseLightTheme",RRF_RT_REG_DWORD,nullptr,&light,&size);return light==0;}
        void Publish(){accepted.dark=Dark(accepted.values);accepted.base.values=accepted.values;++accepted.base.revision;NotifySubscribers();}
    }
    void Initialize(const std::wstring& directory,LANGID legacyLanguage)
    {
        if(initialized)return;initialized=true;thread=GetCurrentThreadId();path=directory+L"native-interface.ini";
        defaults.language=PRIMARYLANGID(legacyLanguage?legacyLanguage:GetUserDefaultUILanguage())==LANG_CHINESE?Language::Cantonese:Language::English;
        accepted.values=defaults;
        WIN32_FILE_ATTRIBUTE_DATA a{};const auto setup=directory+L"dashboard-setup.ini";
        if(GetFileAttributesExW(setup.c_str(),GetFileExInfoStandard,&a)&&!a.nFileSizeHigh&&a.nFileSizeLow<=16384)
        {CIniHelper ini(setup);accepted.values=StartupFallback(ini,defaults);std::wstring schema,language;ini.GetString(L"setup",L"schemaVersion",schema);Language parsed{};Draft saved;if((schema==L"1"&&ini.GetString(L"setup",L"language",language)&&ParseSavedLanguage(language,parsed))||(schema==L"2"&&ReadSetupPreferences(ini,saved)&&saved.base.fingerprint==L"missing"))startupOrigin=Origin::Migrated;}
        missingFallback=accepted.values;Reload();
    }
    Snapshot Current(){return accepted;}
    Values Defaults(){return defaults;}
    bool Reload()
    {
        if(!initialized||GetCurrentThreadId()!=thread)return false;auto stored=Storage::Read(path);
        if(stored.valid)accepted.values=stored.missing?missingFallback:stored.values;
        accepted.origin=stored.valid?(stored.missing?startupOrigin:Origin::Loaded):Origin::Uncertain;accepted.persisted=stored.valid&&!stored.missing;accepted.base.fingerprint=stored.fingerprint;accepted.protectedFile=stored.protectedFile;accepted.error=stored.error;
        Publish();return stored.valid;
    }
    Result TryApply(Values value,const Base& expected)
    {
        if(!initialized||GetCurrentThreadId()!=thread||!Valid(value))return Result::Invalid;
        if(accepted.protectedFile)return Result::Protected;
        if(!Matches(expected,accepted.base))return Result::Conflict;
        Storage::ReadResult stored;const auto result=Storage::Write(path,value,expected.fingerprint,stored);
        if(result==Result::Saved){accepted.values=value;accepted.base.fingerprint=stored.fingerprint;accepted.error=0;accepted.persisted=true;accepted.origin=Origin::Applied;Publish();}
        else if(result==Result::Uncertain){accepted.protectedFile=true;accepted.origin=Origin::Uncertain;accepted.error=ERROR_READ_FAULT;}
        return result;
    }
    void RefreshSystemAppearance(){if(initialized&&GetCurrentThreadId()==thread){accepted.dark=Dark(accepted.values);NotifySubscribers();}}
}
