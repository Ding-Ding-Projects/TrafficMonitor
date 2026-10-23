#include "Draft.h"
#include "Storage.h"
#include <iostream>
#include <vector>
namespace { int passed{},failed{};void Check(bool value,const char* label){std::cout<<(value?"PASS ":"FAIL ")<<label<<'\n';value?++passed:++failed;}
void Put(const std::wstring& path,const std::string& data){HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);DWORD count{};if(h!=INVALID_HANDLE_VALUE){WriteFile(h,data.data(),static_cast<DWORD>(data.size()),&count,nullptr);CloseHandle(h);}}
}
int main()
{
    using namespace NativePreferences;
    wchar_t temporary[MAX_PATH]{};GetTempPathW(MAX_PATH,temporary);const std::wstring directory=std::wstring(temporary)+L"TrafficMonitor-preferences-test-"+std::to_wstring(GetCurrentProcessId());
    if(!CreateDirectoryW(directory.c_str(),nullptr))return 2;const auto path=directory+L"\\native-interface.ini";
    auto initial=Storage::Read(path);Check(initial.missing&&initial.valid&&!initial.protectedFile&&initial.fingerprint==L"missing","missing file is not a saved file");
    Storage::ReadResult result;
    for(int language=0;language<3;++language)for(int appearance=0;appearance<3;++appearance)
    {const Values pair{static_cast<Language>(language),static_cast<Appearance>(appearance)};const auto previous=Storage::Read(path);Check(Storage::Write(path,pair,previous.fingerprint,result)==Result::Saved&&result.values==pair&&!result.missing,"all language/appearance pairs round trip");}
    const auto accepted=Storage::Read(path);Check(Storage::Write(path,{},L"stale",result)==Result::Conflict&&Storage::Read(path).fingerprint==accepted.fingerprint,"stale file fingerprint cannot overwrite accepted values");
    Put(path,"[interface]\r\nschemaVersion=2\r\nlanguage=0\r\nappearance=0\r\n");auto unknown=Storage::Read(path);Check(unknown.protectedFile&&!unknown.valid,"unknown schema remains protected");Check(Storage::Write(path,{},unknown.fingerprint,result)==Result::Protected&&Storage::Read(path).fingerprint==unknown.fingerprint,"unknown schema cannot be downgraded");
    Put(path,"[interface]\r\nschemaVersion=1\r\nlanguage=0\r\nappearance=9\r\n");Check(!Storage::Read(path).valid&&Storage::Read(path).protectedFile,"invalid pair does not partially apply");
    Put(path,std::string(16385,'x'));Check(Storage::Read(path).protectedFile,"oversized file stays protected");
    HANDLE lock=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Check(lock!=INVALID_HANDLE_VALUE&&Storage::Read(path).protectedFile,"sharing failure protects original bytes");if(lock!=INVALID_HANDLE_VALUE)CloseHandle(lock);
    Base base{7,L"fingerprint",{Language::English,Appearance::Light}};Base stale=base;stale.revision=6;Check(!Matches(base,stale),"stale in-process revision rejected");stale=base;stale.values.language=Language::Cantonese;Check(!Matches(base,stale),"stale accepted pair rejected");
    Snapshot newer;newer.values={Language::Bilingual,Appearance::Dark};newer.base={9,L"newer",newer.values};Draft a{{Language::Cantonese,Appearance::Light},base,true,false};ReconcileResume(a,newer);Check(a.values.language==Language::Cantonese&&a.values.appearance==Appearance::Light&&a.conflict&&a.base.revision==0&&newer.values.language==Language::Bilingual,"saved draft A survives accepted B across restart without replacing B");
    Draft unchanged{base.values,base,false,false};ReconcileResume(unchanged,newer);Check(unchanged.values==newer.values&&Matches(unchanged.base,newer.base),"clean resumed draft adopts accepted preferences");
    Language selected=Language::Cantonese;Check(!ParseSavedLanguage(L"",selected)&&selected==Language::Cantonese,"missing old language is not explicit English");Check(ParseSavedLanguage(L"0",selected)&&selected==Language::English,"explicit old English choice migrates");Check(!ParseSavedLanguage(L"0garbage",selected),"malformed old language is not coerced");
    DeleteFileW(path.c_str());RemoveDirectoryW(directory.c_str());std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
