#define NOMINMAX
#include "SilentUpdater.cpp"
#include <iostream>

namespace fs=std::filesystem;
using namespace NativeUpdate;
using namespace NativeUpdate::Detail;
int failures{};
void Expect(bool value,const char* name)
{std::cout<<(value?"PASS ":"FAIL ")<<name<<std::endl;if(!value)++failures;}
void Replace(const fs::path& path,const std::vector<unsigned char>& bytes){std::error_code error;fs::remove(path,error);Write(path,bytes);}
void Put(const fs::path& path,const std::string& text){Replace(path,Bytes(text));}
void Fixture()
{
    auto root=Root();auto pending=Staging(root)/L"pending";fs::create_directories(pending);
    auto package=Bytes("bounded fixture package");
    auto feed=Bytes(Digest(package,true)+" TrafficMonitor-1.0.32-full.nupkg "+std::to_string(package.size())+"\n");
    std::string source(40,'a');
    auto owner="https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/";
    auto tag=std::string("v1.0.32-run1-1");
    auto asset=[&](std::string name,const std::vector<unsigned char>& bytes){return "{\"name\":\""+name+"\",\"state\":\"uploaded\",\"browser_download_url\":\""+owner+"download/"+tag+"/"+name+"\",\"digest\":\"sha256:"+Digest(bytes)+"\",\"size\":"+std::to_string(bytes.size())+"}";};
    Put(pending/L"release.json","{\"draft\":false,\"prerelease\":false,\"tag_name\":\""+tag+"\",\"target_commitish\":\""+source+"\",\"html_url\":\""+owner+"tag/"+tag+"\",\"assets\":["+asset("RELEASES",feed)+","+asset("TrafficMonitor-1.0.32-full.nupkg",package)+"]}");
    Put(pending/L"main-source.json","{\"name\":\"main\",\"commit\":{\"sha\":\""+source+"\"}}");
    Put(pending/L"main-admission.json","{\"status\":\"identical\",\"url\":\"https://api.github.com/repos/Ding-Ding-Projects/TrafficMonitor/compare/"+source+"..."+source+"\",\"base_commit\":{\"sha\":\""+source+"\"},\"merge_base_commit\":{\"sha\":\""+source+"\"}}");
    Replace(pending/L"RELEASES",feed);Replace(pending/L"TrafficMonitor-1.0.32-full.nupkg",package);
}
unsigned Attempts(){return GetPrivateProfileIntW(L"update",L"attempts",0,(Staging(Root())/L"1.0.32.attempts.ini").c_str());}
bool WaitValidated(HelperPreparation& p)
{
    auto end=GetTickCount64()+15000;
    while(GetTickCount64()<end){auto s=p.Observe();if(s==PreparationState::Validated)return true;if(s==PreparationState::Failed)return false;Sleep(10);}
    return false;
}
void Settle(HelperPreparation& p){p.Cancel(ERROR_CANCELLED);if(p.process.get())Expect(WaitForSingleObject(p.process.get(),15000)==WAIT_OBJECT_0,"helper cooperatively exits");}
int main(int argc,char** argv)
{
    int helperResult{};if(HandleHelper(helperResult))return helperResult;
    try
    {
        Fixture();
        if(argc>1&&std::string(argv[1])=="create-failure")
        {HelperPreparation p;Expect(!SpawnHelper(p),"helper creation fails");Expect(Attempts()==0,"helper creation failure consumes no apply budget");return failures?1:0;}
        for(int i=0;i<3;++i)
        {
            HelperPreparation p;Expect(SpawnHelper(p),"valid preparation helper starts");Expect(WaitValidated(p),"valid bytes receive readiness");Settle(p);
            Expect(Attempts()==0,"repeated cancellation consumes no apply budget");
        }
        for(auto file:{L"RELEASES",L"TrafficMonitor-1.0.32-full.nupkg"})
        {
            // Independent corrupt-input cases must not inherit the intentionally
            // exhausted baseline's unrelated preparation counter.
            fs::remove(Staging(Root())/L"1.0.32.attempts.ini");
            Fixture();auto path=Staging(Root())/L"pending"/file;auto bytes=Read(path,1024);bytes[0]^=1;Replace(path,bytes);
            HelperPreparation p;Expect(SpawnHelper(p),"damaged staging helper starts");bool validated=WaitValidated(p);
            Expect(!validated,"corrupt feed/package never acknowledges parent exit");Settle(p);Expect(Attempts()==0,"corrupt staging starts no apply transaction");
        }
#ifndef NATIVE_UPDATE_BASELINE
        Fixture();
        service=std::make_shared<State>();service->enabled=true;service->root=Root();service->status.phase=Phase::Ready;
        for(int i=0;i<2;++i)
        {
            Expect(!PrepareRestart(),"initial preparation never exits parent");
            Sleep(HelperPreparationTimeout+200);
            Expect(!PrepareRestart(),"expired permission never exits parent");
            Expect(GetStatus().phase==Phase::Ready&&Attempts()==0,"expired preparation stays retryable without apply charge");
            if(preparation){Settle(*preparation);preparation.reset();}
            nextPreparation=0;
        }
        service.reset();
        Fixture();auto release=Pending(Root());int calls{};
        auto fail=[&]()->DWORD{++calls;return ERROR_GEN_FAILURE;};
        Expect(ApplyWithBudget(Root(),release,fail)==ERROR_GEN_FAILURE,"first real apply failure recorded");
        Expect(ApplyWithBudget(Root(),release,fail)==ERROR_GEN_FAILURE,"second real apply failure recorded");
        Expect(ApplyWithBudget(Root(),release,fail)==ERROR_RETRY,"third real apply rejected");
        Expect(calls==2&&Attempts()==2,"exactly two apply callbacks and durable attempts");
#endif
    }
    catch(const std::exception& e){std::cout<<"Fixture exception: "<<e.what()<<std::endl;Expect(false,"unexpected exception");}
    std::cout<<"Protocol checks: "<<failures<<" failed"<<std::endl;return failures?1:0;
}
