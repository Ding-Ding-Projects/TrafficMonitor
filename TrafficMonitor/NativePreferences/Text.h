#pragma once
#include "Preferences.h"
#include "../NativeMaterial/Theme.h"
#include <afxstr.h>
namespace NativePreferences
{
    enum class TextId { Interface, LanguageLabel, AppearanceLabel, Apply, Revert, Reset, UseSaved, Reapply, Draft, Saved, Conflict, Protected, WriteFailed, Uncertain, Scope, Preview, English, Cantonese, Bilingual, System, Light, Dark, Loaded, Defaults, Count };
    CString Text(TextId id, Language language);
    inline CString Text(TextId id){return Text(id,Current().values.language);}
    CString Pair(const wchar_t* english,const wchar_t* cantonese,Language language);
    inline CString Pair(const wchar_t* english,const wchar_t* cantonese){return Pair(english,cantonese,Current().values.language);}
    template<class... Args> CString Format(const wchar_t* english,const wchar_t* cantonese,Args... args)
    {CString a,b;a.Format(english,args...);b.Format(cantonese,args...);return Pair(a,b);}
    inline NativeMaterial::Appearance EffectiveAppearance(){return Current().dark?NativeMaterial::Appearance::Dark:NativeMaterial::Appearance::Light;}
}
