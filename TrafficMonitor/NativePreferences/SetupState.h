#pragma once
#include "Draft.h"
namespace NativePreferences
{
    // Uses the application's INI reader/writer without silently coercing protected fields.
    template<class Ini> bool ReadSetupPreferences(const Ini& ini,Draft& draft)
    {
        std::wstring schema,language,appearance,baseLanguage,baseAppearance,dirty,fingerprint;
        if(!ini.GetString(L"setup",L"schemaVersion",schema)||schema!=L"2")return false;
        if(!ini.GetString(L"setup",L"language",language)||!ParseSavedLanguage(language,draft.values.language))return false;
        Language a{},b{};
        if(!ini.GetString(L"setup",L"appearance",appearance)||!ParseSavedLanguage(appearance,a)
            ||!ini.GetString(L"setup",L"baseLanguage",baseLanguage)||!ParseSavedLanguage(baseLanguage,draft.base.values.language)
            ||!ini.GetString(L"setup",L"baseAppearance",baseAppearance)||!ParseSavedLanguage(baseAppearance,b)
            ||!ini.GetString(L"setup",L"preferenceDirty",dirty)||(dirty!=L"true"&&dirty!=L"false")
            ||!ini.GetString(L"setup",L"baseFingerprint",fingerprint))return false;
        if(fingerprint!=L"missing")
        {
            if(fingerprint.size()!=64)return false;
            for(auto c:fingerprint)if(!((c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f')))return false;
        }
        draft.values.appearance=static_cast<Appearance>(a);draft.base.values.appearance=static_cast<Appearance>(b);
        draft.base.fingerprint=fingerprint;draft.dirty=dirty==L"true";return true;
    }
    template<class Ini> void WriteSetupPreferences(Ini& ini,const Draft& draft)
    {
        ini.WriteInt(L"setup",L"schemaVersion",2);
        ini.WriteInt(L"setup",L"language",static_cast<int>(draft.values.language));
        ini.WriteInt(L"setup",L"appearance",static_cast<int>(draft.values.appearance));
        ini.WriteInt(L"setup",L"baseLanguage",static_cast<int>(draft.base.values.language));
        ini.WriteInt(L"setup",L"baseAppearance",static_cast<int>(draft.base.values.appearance));
        ini.WriteString(L"setup",L"baseFingerprint",draft.base.fingerprint);
        ini.WriteBool(L"setup",L"preferenceDirty",draft.dirty);
    }
    template<class Ini> Values StartupFallback(const Ini& ini,Values defaults)
    {
        std::wstring schema;ini.GetString(L"setup",L"schemaVersion",schema);
        if(schema==L"1"){std::wstring language;if(ini.GetString(L"setup",L"language",language))ParseSavedLanguage(language,defaults.language);}
        else if(schema==L"2"){Draft state;if(ReadSetupPreferences(ini,state)&&state.base.fingerprint==L"missing")defaults=state.base.values;}
        return defaults;
    }
}