// Bounded, opt-in observation of one verified native disclosure control.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <UIAutomation.h>
#include <bcrypt.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace {
struct Failure { HRESULT result; };
void Require(bool value, HRESULT hr=E_FAIL) { if(!value) throw Failure{hr}; }
void Check(HRESULT hr) { if(FAILED(hr)) throw Failure{hr}; }
void WinCheck(BOOL value) {if(!value)throw Failure{HRESULT_FROM_WIN32(GetLastError())};}
struct Handle {
    HANDLE value{INVALID_HANDLE_VALUE};
    ~Handle(){if(value!=INVALID_HANDLE_VALUE&&value)CloseHandle(value);}
};
struct Log {
    Handle file; ULONGLONG start{GetTickCount64()};
    void Write(const char* phase,HRESULT hr=S_OK,long long value=0) {
        char line[384];const int length=sprintf_s(line,"{\"phase\":\"%s\",\"elapsedMs\":%llu,\"hresult\":%ld,\"value\":%lld}\n",phase,GetTickCount64()-start,static_cast<long>(hr),value);
        DWORD written=0;Require(length>0&&WriteFile(file.value,line,length,&written,nullptr)&&written==static_cast<DWORD>(length));FlushFileBuffers(file.value);
    }
};
uint64_t Number(const wchar_t* s) { wchar_t* end=nullptr;Require(s&&*s&&*s!=L'-');const auto n=_wcstoui64(s,&end,10);Require(end&&!*end);return n; }
std::wstring FullPath(const std::wstring& path) {std::vector<wchar_t> b(32768);const DWORD n=GetFullPathNameW(path.c_str(),static_cast<DWORD>(b.size()),b.data(),nullptr);Require(n&&n<b.size());return {b.data(),n};}
std::wstring DesktopName(HDESK desktop) {wchar_t name[512]{};DWORD needed=0;WinCheck(GetUserObjectInformationW(desktop,UOI_NAME,name,sizeof(name),&needed));return name;}
std::wstring Hash(const std::wstring& path) {
    Handle file;file.value=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Require(file.value!=INVALID_HANDLE_VALUE);
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    Require(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0);
    std::wstring result;
    try {
        Require(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0);
        BYTE buffer[65536],digest[32];DWORD read=0;
        for(;;){Require(ReadFile(file.value,buffer,sizeof(buffer),&read,nullptr)!=FALSE);if(!read)break;Require(BCryptHashData(hash,buffer,read,0)>=0);}
        Require(BCryptFinishHash(hash,digest,sizeof(digest),0)>=0);
        const wchar_t* hex=L"0123456789abcdef";for(BYTE b:digest){result+=hex[b>>4];result+=hex[b&15];}
    } catch(...) {if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);throw;}
    BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);return result;
}
struct Target {
    DWORD pid{};HWND window{};HDESK authorizedDesktop{};std::wstring desktop,path,hash;uint64_t creation{};Handle process;
    struct Membership {HWND root;DWORD pid;bool found{};};
    static BOOL CALLBACK FindRoot(HWND candidate,LPARAM context) {
        auto& membership=*reinterpret_cast<Membership*>(context);
        if(candidate==membership.root){DWORD owner=0;GetWindowThreadProcessId(candidate,&owner);membership.found=owner==membership.pid;}
        return TRUE;
    }
    void Validate(bool verifyHash,Log& log) {
        log.Write("identity.processAlive.begin");
        Require(WaitForSingleObject(process.value,0)==WAIT_TIMEOUT);
        log.Write("identity.creation.begin");
        FILETIME created,exit,kernel,user;Require(GetProcessTimes(process.value,&created,&exit,&kernel,&user)!=FALSE);
        Require(((uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime)==creation);
        log.Write("identity.path.begin");
        wchar_t actual[32768];DWORD size=_countof(actual);Require(QueryFullProcessImageNameW(process.value,0,actual,&size)!=FALSE);
        Require(_wcsicmp(FullPath(actual).c_str(),path.c_str())==0);
        if(verifyHash){log.Write("identity.hash.begin");Require(_wcsicmp(Hash(path).c_str(),hash.c_str())==0);}
        log.Write("identity.child.begin");
        DWORD owner=0;GetWindowThreadProcessId(window,&owner);
        Require(IsWindow(window)&&owner==pid&&GetDlgCtrlID(window)==56206);
        log.Write("identity.desktopMembership.begin");
        Membership membership{GetAncestor(window,GA_ROOT),pid,false};Require(membership.root!=nullptr);
        // Enumerate only the explicitly opened, authorized desktop. A borrowed
        // handle from another process's thread is not a usable access grant.
        WinCheck(EnumDesktopWindows(authorizedDesktop,FindRoot,reinterpret_cast<LPARAM>(&membership)));
        Require(membership.found);
        log.Write("identity.desktopMembership.ok");
    }
};
class Events final:public IUIAutomationPropertyChangedEventHandler {
    std::atomic<ULONG> references{1};std::mutex mutex;std::vector<int> values;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override {if(!out)return E_POINTER;*out=nullptr;if(id!=__uuidof(IUnknown)&&id!=__uuidof(IUIAutomationPropertyChangedEventHandler))return E_NOINTERFACE;*out=static_cast<IUIAutomationPropertyChangedEventHandler*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++references;}
    ULONG STDMETHODCALLTYPE Release()override{const auto n=--references;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent(IUIAutomationElement*,PROPERTYID id,VARIANT value)override {
        // UIA callbacks never use sender or call UIA. Only bounded primitive data is retained.
        if(id==UIA_ExpandCollapseExpandCollapseStatePropertyId&&value.vt==VT_I4){std::lock_guard<std::mutex> lock(mutex);if(values.size()<64)values.push_back(value.lVal);}return S_OK;
    }
    void Drain(Log& log){std::vector<int> copy;{std::lock_guard<std::mutex> lock(mutex);copy.swap(values);}for(int v:copy)log.Write("event.expandCollapse",S_OK,v);}
};
void Inspect(Target& target,Log& log,bool exercise) {
    ComPtr<IUIAutomation> automation;log.Write("uia.create.begin");
    Check(CoCreateInstance(CLSID_CUIAutomation,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&automation)));log.Write("uia.create.ok");
    ComPtr<IUIAutomationElement> element;log.Write("element.begin");Check(automation->ElementFromHandle(target.window,&element));
    int pid=0;UIA_HWND hwnd=nullptr;Check(element->get_CurrentProcessId(&pid));Check(element->get_CurrentNativeWindowHandle(&hwnd));
    Require(pid==static_cast<int>(target.pid)&&hwnd==target.window);log.Write("element.identity.ok");
    BOOL enabled=FALSE;Check(element->get_CurrentIsEnabled(&enabled));log.Write("element.enabled",S_OK,enabled);
    ComPtr<IUIAutomationExpandCollapsePattern> pattern;log.Write("pattern.begin");
    Check(element->GetCurrentPatternAs(UIA_ExpandCollapsePatternId,IID_PPV_ARGS(&pattern)));
    Require(pattern!=nullptr);ExpandCollapseState state;Check(pattern->get_CurrentExpandCollapseState(&state));log.Write("pattern.state",S_OK,state);
    if(!exercise)return;
    Require(enabled!=FALSE,UIA_E_ELEMENTNOTENABLED);
    ComPtr<Events> events;events.Attach(new Events());bool subscribed=false;
    try {
        PROPERTYID property=UIA_ExpandCollapseExpandCollapseStatePropertyId;log.Write("events.subscribe.begin");
        Check(automation->AddPropertyChangedEventHandlerNativeArray(element.Get(),TreeScope_Element,nullptr,events.Get(),&property,1));subscribed=true;log.Write("events.subscribe.ok");
        for(bool expand:{true,true,false,false}) {
            target.Validate(false,log);Check(pattern->get_CurrentExpandCollapseState(&state));log.Write("action.before",S_OK,state);
            log.Write(expand?"expand.begin":"collapse.begin");Check(expand?pattern->Expand():pattern->Collapse());
            const auto expected=expand?ExpandCollapseState_Expanded:ExpandCollapseState_Collapsed;const auto start=GetTickCount64();
            do {Sleep(25);Check(pattern->get_CurrentExpandCollapseState(&state));}while(state!=expected&&GetTickCount64()-start<1500);
            log.Write("action.after",S_OK,state);Require(state==expected,HRESULT_FROM_WIN32(ERROR_TIMEOUT));
            Sleep(150);events->Drain(log);
            // Read again after the queued-action settling interval to detect an extra toggle.
            Check(pattern->get_CurrentExpandCollapseState(&state));log.Write("action.settled",S_OK,state);Require(state==expected);
        }
    } catch(...) {if(subscribed){log.Write("events.remove.begin");const auto hr=automation->RemovePropertyChangedEventHandler(element.Get(),events.Get());log.Write("events.remove.result",hr);}throw;}
    log.Write("events.remove.begin");Check(automation->RemovePropertyChangedEventHandler(element.Get(),events.Get()));log.Write("events.remove.ok");events->Drain(log);
    // All client-owned COM interfaces are released here, on this MTA, before CoUninitialize.
}
int Worker(wchar_t** a) {
    Log log;HDESK desktop=nullptr;bool initialized=false;
    try {
        log.file.value=CreateFileW(a[6],GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);Require(log.file.value!=INVALID_HANDLE_VALUE);
        APTTYPE type;APTTYPEQUALIFIER qualifier;HRESULT hr=CoGetApartmentType(&type,&qualifier);log.Write("com.beforeAttach",hr);Require(hr==CO_E_NOTINITIALIZED);
        desktop=OpenDesktopW(a[1],0,FALSE,DESKTOP_READOBJECTS|DESKTOP_ENUMERATE);Require(desktop!=nullptr);
        log.Write("attach.begin");WinCheck(SetThreadDesktop(desktop));Require(DesktopName(GetThreadDesktop(GetCurrentThreadId()))==a[1]);log.Write("attach.ok");
        hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);log.Write("com.initialize",hr);Check(hr);initialized=true;
        Check(CoGetApartmentType(&type,&qualifier));Require(type==APTTYPE_MTA);log.Write("com.apartment",S_OK,type);
        Target target;const auto pid=Number(a[0]);Require(pid&&pid<=MAXDWORD);target.pid=static_cast<DWORD>(pid);target.window=reinterpret_cast<HWND>(static_cast<uintptr_t>(Number(a[2])));
        target.desktop=a[1];target.authorizedDesktop=desktop;target.path=FullPath(a[3]);target.creation=Number(a[4]);target.hash=a[5];Require(target.hash.size()==64);
        for(auto c:target.hash)Require(iswxdigit(c)!=0);
        log.Write("identity.openProcess.begin");target.process.value=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,target.pid);Require(target.process.value!=nullptr);
        target.Validate(true,log);log.Write("identity.ok",S_OK,target.pid);
        const std::wstring mode=a[7];Require(mode==L"read"||mode==L"exercise");Inspect(target,log,mode==L"exercise");
        target.Validate(true,log);log.Write("complete");CoUninitialize();initialized=false;CloseDesktop(desktop);return 0;
    } catch(const Failure& failure){if(log.file.value!=INVALID_HANDLE_VALUE)try{log.Write("error",failure.result);}catch(...){} }
      catch(...){if(log.file.value!=INVALID_HANDLE_VALUE)try{log.Write("error",E_UNEXPECTED);}catch(...){} }
    if(initialized)CoUninitialize();if(desktop)CloseDesktop(desktop);return 1;
}
std::wstring Quote(const wchar_t* value) {
    std::wstring result=L"\"";size_t slashes=0;
    for(const wchar_t* p=value;*p;++p){if(*p==L'\\'){++slashes;continue;}if(*p==L'\"'){result.append(slashes*2+1,L'\\');result+=*p;}else{result.append(slashes,L'\\');result+=*p;}slashes=0;}
    result.append(slashes*2,L'\\');return result+L"\"";
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc==11&&std::wstring(argv[1])==L"--worker")return Worker(argv+2);
    if(argc!=10){std::fputs("Usage: native-uia-observer PID DESKTOP HWND EXE CREATION_FILETIME SHA256 NEW_JSONL read|exercise TIMEOUT_SECONDS\n",stderr);return 2;}
    try {
        const auto seconds=Number(argv[9]);Require(seconds>=5&&seconds<=30);
        wchar_t executable[32768];const auto n=GetModuleFileNameW(nullptr,executable,_countof(executable));Require(n&&n<_countof(executable));
        std::wstring command=Quote(executable)+L" --worker";for(int i=1;i<argc;++i)command+=L" "+Quote(argv[i]);
        STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION child{};
        Require(CreateProcessW(executable,&command[0],nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)!=FALSE);
        Handle process;process.value=child.hProcess;Handle thread;thread.value=child.hThread;
        const DWORD waited=WaitForSingleObject(process.value,static_cast<DWORD>(seconds*1000));
        if(waited!=WAIT_OBJECT_0){Require(TerminateProcess(process.value,124)!=FALSE);WaitForSingleObject(process.value,2000);std::printf("{\"observerPid\":%lu,\"timeout\":true,\"targetTerminated\":false}\n",child.dwProcessId);return 124;}
        DWORD exit=1;Require(GetExitCodeProcess(process.value,&exit)!=FALSE);std::printf("{\"observerPid\":%lu,\"exitCode\":%lu}\n",child.dwProcessId,exit);return static_cast<int>(exit);
    }catch(...){std::fputs("Observer could not start with the supplied bounded arguments.\n",stderr);return 2;}
}
