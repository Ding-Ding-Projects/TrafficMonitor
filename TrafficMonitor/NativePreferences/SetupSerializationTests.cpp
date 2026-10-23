#include "../stdafx.h"
#include "../IniHelper.h"
#include "SetupState.h"
#include "Storage.h"
#include <iostream>
namespace { int passed{},failed{};void Check(bool value,const char* label){std::cout<<(value?"PASS ":"FAIL ")<<label<<'\n';value?++passed:++failed;} }
int wmain()
{
    using namespace NativePreferences;
    wchar_t temporary[MAX_PATH]{};GetTempPathW(MAX_PATH,temporary);
    const std::wstring directory=std::wstring(temporary)+L"TrafficMonitor-setup-test-"+std::to_wstring(GetCurrentProcessId())+L"\\";
    if(!CreateDirectoryW(directory.c_str(),nullptr))return 2;const auto file=directory+L"dashboard-setup.ini";
    {CIniHelper old(file);old.WriteInt(L"setup",L"schemaVersion",1);old.WriteInt(L"setup",L"language",1);Check(old.Save(),"schema1 fixture uses production INI writer");}
    Initialize(directory,MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US));
    auto accepted=Current();Check(accepted.values.language==Language::Cantonese&&!accepted.persisted,"service initializes explicit schema1 Cantonese over legacy English");
    Draft draft{accepted.values,accepted.base,false,false};
    {CIniHelper output(file);WriteSetupPreferences(output,draft);output.WriteInt(L"setup",L"step",2);output.WriteInt(L"setup",L"result0",1);Check(output.Save(),"schema2 navigation serializes production preference state");}
    CIniHelper saved(file);Draft resumed;Check(ReadSetupPreferences(saved,resumed)&&!resumed.dirty&&resumed.values==accepted.values,"production INI readback preserves clean migrated pair");
    Check(StartupFallback(saved,Values{})==accepted.values&&saved.GetInt(L"setup",L"step",0)==2&&saved.GetInt(L"setup",L"result0",0)==1,"restart fallback preserves migrated base and unrelated progress");
    draft.values={Language::English,Appearance::Dark};draft.dirty=true;
    {CIniHelper output(file);WriteSetupPreferences(output,draft);Check(output.Save(),"dirty draft A serialized with accepted base");}
    auto outcome=TryApply({Language::Bilingual,Appearance::Light},accepted.base);accepted=Current();
    Check(outcome==Result::Saved,"service accepts B independently of serialized draft A");
    CIniHelper dirty(file);Draft a;Check(ReadSetupPreferences(dirty,a),"dirty draft A parses from actual INI bytes");ReconcileResume(a,accepted);
    Check(a.dirty&&a.conflict&&a.values==draft.values&&Current().values==accepted.values,"serialized draft A survives accepted B without replacing B");
    for(const auto* key:{L"schemaVersion",L"language",L"appearance",L"baseLanguage",L"baseAppearance",L"preferenceDirty",L"baseFingerprint"})
    {
        CIniHelper invalid;WriteSetupPreferences(invalid,draft);invalid.WriteString(L"setup",key,L"0garbage");Draft value;
        Check(!ReadSetupPreferences(invalid,value),"malformed protected field rejected without integer coercion");
        invalid.WriteString(L"setup",key,L"");Check(!ReadSetupPreferences(invalid,value),"empty protected field rejected");
    }
    {CIniHelper absent;absent.WriteInt(L"setup",L"schemaVersion",1);Check(StartupFallback(absent,{Language::Cantonese,Appearance::System}).language==Language::Cantonese,"missing schema1 language does not manufacture explicit English");}
    DeleteFileW(file.c_str());DeleteFileW((directory+L"native-interface.ini").c_str());RemoveDirectoryW(directory.c_str());
    std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}