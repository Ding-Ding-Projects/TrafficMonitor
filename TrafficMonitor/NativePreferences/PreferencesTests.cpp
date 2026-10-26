#include "Draft.h"
#include "Storage.h"
#include "ActionSounds.h"
#include <iostream>
#include <vector>
namespace { int passed{},failed{};void Check(bool value,const char* label){std::cout<<(value?"PASS ":"FAIL ")<<label<<'\n';value?++passed:++failed;}
void Put(const std::wstring& path,const std::string& data){HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);DWORD count{};if(h!=INVALID_HANDLE_VALUE){WriteFile(h,data.data(),static_cast<DWORD>(data.size()),&count,nullptr);CloseHandle(h);}}
}
namespace { int playCount{};NativeSounds::Cue lastCue{};bool playResult{true};
bool FakePlayer(NativeSounds::Cue cue){++playCount;lastCue=cue;return playResult;}
void ResetPlayer(){playCount=0;lastCue=NativeSounds::Cue::None;playResult=true;} }
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
    Put(path,"[interface]\r\nschemaVersion=3\r\nlanguage=0\r\nappearance=0\r\n");auto unknown=Storage::Read(path);Check(unknown.protectedFile&&!unknown.valid,"unknown schema remains protected");Check(Storage::Write(path,{},unknown.fingerprint,result)==Result::Protected&&Storage::Read(path).fingerprint==unknown.fingerprint,"unknown schema cannot be downgraded");
    Put(path,"[interface]\r\nschemaVersion=1\r\nlanguage=0\r\nappearance=9\r\n");Check(!Storage::Read(path).valid&&Storage::Read(path).protectedFile,"invalid pair does not partially apply");
    Put(path,std::string(16385,'x'));Check(Storage::Read(path).protectedFile,"oversized file stays protected");
    HANDLE lock=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Check(lock!=INVALID_HANDLE_VALUE&&Storage::Read(path).protectedFile,"sharing failure protects original bytes");if(lock!=INVALID_HANDLE_VALUE)CloseHandle(lock);
    Base base{7,L"fingerprint",{Language::English,Appearance::Light}};Base stale=base;stale.revision=6;Check(!Matches(base,stale),"stale in-process revision rejected");stale=base;stale.values.language=Language::Cantonese;Check(!Matches(base,stale),"stale accepted pair rejected");
    Snapshot newer;newer.values={Language::Bilingual,Appearance::Dark};newer.base={9,L"newer",newer.values};Draft a{{Language::Cantonese,Appearance::Light},base,true,false};ReconcileResume(a,newer);Check(a.values.language==Language::Cantonese&&a.values.appearance==Appearance::Light&&a.conflict&&a.base.revision==0&&newer.values.language==Language::Bilingual,"saved draft A survives accepted B across restart without replacing B");
    Draft unchanged{base.values,base,false,false};ReconcileResume(unchanged,newer);Check(unchanged.values==newer.values&&Matches(unchanged.base,newer.base),"clean resumed draft adopts accepted preferences");
    Language selected=Language::Cantonese;Check(!ParseSavedLanguage(L"",selected)&&selected==Language::Cantonese,"missing old language is not explicit English");Check(ParseSavedLanguage(L"0",selected)&&selected==Language::English,"explicit old English choice migrates");Check(!ParseSavedLanguage(L"0garbage",selected),"malformed old language is not coerced");
    // Action sound preference: default, persistence, bounds and recovery.
    Check(Values{}.actionSounds==ActionSounds::Off,"action sounds default to Off");
    Check(Valid({Language::English,Appearance::System,ActionSounds::All})&&!Valid({Language::English,Appearance::System,static_cast<ActionSounds>(3)}),"action sound level is bounded to Off, Important and All");
    Check(Values{Language::English,Appearance::System,ActionSounds::All}!=Values{Language::English,Appearance::System,ActionSounds::Off},"action sound level takes part in value comparison and conflict detection");
    {
        bool allRoundTrip=true;
        for(int language=0;language<3;++language)for(int appearance=0;appearance<3;++appearance)for(int sounds=0;sounds<3;++sounds)
        {
            const Values value{static_cast<Language>(language),static_cast<Appearance>(appearance),static_cast<ActionSounds>(sounds)};
            DeleteFileW(path.c_str());const auto fresh=Storage::Read(path);Storage::ReadResult written;
            if(Storage::Write(path,value,fresh.fingerprint,written)!=Result::Saved||!(Storage::Read(path).values==value))allRoundTrip=false;
        }
        Check(allRoundTrip,"every language, appearance and action sound combination round trips");
    }
    DeleteFileW(path.c_str());{const auto fresh=Storage::Read(path);Storage::ReadResult written;Storage::Write(path,{Language::Cantonese,Appearance::Dark,ActionSounds::Off},fresh.fingerprint,written);}
    {HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);char buffer[256]{};DWORD count{};if(h!=INVALID_HANDLE_VALUE){ReadFile(h,buffer,sizeof(buffer)-1,&count,nullptr);CloseHandle(h);}
     Check(std::string(buffer,count)=="[interface]\r\nschemaVersion=1\r\nlanguage=1\r\nappearance=2\r\n","Off writes the original schema 1 bytes so older builds keep reading the file");}
    Put(path,"[interface]\r\nschemaVersion=1\r\nlanguage=2\r\nappearance=1\r\n");{const auto legacy=Storage::Read(path);Check(legacy.valid&&legacy.values.actionSounds==ActionSounds::Off&&legacy.values.language==Language::Bilingual,"an existing schema 1 file loads with action sounds Off and keeps its other choices");}
    Put(path,"[interface]\r\nschemaVersion=2\r\nlanguage=1\r\nappearance=1\r\nactionSounds=2\r\n");{const auto saved=Storage::Read(path);Check(saved.valid&&saved.values.actionSounds==ActionSounds::All&&saved.values.language==Language::Cantonese,"schema 2 loads the saved action sound level");}
    Put(path,"[interface]\r\nschemaVersion=2\r\nlanguage=1\r\nappearance=1\r\nactionSounds=3\r\n");Check(!Storage::Read(path).valid&&Storage::Read(path).protectedFile,"out of range action sound level stays protected");
    Put(path,"[interface]\r\nschemaVersion=2\r\nlanguage=1\r\nappearance=1\r\n");Check(!Storage::Read(path).valid&&Storage::Read(path).protectedFile,"schema 2 without an action sound line stays protected");
    Put(path,"[interface]\r\nschemaVersion=2\r\nlanguage=1\r\nappearance=1\r\nactionSounds=0\r\n");Check(!Storage::Read(path).valid,"non-canonical schema 2 Off is not partially accepted");
    Put(path,"[interface]\r\nschemaVersion=2\r\nlanguage=1\r\nappearance=1\r\nactionSounds=1garbage\r\n");Check(!Storage::Read(path).valid,"malformed action sound level is not coerced");
    {const auto fresh=Storage::Read(path);Check(Storage::Write(path,{Language::English,Appearance::System,ActionSounds::All},fresh.fingerprint,result)==Result::Protected,"protected malformed file is not overwritten by an action sound change");}
    // Dispatcher decisions with an injected player: no real audio is produced here.
    using namespace NativeSounds;
    {
        Throttle gate;ResetPlayer();
        Check(!Dispatch(ActionSounds::Off,Action::SettingsApplied,FakePlayer,1000,gate)&&playCount==0,"Off plays nothing for a successful action");
        Check(!Dispatch(ActionSounds::Off,Action::ActionRefused,FakePlayer,2000,gate)&&playCount==0,"Off plays nothing for a refused action");
        Check(Dispatch(ActionSounds::All,Action::SettingsApplied,FakePlayer,10000,gate)&&playCount==1&&lastCue==Cue::Success,"All plays the success cue after a successful apply");
    }
    {Throttle gate;ResetPlayer();Dispatch(ActionSounds::All,Action::ActionRefused,FakePlayer,5000,gate);Check(playCount==1&&lastCue==Cue::Failure,"a refused action maps to the failure cue");}
    {Throttle gate;ResetPlayer();Dispatch(ActionSounds::All,Action::SearchNoResults,FakePlayer,5000,gate);Check(playCount==1&&lastCue==Cue::Notice,"an empty search maps to the notice cue");}
    {Throttle gate;ResetPlayer();Dispatch(ActionSounds::All,Action::SetupCompleted,FakePlayer,5000,gate);Check(playCount==1&&lastCue==Cue::Success,"completed setup maps to the success cue");}
    {Throttle gate;ResetPlayer();Check(!Dispatch(ActionSounds::Important,Action::SettingsApplied,FakePlayer,5000,gate)&&!Dispatch(ActionSounds::Important,Action::SearchNoResults,FakePlayer,6000,gate)&&playCount==0,"Important suppresses success and notice cues");Check(Dispatch(ActionSounds::Important,Action::ActionRefused,FakePlayer,7000,gate)&&lastCue==Cue::Failure,"Important still plays the failure cue");}
    {Throttle gate;ResetPlayer();Dispatch(ActionSounds::All,Action::SettingsApplied,FakePlayer,5000,gate);Dispatch(ActionSounds::All,Action::SettingsApplied,FakePlayer,5000+MinimumGapMilliseconds-1,gate);Check(playCount==1,"repeated cues inside the minimum gap do not overlap");Dispatch(ActionSounds::All,Action::SettingsApplied,FakePlayer,5000+MinimumGapMilliseconds,gate);Check(playCount==2,"a cue after the minimum gap plays again");}
    {Throttle gate;ResetPlayer();playResult=false;Check(!Dispatch(ActionSounds::All,Action::SettingsApplied,FakePlayer,5000,gate)&&playCount==1,"an unavailable audio device is reported as not played without throwing");}
    {Throttle gate;Check(!Dispatch(ActionSounds::All,Action::SettingsApplied,nullptr,5000,gate),"a missing player is tolerated");}
    Check(AliasFor(Cue::Success)==static_cast<DWORD_PTR>(SND_ALIAS_SYSTEMASTERISK)&&AliasFor(Cue::Failure)==static_cast<DWORD_PTR>(SND_ALIAS_SYSTEMHAND)&&AliasFor(Cue::Notice)==static_cast<DWORD_PTR>(SND_ALIAS_SYSTEMEXCLAMATION)&&AliasFor(Cue::None)==0,"cues map to Windows system sound aliases only");
    DeleteFileW(path.c_str());RemoveDirectoryW(directory.c_str());std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
