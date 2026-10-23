#define NOMINMAX
#include "SilentUpdater.h"
#include "Transport.h"
#include "Json.h"
#include "../InstallerLifecycle.h"
#include <filesystem>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <sstream>
#include <array>
#include <algorithm>

namespace NativeUpdate
{
    namespace
    {
        namespace fs=std::filesystem;
        using namespace Detail;
        constexpr size_t PackageLimit=256*1024*1024;
        constexpr size_t MetadataLimit=2*1024*1024;
        struct State
        {
            std::atomic<bool> cancel{false}, networkCancel{false};
            std::mutex mutex;std::condition_variable wake;
            bool enabled{},check{true};Status status;
            DWORD httpStatus{};fs::path root;const wchar_t* stage{L"startup"};
        };
        std::mutex serviceMutex;
        std::shared_ptr<State> service;
        struct Version
        {
            std::array<unsigned,3> numbers{};
            explicit Version(const std::string& text)
            {
                std::istringstream stream(text);char dot{};
                for(size_t i=0;i<3;++i)
                {
                    if(!(stream>>numbers[i])||numbers[i]>1000000)throw std::runtime_error("version");
                    if(i<2&&(!(stream>>dot)||dot!='.'))throw std::runtime_error("version separator");
                }
                if(stream.peek()!=EOF)throw std::runtime_error("version suffix");
                std::string normalized=std::to_string(numbers[0])+"."+std::to_string(numbers[1])+"."+std::to_string(numbers[2]);
                if(normalized!=text)throw std::runtime_error("version spelling");
            }
        };
        struct Release
        {
            std::string version,tag,source,package,packageHash,releasesHash,releasesSha1;
            unsigned long long packageSize{},releasesSize{};
            Json metadata;
        };
        std::string Text(const std::vector<unsigned char>& data)
        { return std::string(data.begin(),data.end()); }
        std::vector<unsigned char> Bytes(const std::string& text)
        { return std::vector<unsigned char>(text.begin(),text.end()); }
        bool Hex(const std::string& text,size_t length)
        { return text.size()==length&&text.find_first_not_of("0123456789abcdef")==std::string::npos; }
        fs::path Root()
        {
            fs::path module(InstallerLifecycle::ModuleDirectory());
            if(module.filename().empty())module=module.parent_path();
            auto root=module.parent_path();
            if(!InstallerLifecycle::IsManaged()||module.filename().wstring().find(L"app-")!=0||!PlainDirectory(root)||!fs::is_regular_file(root/L"Update.exe"))throw std::runtime_error("installation");
            return root;
        }
        fs::path Staging(const fs::path& root)
        {
            auto path=root/L"native-update";
            fs::create_directory(path);
            if(!PlainDirectory(path))throw std::runtime_error("staging");return path;
        }
        std::string CurrentVersion()
        {
            auto manifest=Read(fs::path(InstallerLifecycle::ModuleDirectory())/L"build-manifest.json",65536);
            // PowerShell 5 writes a UTF-8 BOM.
            if(manifest.size()>=3&&manifest[0]==0xef&&manifest[1]==0xbb&&manifest[2]==0xbf)manifest.erase(manifest.begin(),manifest.begin()+3);
            auto value=Reader(Text(manifest)).parse();
            if(value.at("architecture").text()!="x64"||value.at("squirrel").text()!="2.0.1")throw std::runtime_error("channel");
            auto version=value.at("version").text();Version parsed(version);return version;
        }
        unsigned long long Integer(const Json& value)
        {
            if(value.type!=Json::Number||value.value.empty()||value.value.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("integer");
            size_t end{};auto number=std::stoull(value.value,&end);if(end!=value.value.size())throw std::runtime_error("integer trailing");return number;
        }
        Release ParseRelease(const std::vector<unsigned char>& bytes,const std::string& current,bool requireNewer=true)
        {
            Release result;result.metadata=Reader(Text(bytes)).parse();auto& json=result.metadata;
            if(json.at("draft").flag()||json.at("prerelease").flag())throw std::runtime_error("release channel");
            result.tag=json.at("tag_name").text();
            // Only normal releases produced by the owning build pipeline are admitted.
            if(result.tag.empty()||result.tag[0]!='v')throw std::runtime_error("tag");
            auto split=result.tag.find("-run");if(split==std::string::npos)throw std::runtime_error("tag channel");
            result.version=result.tag.substr(1,split-1);Version version(result.version),installed(current);
            if(requireNewer&&!(version.numbers>installed.numbers))throw std::runtime_error("not newer");
            auto suffix=result.tag.substr(split+4);
            auto dash=suffix.find('-');if(dash==std::string::npos||dash==0||dash==suffix.size()-1||suffix.find_first_not_of("0123456789-")!=std::string::npos||suffix.find('-',dash+1)!=std::string::npos)throw std::runtime_error("run tag");
            result.source=json.at("target_commitish").text();if(!Hex(result.source,40))throw std::runtime_error("source");
            const std::string owner="https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/";
            if(json.at("html_url").text()!=owner+"tag/"+result.tag)throw std::runtime_error("owner");
            result.package="TrafficMonitor-"+result.version+"-full.nupkg";
            auto& assets=json.at("assets");if(assets.type!=Json::Array)throw std::runtime_error("assets");
            bool package=false,releases=false;
            for(auto& asset:assets.array)
            {
                auto name=asset.at("name").text();if(name!=result.package&&name!="RELEASES")continue;
                if(asset.at("state").text()!="uploaded"||asset.at("browser_download_url").text()!=owner+"download/"+result.tag+"/"+name)throw std::runtime_error("asset source");
                auto digest=asset.at("digest").text();if(digest.compare(0,7,"sha256:")!=0||!Hex(digest.substr(7),64))throw std::runtime_error("asset hash");
                auto size=Integer(asset.at("size"));if(!size||size>(name=="RELEASES"?65536:PackageLimit))throw std::runtime_error("asset size");
                if(name=="RELEASES") {if(releases)throw std::runtime_error("duplicate asset");releases=true;result.releasesHash=digest.substr(7);result.releasesSize=size;}
                else {if(package)throw std::runtime_error("duplicate asset");package=true;result.packageHash=digest.substr(7);result.packageSize=size;}
            }
            if(!package||!releases)throw std::runtime_error("missing assets");return result;
        }
        struct ValidationError : std::runtime_error
        {
            const wchar_t* stage;
            explicit ValidationError(const wchar_t* operation):std::runtime_error("validation"),stage(operation){}
        };
        void ValidateFeed(Release& release,const std::vector<unsigned char>& releases,const std::vector<unsigned char>& package)
        {
            if(releases.size()!=release.releasesSize)throw ValidationError(L"feed-size");
            if(Digest(releases)!=release.releasesHash)throw ValidationError(L"feed-sha256");
            if(package.size()!=release.packageSize)throw ValidationError(L"package-size");
            if(Digest(package)!=release.packageHash)throw ValidationError(L"package-sha256");
            // A single full package and exact safe basename prevents feed path escapes,
            // deltas and unrelated packages from reaching Squirrel's local source reader.
            // PowerShell-generated Squirrel feeds may carry a UTF-8 BOM. Verify
            // the original bytes above before skipping only this encoding marker
            // for row parsing; never normalize the bytes used by either digest.
            auto feed=Text(releases);
            if(feed.size()>=3&&static_cast<unsigned char>(feed[0])==0xef&&static_cast<unsigned char>(feed[1])==0xbb&&static_cast<unsigned char>(feed[2])==0xbf)feed.erase(0,3);
            std::istringstream input(feed);std::string line,sha,name,extra;unsigned long long size{};unsigned entries{};
            while(std::getline(input,line))
            {
                if(line.empty()||line=="\r")continue;
                std::istringstream row(line);if(!(row>>sha>>name>>size)||(row>>extra)||name!=release.package||size!=release.packageSize||sha.size()!=40||sha.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)throw ValidationError(L"feed-format");
                // Squirrel emits uppercase SHA-1; hexadecimal spelling does not alter
                // the digest. Normalize only ASCII hex before the exact comparison.
                std::transform(sha.begin(),sha.end(),sha.begin(),[](unsigned char c){return c>='A'&&c<='F'?static_cast<char>(c-'A'+'a'):static_cast<char>(c);});
                if(++entries!=1)throw ValidationError(L"feed-entry-count");
                if(Digest(package,true)!=sha)throw ValidationError(L"package-sha1");release.releasesSha1=sha;
            }
            if(entries!=1)throw ValidationError(L"feed-entry-count");
        }
        std::string MainSource(const std::vector<unsigned char>& bytes)
        {
            auto branch=Reader(Text(bytes)).parse();
            if(branch.at("name").text()!="main")throw std::runtime_error("accepted branch");
            auto sha=branch.at("commit").at("sha").text();
            if(!Hex(sha,40))throw std::runtime_error("accepted source");return sha;
        }
        void ValidateAdmission(const Release& release,const std::vector<unsigned char>& main,const std::vector<unsigned char>& bytes)
        {
            auto mainSha=MainSource(main);
            auto comparison=Reader(Text(bytes)).parse();
            auto status=comparison.at("status").text();
            if((status!="ahead"&&status!="identical")
                ||comparison.at("url").text()!="https://api.github.com/repos/Ding-Ding-Projects/TrafficMonitor/compare/"+release.source+"..."+mainSha
                ||comparison.at("base_commit").at("sha").text()!=release.source
                ||comparison.at("merge_base_commit").at("sha").text()!=release.source)
                throw std::runtime_error("release outside accepted main channel");
        }
        Release Pending(const fs::path& root,bool requireNewer=true)
        {
            auto pending=Staging(root)/L"pending";
            if(!PlainDirectory(pending))throw std::runtime_error("pending directory");
            auto release=ParseRelease(Read(pending/L"release.json",MetadataLimit),CurrentVersion(),requireNewer);
            ValidateAdmission(release,Read(pending/L"main-source.json",65536),Read(pending/L"main-admission.json",MetadataLimit));
            auto releases=Read(pending/L"RELEASES",65536),package=Read(pending/Wide(release.package),PackageLimit);
            ValidateFeed(release,releases,package);return release;
        }
        void Publish(const std::shared_ptr<State>& state,Phase phase,const std::wstring& version={},DWORD error=0)
        {
            {std::lock_guard<std::mutex> lock(state->mutex);state->status={phase,version,error};}
            // Bounded local diagnostics contain fixed stage names and numeric results,
            // never response bodies, URLs, credentials or exception text. Diagnostics
            // are best effort and cannot turn a rejected update into success.
            try
            {
                auto file=Staging(state->root)/L"status.ini";
                auto attributes=GetFileAttributesW(file.c_str());
                if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)))return;
                WritePrivateProfileStringW(L"update",L"stage",state->stage,file.c_str());
                WritePrivateProfileStringW(L"update",L"phase",std::to_wstring(static_cast<unsigned>(phase)).c_str(),file.c_str());
                WritePrivateProfileStringW(L"update",L"error",std::to_wstring(error).c_str(),file.c_str());
                WritePrivateProfileStringW(L"update",L"http_status",std::to_wstring(state->httpStatus).c_str(),file.c_str());
            }catch(...){}
        }
        void Check(const std::shared_ptr<State>& state,const fs::path& root)
        {
            state->stage=L"pending";
            if(fs::exists(Staging(root)/L"pending"))
            {
                auto pending=Pending(root,false);
                if(Version(pending.version).numbers<=Version(CurrentVersion()).numbers)
                {
                    // Preserve fully validated stale staging after a supported external
                    // update. It must not block discovery or become a downgrade candidate.
                    state->stage=L"retire-installed-pending";
                    auto retained=Staging(root)/(L"retained-"+Wide(pending.version)+L"-"+std::to_wstring(GetTickCount64()));
                    if(!MoveFileExW((Staging(root)/L"pending").c_str(),retained.c_str(),MOVEFILE_WRITE_THROUGH))throw TransportError("retain pending",GetLastError());
                }
                else
                {
                    auto attempts=Staging(root)/(Wide(pending.version)+L".attempts.ini");
                    if(GetPrivateProfileIntW(L"update",L"attempts",0,attempts.c_str())>=2)
                        Publish(state,Phase::Failed,Wide(pending.version),GetPrivateProfileIntW(L"update",L"last_exit_code",ERROR_RETRY,attempts.c_str()));
                    else Publish(state,Phase::Ready,Wide(pending.version));return;
                }
            }
            state->stage=L"release-metadata";Publish(state,Phase::Checking);
            auto deadline=GetTickCount64()+120000;
            auto metadata=Fetch(L"https://api.github.com/repos/Ding-Ding-Projects/TrafficMonitor/releases/latest",MetadataLimit,state->networkCancel,deadline);
            auto json=Reader(Text(metadata)).parse();
            // No available update is a normal state, separate from invalid metadata.
            auto tag=json.at("tag_name").text();auto split=tag.find("-run");
            if(split!=std::string::npos&&tag.size()>1&&Version(tag.substr(1,split-1)).numbers<=Version(CurrentVersion()).numbers)
            {Publish(state,Phase::Idle);return;}
            state->stage=L"release-validation";
            auto release=ParseRelease(metadata,CurrentVersion());
            // A normal task-branch preservation release is not the accepted update
            // channel. Require its source to be contained in the owning main history.
            state->stage=L"main-source";
            auto main=Fetch(L"https://api.github.com/repos/Ding-Ding-Projects/TrafficMonitor/branches/main",65536,state->networkCancel,deadline);
            auto mainSha=MainSource(main);
            state->stage=L"main-admission";
            auto admission=Fetch(L"https://api.github.com/repos/Ding-Ding-Projects/TrafficMonitor/compare/"+Wide(release.source)+L"..."+Wide(mainSha),MetadataLimit,state->networkCancel,deadline);
            state->stage=L"admission-validation";
            ValidateAdmission(release,main,admission);
            Publish(state,Phase::Downloading,Wide(release.version));
            const auto base=L"https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/download/"+Wide(release.tag)+L"/";
            state->stage=L"release-feed";
            auto releases=Fetch(base+L"RELEASES",65536,state->networkCancel,deadline);
            state->stage=L"package";
            auto package=Fetch(base+Wide(release.package),PackageLimit,state->networkCancel,deadline);
            state->stage=L"integrity-validation";
            ValidateFeed(release,releases,package);
            if(state->cancel||state->networkCancel)throw std::runtime_error("cancel");
            state->stage=L"staging";
            auto staging=Staging(root);auto temporary=staging/(L"download-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
            fs::create_directory(temporary);
            Write(temporary/L"release.json",metadata);Write(temporary/L"main-source.json",main);Write(temporary/L"main-admission.json",admission);Write(temporary/L"RELEASES",releases);Write(temporary/Wide(release.package),package);
            if(state->cancel||state->networkCancel)throw std::runtime_error("cancel");
            if(!MoveFileExW(temporary.c_str(),(staging/L"pending").c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("publish");
            state->stage=L"ready";Publish(state,Phase::Ready,Wide(release.version));
        }
        void Worker(std::shared_ptr<State> state,fs::path root)
        {
            std::unique_lock<std::mutex> lock(state->mutex);
            while(!state->cancel)
            {
                if(!state->enabled){state->status.phase=Phase::Disabled;state->wake.wait(lock,[&]{return state->cancel||state->enabled;});continue;}
                state->check=false;state->networkCancel=false;state->httpStatus=0;auto started=GetTickCount64();lock.unlock();
                try{Check(state,root);}catch(const ValidationError& error){state->stage=error.stage;if(!state->cancel)Publish(state,Phase::Failed,{},ERROR_INVALID_DATA);}catch(const TransportError& error){state->httpStatus=error.httpStatus;if(!state->cancel)Publish(state,Phase::Failed,{},error.code);}catch(...){if(!state->cancel)Publish(state,Phase::Failed,{},ERROR_INVALID_DATA);}
                lock.lock();
                auto elapsed=GetTickCount64()-started;
                auto remaining=elapsed<300000?300000-elapsed:0;
                state->wake.wait_for(lock,std::chrono::milliseconds(remaining),[&]{return state->cancel||state->check||!state->enabled;});
            }
            state->status.phase=Phase::Stopped;
        }
        unsigned long long FileTimeValue(const FILETIME& value)
        {return (static_cast<unsigned long long>(value.dwHighDateTime)<<32)|value.dwLowDateTime;}
        DWORD Child(const fs::path& executable,const std::wstring& arguments,DWORD timeout)
        {
            std::wstring text=L"\""+executable.wstring()+L"\" "+arguments;std::vector<wchar_t> command(text.begin(),text.end());command.push_back(0);
            STARTUPINFOW start{};start.cb=sizeof(start);start.dwFlags=STARTF_USESHOWWINDOW;start.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
            if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,executable.parent_path().c_str(),&start,&process))return GetLastError();
            CloseHandle(process.hThread);
            DWORD result=ERROR_TIMEOUT;
            if(!timeout)result=ERROR_SUCCESS;
            else if(WaitForSingleObject(process.hProcess,timeout)==WAIT_OBJECT_0) {if(!GetExitCodeProcess(process.hProcess,&result))result=GetLastError();}
            CloseHandle(process.hProcess);return result;
        }
        void HelperDiagnostic(const fs::path& root,const wchar_t* section,const wchar_t* stage,DWORD result)
        {
            // Fixed stages and numeric codes only. This file records the last operation,
            // not a heartbeat, and cannot affect admission or process identity checks.
            try
            {
                auto file=Staging(root)/L"helper-status.ini";
                auto attributes=GetFileAttributesW(file.c_str());
                if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)))return;
                if(stage)WritePrivateProfileStringW(section,L"stage",stage,file.c_str());
                WritePrivateProfileStringW(section,L"result",std::to_wstring(result).c_str(),file.c_str());
            }catch(...){}
        }
        bool SpawnHelper()
        {
            auto root=Root();auto staging=Staging(root);
            // The helper revalidates all package bytes. UI preparation only reads the
            // bounded manifest, avoiding a package-sized hash operation on the UI thread.
            auto release=ParseRelease(Read(staging/L"pending"/L"release.json",MetadataLimit),CurrentVersion());
            // Attempts survive application restarts and are bounded per immutable version.
            auto attemptsFile=staging/(Wide(release.version)+L".attempts.ini");
            int attempts=GetPrivateProfileIntW(L"update",L"attempts",0,attemptsFile.c_str());
            if(attempts>=2){HelperDiagnostic(root,L"launch",L"attempt-limit",ERROR_RETRY);return false;}
            FILETIME creation{},exit{},kernel{},user{};if(!GetProcessTimes(GetCurrentProcess(),&creation,&exit,&kernel,&user)){HelperDiagnostic(root,L"launch",L"parent-times",GetLastError());return false;}
            HANDLE identity=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,TRUE,GetCurrentProcessId());
            if(!identity){HelperDiagnostic(root,L"launch",L"parent-handle",GetLastError());return false;}
            std::wstring arguments=L"--native-update-helper "+std::to_wstring(GetCurrentProcessId())+L" "+std::to_wstring(FileTimeValue(creation))+L" "+std::to_wstring(reinterpret_cast<ULONG_PTR>(identity));
            // The helper keeps the old version's application-local runtime available until
            // Squirrel finishes. No shell, elevation or credential-bearing URL is involved.
            auto executable=fs::path(InstallerLifecycle::ModuleDirectory())/L"TrafficMonitor.exe";
            // Inherit one identity-bound process handle so even an immediate parent exit
            // cannot race helper startup. No unrelated inheritable handle is forwarded.
            SIZE_T bytes{};InitializeProcThreadAttributeList(nullptr,1,0,&bytes);
            std::vector<unsigned char> storage(bytes);
            auto attributes=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
            bool initialized=InitializeProcThreadAttributeList(attributes,1,0,&bytes)!=FALSE;
            bool ready=initialized&&UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,&identity,sizeof(identity),nullptr,nullptr);
            STARTUPINFOEXW start{};start.StartupInfo.cb=sizeof(start);start.StartupInfo.dwFlags=STARTF_USESHOWWINDOW;start.StartupInfo.wShowWindow=SW_HIDE;start.lpAttributeList=attributes;
            PROCESS_INFORMATION process{};
            std::wstring text=L"\""+executable.wstring()+L"\" "+arguments;std::vector<wchar_t> command(text.begin(),text.end());command.push_back(0);
            if(fs::exists(attemptsFile)&&(GetFileAttributesW(attemptsFile.c_str())&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))){SetLastError(ERROR_ACCESS_DENIED);ready=false;}
            if(ready)ready=WritePrivateProfileStringW(L"update",L"attempts",std::to_wstring(attempts+1).c_str(),attemptsFile.c_str())!=FALSE;
            DWORD preparation=ready?ERROR_SUCCESS:GetLastError();
            bool launched=ready&&CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,executable.parent_path().c_str(),&start.StartupInfo,&process);
            DWORD launchResult=launched?ERROR_SUCCESS:(ready?GetLastError():preparation);
            if(initialized)DeleteProcThreadAttributeList(attributes);CloseHandle(identity);
            HelperDiagnostic(root,L"launch",launched?L"created":L"create-process",launchResult);
            if(launched){CloseHandle(process.hThread);CloseHandle(process.hProcess);}return launched;
        }
        int RunHelper(DWORD pid,unsigned long long expectedCreation,HANDLE old)
        {
            auto root=Root();auto staging=Staging(root);
            HelperDiagnostic(root,L"helper",L"entry",STILL_ACTIVE);
            auto rootText=root.wstring();
            std::vector<unsigned char> identityBytes(reinterpret_cast<const unsigned char*>(rootText.data()),reinterpret_cast<const unsigned char*>(rootText.data()+rootText.size()));
            std::wstring mutexName=L"Local\\TrafficMonitor.NativeUpdate."+Wide(Digest(identityBytes));
            HANDLE mutex=CreateMutexW(nullptr,TRUE,mutexName.c_str());
            if(!mutex||GetLastError()==ERROR_ALREADY_EXISTS){if(mutex)CloseHandle(mutex);return ERROR_BUSY;}
            struct Close {HANDLE value;~Close(){if(value)CloseHandle(value);}} mutexOwner{mutex};
            HelperDiagnostic(root,L"helper",L"parent-identity",STILL_ACTIVE);
            if(!old||GetProcessId(old)!=pid)return ERROR_INVALID_HANDLE;Close oldOwner{old};
            FILETIME creation{},exit{},kernel{},user{};
            if(!GetProcessTimes(old,&creation,&exit,&kernel,&user)||FileTimeValue(creation)!=expectedCreation)return ERROR_INVALID_DATA;
            wchar_t image[32768]{};DWORD length=32768;
            const auto executable=fs::path(InstallerLifecycle::ModuleDirectory())/L"TrafficMonitor.exe";
            if(!QueryFullProcessImageNameW(old,0,image,&length)||_wcsicmp(image,executable.c_str()))return ERROR_INVALID_DATA;
            HelperDiagnostic(root,L"helper",L"parent-exit",STILL_ACTIVE);
            if(WaitForSingleObject(old,120000)!=WAIT_OBJECT_0)return ERROR_TIMEOUT;
            struct Recovery
            {
                fs::path root;bool launch{true};
                ~Recovery(){if(launch)Child(root/L"Update.exe",L"--processStart TrafficMonitor.exe",15000);}
            } recovery{root};
            // Hold the same normal-instance mutex across update application. A concurrent
            // launch cannot start monitoring or apply a second staged package.
            HANDLE application=CreateMutexW(nullptr,TRUE,L"TrafficMonitor-1419J3XLKL1w8OZc");
            if(!application||GetLastError()==ERROR_ALREADY_EXISTS){if(application)CloseHandle(application);recovery.launch=false;return ERROR_BUSY;}
            Close applicationOwner{application};
            HelperDiagnostic(root,L"helper",L"pending-validation",STILL_ACTIVE);
            auto release=Pending(root);auto attemptsFile=staging/(Wide(release.version)+L".attempts.ini");
            int attempts=GetPrivateProfileIntW(L"update",L"attempts",0,attemptsFile.c_str());
            if(attempts<1||attempts>2)return ERROR_RETRY;
            HelperDiagnostic(root,L"helper",L"squirrel-update",STILL_ACTIVE);
            DWORD result=Child(root/L"Update.exe",L"--update \""+(staging/L"pending").wstring()+L"\" --silent",180000);
            WritePrivateProfileStringW(L"update",L"last_exit_code",std::to_wstring(result).c_str(),attemptsFile.c_str());
            // Never overlap a still-running updater after timeout or kill a child.
            if(result==ERROR_TIMEOUT){recovery.launch=false;return result;}
            if(result==ERROR_SUCCESS)
            {
                auto target=root/(L"app-"+Wide(release.version))/L"TrafficMonitor.exe";
                if(!fs::is_regular_file(target))result=ERROR_FILE_NOT_FOUND;
                else
                {
                    auto completed=staging/(L"applied-"+Wide(release.version));
                    if(!MoveFileExW((staging/L"pending").c_str(),completed.c_str(),MOVEFILE_WRITE_THROUGH))result=GetLastError();
                }
            }
            // The stable launcher selects the latest usable installed version even after
            // an unsuccessful update. Failed attempts are never reported as success.
            ReleaseMutex(application);CloseHandle(application);applicationOwner.value=nullptr;
            recovery.launch=false;
            HelperDiagnostic(root,L"helper",L"stable-launcher",STILL_ACTIVE);
            DWORD launch=Child(root/L"Update.exe",L"--processStart TrafficMonitor.exe",15000);
            return result==ERROR_SUCCESS?launch:result;
        }
    }
    void Start(bool enabled)
    {
        std::lock_guard<std::mutex> guard(serviceMutex);if(service)return;
#if defined(_M_X64) && !defined(_DEBUG)
        if(!InstallerLifecycle::IsManaged())return;
        auto state=std::make_shared<State>();state->enabled=enabled;service=state;
        try
        {
            state->root=Root();state->status.phase=enabled?Phase::Idle:Phase::Disabled;
            Publish(state,state->status.phase);
            std::thread(Worker,state,state->root).detach();
        }
        catch(...){state->status={Phase::Failed,{},ERROR_INVALID_DATA};}
#endif
    }
    void SetEnabled(bool enabled)
    {
        std::lock_guard<std::mutex> guard(serviceMutex);if(!service)return;
        std::lock_guard<std::mutex> lock(service->mutex);if(service->enabled==enabled)return;service->enabled=enabled;service->networkCancel=!enabled;service->check=true;service->wake.notify_one();
    }
    void RequestCheck()
    {
        std::lock_guard<std::mutex> guard(serviceMutex);if(!service)return;
        std::lock_guard<std::mutex> lock(service->mutex);service->check=true;service->wake.notify_one();
    }
    void Stop()
    {
        std::lock_guard<std::mutex> guard(serviceMutex);if(!service)return;service->cancel=true;service->networkCancel=true;service->wake.notify_one();
        // Detached worker owns only its shared state and immutable installation path.
        // Keep this session single-flight, including while cancellation settles.
    }
    Status GetStatus()
    {
        std::lock_guard<std::mutex> guard(serviceMutex);if(!service)return {};
        std::lock_guard<std::mutex> lock(service->mutex);auto status=service->status;if(!service->enabled)status.phase=Phase::Disabled;return status;
    }
    bool PrepareRestart()
    {
        std::lock_guard<std::mutex> guard(serviceMutex);if(!service)return false;
        std::lock_guard<std::mutex> lock(service->mutex);
        if(!service->enabled||service->status.phase!=Phase::Ready||!AutomaticRestartEnabled())return false;
        try{if(!SpawnHelper()){service->status.phase=Phase::Failed;service->status.error=ERROR_RETRY;return false;}service->status.phase=Phase::Applying;service->cancel=true;service->networkCancel=true;service->wake.notify_one();return true;}catch(...){service->status.phase=Phase::Failed;service->status.error=ERROR_INVALID_DATA;return false;}
    }
    bool ApplyPendingOnLaunch()
    {try{if(!fs::exists(Staging(Root())/L"pending"))return false;return SpawnHelper();}catch(...){return false;}}
    bool AutomaticRestartEnabled()
    {
        try{return GetPrivateProfileIntW(L"update",L"automatic_restart",1,(Staging(Root())/L"preferences.ini").c_str())!=0;}catch(...){return false;}
    }
    bool SaveAutomaticRestart(bool enabled)
    {
        try
        {
            auto file=Staging(Root())/L"preferences.ini";
            if(fs::exists(file)&&(GetFileAttributesW(file.c_str())&FILE_ATTRIBUTE_REPARSE_POINT))return false;
            return WritePrivateProfileStringW(L"update",L"automatic_restart",enabled?L"1":L"0",file.c_str())&&AutomaticRestartEnabled()==enabled;
        }catch(...){return false;}
    }
    bool HandleHelper(int& exitCode)
    {
        int count{};auto arguments=CommandLineToArgvW(GetCommandLineW(),&count);if(!arguments)return false;
        bool helper=count>1&&std::wstring(arguments[1])==L"--native-update-helper";
        if(helper)
        {
            exitCode=ERROR_INVALID_PARAMETER;
            try
            {
                if(count==5)
                {
                    std::wstring pidText=arguments[2],timeText=arguments[3],handleText=arguments[4];
                    if(!pidText.empty()&&!timeText.empty()&&!handleText.empty()&&pidText.find_first_not_of(L"0123456789")==std::wstring::npos&&timeText.find_first_not_of(L"0123456789")==std::wstring::npos&&handleText.find_first_not_of(L"0123456789")==std::wstring::npos)
                    {auto pid=std::stoull(pidText);if(pid&&pid<=MAXDWORD&&pid!=GetCurrentProcessId())exitCode=RunHelper(static_cast<DWORD>(pid),std::stoull(timeText),reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(std::stoull(handleText))));}
                }
            }catch(...){exitCode=ERROR_INVALID_DATA;}
            try{HelperDiagnostic(Root(),L"helper",nullptr,static_cast<DWORD>(exitCode));}catch(...){}
        }
        LocalFree(arguments);return helper;
    }
}
