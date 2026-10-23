#pragma once
#include "Preferences.h"
namespace NativePreferences
{
    struct Draft { Values values; Base base; bool dirty{}, conflict{}; };
    inline bool ParseSavedLanguage(const std::wstring& text,Language& language)
    {if(text!=L"0"&&text!=L"1"&&text!=L"2")return false;language=static_cast<Language>(text[0]-L'0');return true;}
    inline void ReconcileResume(Draft& draft,const Snapshot& accepted)
    {
        if(!draft.dirty){draft={accepted.values,accepted.base,false,false};return;}
        draft.conflict=draft.base.fingerprint!=accepted.base.fingerprint||draft.base.values!=accepted.values;
        draft.base.revision=draft.conflict?0:accepted.base.revision;
    }
}
