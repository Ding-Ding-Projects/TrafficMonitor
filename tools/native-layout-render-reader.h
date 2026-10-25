#pragma once
// Included after the observer helpers. Never reads arbitrary target memory or text.

namespace RendererRead {
using namespace NativeLayoutObservation;
struct SectionBasic { void* base; ULONG attributes; LARGE_INTEGER maximum; };
using QuerySection = LONG(NTAPI*)(HANDLE, int, void*, SIZE_T, SIZE_T*);
inline std::uint64_t SectionBytes(HANDLE mapping) {
    const auto query=reinterpret_cast<QuerySection>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQuerySection"));
    Need(query!=nullptr,"renderer-section-query-unavailable");
    SectionBasic info{};SIZE_T returned{};
    Need(query(mapping,0,&info,sizeof(info),&returned)>=0&&returned==sizeof(info)&&info.maximum.QuadPart>0,"renderer-section-size-query");
    return static_cast<std::uint64_t>(info.maximum.QuadPart);
}
inline void PrivateAcl(HANDLE mapping) {
    Handle token;Need(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.h)!=FALSE,"renderer-token");
    DWORD bytes=0;GetTokenInformation(token.h,TokenUser,nullptr,0,&bytes);std::vector<BYTE> data(bytes);
    Need(GetTokenInformation(token.h,TokenUser,data.data(),bytes,&bytes)!=FALSE,"renderer-user");
    PACL acl=nullptr;PSECURITY_DESCRIPTOR sd=nullptr;
    Need(GetSecurityInfo(mapping,SE_KERNEL_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&acl,nullptr,&sd)==ERROR_SUCCESS,"renderer-acl-read");
    bool safe=acl&&acl->AceCount>0;
    if(acl)for(DWORD i=0;i<acl->AceCount;++i){void* ace=nullptr;if(!GetAce(acl,i,&ace)){safe=false;break;}auto* header=static_cast<ACE_HEADER*>(ace);if(header->AceType!=ACCESS_ALLOWED_ACE_TYPE||(header->AceFlags&INHERITED_ACE)){safe=false;break;}auto* allowed=static_cast<ACCESS_ALLOWED_ACE*>(ace);if(!EqualSid(&allowed->SidStart,reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid)){safe=false;break;}}
    LocalFree(sd);Need(safe,"renderer-acl-not-current-user-only");
}
inline std::uint64_t Now(){LARGE_INTEGER q;Need(QueryPerformanceCounter(&q)!=FALSE,"renderer-clock");return q.QuadPart;}
inline std::string NumericRect(const NativeLayoutObservation::Rect& r) {
    for(float n:{r.left,r.top,r.right,r.bottom})Need(std::isfinite(n)&&std::abs(n)<=1000000,"renderer-nonfinite-rectangle");
    return "{\"left\":"+std::to_string(r.left)+",\"top\":"+std::to_string(r.top)+",\"right\":"+std::to_string(r.right)+",\"bottom\":"+std::to_string(r.bottom)+"}";
}
inline bool Contains(const NativeLayoutObservation::Rect& a,const NativeLayoutObservation::Rect& b){return b.left>=a.left&&b.top>=a.top&&b.right<=a.right&&b.bottom<=a.bottom;}
inline void ValidateNumeric(const Record& r) {
    Need(r.sequence&&r.reserved==0&&r.reserved2==0,"renderer-record-sequence");
    constexpr std::uint32_t known=Geometry|Font|TextMetrics|Overhang|Transform|Clip|Scroll|FallbackUnavailable|NativePaintUnobserved|FontNotApplicable|TextArea|MeasurementOnly|MetricsUnavailable;
    Need(!(r.validity&~known)&&r.route>=Route::NotPaint&&r.route<=Route::PrintClient&&r.outcome>=Outcome::NotPaint&&r.outcome<=Outcome::NotReported,"renderer-record-flags");
    Need(r.visible<=1&&r.enabled<=1&&r.font.source>=FontSource::Unavailable&&r.font.source<=FontSource::NotApplicable,"renderer-record-state");
    for(float f:r.transform)Need(std::isfinite(f),"renderer-transform");
    for(float f:{r.font.size,r.font.height,r.font.ascent,r.font.descent,r.font.externalLeading,r.layoutWidth,r.layoutHeight,r.widthIncludingTrailingWhitespace})Need(std::isfinite(f),"renderer-numeric-metric");
    NumericRect(r.clientPixels);NumericRect(r.textArea);NumericRect(r.measured);NumericRect(r.overhang);NumericRect(r.clipBox);
    Need(std::find(std::begin(r.font.family),std::end(r.font.family),L'\0')!=std::end(r.font.family),"renderer-font-family-bounds");
    if(r.validity&Scroll)Need(r.content>=0&&r.page>=0&&r.position>=0&&r.position<=std::max<std::int64_t>(0,r.content-r.page),"renderer-scroll-range");
}
struct Session {
    Handle mapping;const Snapshot* view{};Snapshot snapshot{};std::uint64_t minimum{},maxAge{},begin{},end{};std::set<std::uint64_t> observed,missing;
    ~Session(){if(view)UnmapViewOfFile(view);}
    void OpenRead(const std::wstring& name) {
        mapping.h=OpenFileMappingW(FILE_MAP_READ|0x0001|READ_CONTROL,FALSE,name.c_str());
        Need(mapping.h!=nullptr,"renderer-mapping-missing");PrivateAcl(mapping.h);
        SYSTEM_INFO system{};GetSystemInfo(&system);Need(system.dwPageSize>0,"renderer-page-size");
        const auto expected=(sizeof(Snapshot)+system.dwPageSize-1)/system.dwPageSize*system.dwPageSize;
        Need(SectionBytes(mapping.h)==expected,"renderer-section-extent");
        view=static_cast<const Snapshot*>(MapViewOfFile(mapping.h,FILE_MAP_READ,0,0,sizeof(Snapshot)));Need(view!=nullptr,"renderer-map-read");
    }
    void Begin(Target& target,std::uint64_t minSequence,std::uint64_t maxAgeMs,Writer& output) {
        Need(minSequence>0&&maxAgeMs>0&&maxAgeMs<=10000,"renderer-freshness-arguments");minimum=minSequence;
        const DWORD marker=GetFileAttributesW((target.path+MarkerSuffix).c_str());Need(marker!=INVALID_FILE_ATTRIBUTES&&!(marker&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)),"renderer-opt-in-missing");
        const auto name=std::wstring(MappingPrefix)+std::to_wstring(target.pid)+L"."+std::to_wstring(target.creation);
        OpenRead(name);const auto bytes=SectionBytes(mapping.h);SYSTEM_INFO system{};GetSystemInfo(&system);
        Need(CopyStable(view,snapshot)==ReadResult::Ready,"renderer-torn-header-or-overflow");
        LARGE_INTEGER frequency;Need(QueryPerformanceFrequency(&frequency)!=FALSE&&snapshot.header.qpcFrequency==static_cast<std::uint64_t>(frequency.QuadPart),"renderer-clock-frequency");
        maxAge=snapshot.header.qpcFrequency*maxAgeMs/1000;begin=Now();
        Need(ValidateFresh(snapshot,target.pid,target.creation,minimum,begin,maxAge)==ReadResult::Ready,"renderer-foreign-or-stale-header");
        output.Write("{\"phase\":\"renderer.interval.begin\",\"sequence\":\""+std::to_string(snapshot.header.sequence)+"\",\"qpc\":\""+std::to_string(begin)+"\",\"publishedQpc\":\""+std::to_string(snapshot.header.publishedQpc)+"\",\"minimumSequence\":\""+std::to_string(minimum)+"\",\"maximumAgeTicks\":\""+std::to_string(maxAge)+"\",\"qpcFrequency\":\""+std::to_string(snapshot.header.qpcFrequency)+"\",\"logicalBytes\":"+std::to_string(sizeof(Snapshot))+",\"sectionBytes\":"+std::to_string(bytes)+",\"pageBytes\":"+std::to_string(system.dwPageSize)+",\"receiptComplete\":false}");
    }
    void Control(Target& target,HWND hwnd,Writer& output) {
        if(!IsWindowVisible(hwnd))return;const auto id=reinterpret_cast<std::uint64_t>(hwnd);std::set<std::uint32_t> kinds;bool found=false;
        RECT client;Need(GetClientRect(hwnd,&client)!=FALSE,"renderer-client-read");POINT origin{};Need(ClientToScreen(hwnd,&origin)!=FALSE,"renderer-origin-read");const auto dpi=GetDpiForWindow(hwnd);DWORD pid;const auto thread=GetWindowThreadProcessId(hwnd,&pid);
        for(std::uint32_t i=0;i<snapshot.header.count;++i){const auto& r=snapshot.records[i];if(r.hwnd!=id)continue;found=true;
            Need(ValidateRecord(r,minimum,begin,maxAge)==ReadResult::Ready,"renderer-stale-or-invalid-record");ValidateNumeric(r);
            Need(r.sequence<=snapshot.header.sequence&&r.qpc<=snapshot.header.publishedQpc,"renderer-record-outside-publication");
            Need(kinds.insert(static_cast<std::uint32_t>(r.kind)).second,"renderer-duplicate-record");
            Need(r.dpi==dpi&&r.threadId==thread&&pid==target.pid&&r.parentHwnd==reinterpret_cast<std::uint64_t>(GetParent(hwnd))&&r.controlId==static_cast<std::uint32_t>(GetDlgCtrlID(hwnd)),"renderer-control-identity-or-dpi");
            Need(r.visible==1&&r.enabled==static_cast<std::uint32_t>(IsWindowEnabled(hwnd)!=FALSE),"renderer-control-state-changed");
            if(r.validity&Geometry)Need(r.clientPixels.left==client.left&&r.clientPixels.top==client.top&&r.clientPixels.right==client.right&&r.clientPixels.bottom==client.bottom,"renderer-client-geometry-changed");
            observed.insert(id);
            // Preserve actual units and flags, not a fabricated resolved-font claim.
            output.Write("{\"phase\":\"renderer.record\",\"hwnd\":"+Id(hwnd)+",\"registration\":\""+std::to_string(r.registration)+"\",\"sequence\":\""+std::to_string(r.sequence)+"\",\"paintSequence\":\""+std::to_string(r.paintSequence)+"\",\"qpc\":\""+std::to_string(r.qpc)+"\",\"kind\":"+std::to_string(static_cast<unsigned>(r.kind))+",\"units\":"+std::to_string(static_cast<unsigned>(r.units))+",\"route\":"+std::to_string(static_cast<unsigned>(r.route))+",\"outcome\":"+std::to_string(static_cast<unsigned>(r.outcome))+",\"paintResult\":"+std::to_string(r.paintResult)+",\"dpi\":"+std::to_string(r.dpi)+",\"validity\":"+std::to_string(r.validity)+",\"clientPixels\":"+NumericRect(r.clientPixels)+",\"textArea\":"+NumericRect(r.textArea)+",\"measured\":"+NumericRect(r.measured)+",\"overhang\":"+NumericRect(r.overhang)+",\"fontSource\":"+std::to_string(static_cast<unsigned>(r.font.source))+",\"fontFamily\":"+Json(r.font.family)+",\"fontHeight\":"+std::to_string(r.font.height)+",\"fontAscent\":"+std::to_string(r.font.ascent)+",\"fontDescent\":"+std::to_string(r.font.descent)+",\"content\":"+std::to_string(r.content)+",\"page\":"+std::to_string(r.page)+",\"position\":"+std::to_string(r.position)+",\"originX\":"+std::to_string(origin.x)+",\"originY\":"+std::to_string(origin.y)+",\"measuredOverflow\":null,\"overflowAssessment\":\"deferred-origin-contract\""+",\"strictFontComplete\":false,\"receiptComplete\":false}");
            std::string transform="[";for(int k=0;k<6;++k){if(k)transform+=",";transform+=std::to_string(r.transform[k]);}transform+="]";
            output.Write("{\"phase\":\"renderer.record.details\",\"hwnd\":"+Id(hwnd)+",\"sequence\":\""+std::to_string(r.sequence)+"\",\"kind\":"+std::to_string(static_cast<unsigned>(r.kind))+",\"clipBox\":"+NumericRect(r.clipBox)+",\"clipSource\":"+std::to_string(static_cast<unsigned>(r.clipSource))+",\"mappingMode\":"+std::to_string(r.mappingMode)+",\"transform\":"+transform+",\"fontSize\":"+std::to_string(r.font.size)+",\"fontWeight\":"+std::to_string(r.font.weight)+",\"fontItalic\":"+std::to_string(r.font.italic)+",\"fontCharacterSet\":"+std::to_string(r.font.characterSet)+",\"fontExternalLeading\":"+std::to_string(r.font.externalLeading)+",\"layoutWidth\":"+std::to_string(r.layoutWidth)+",\"layoutHeight\":"+std::to_string(r.layoutHeight)+",\"widthIncludingTrailingWhitespace\":"+std::to_string(r.widthIncludingTrailingWhitespace)+",\"lineCount\":"+std::to_string(r.lineCount)+",\"bidiDepth\":"+std::to_string(r.bidiDepth)+"}");
        }
        if(!found){missing.insert(id);output.Write("{\"phase\":\"renderer.missing\",\"hwnd\":"+Id(hwnd)+",\"reason\":\"no-renderer-record-for-visible-control\",\"receiptComplete\":false}");}
    }
    void End(Target& target,Writer& output) {
        Snapshot current{};Need(CopyStable(view,current)==ReadResult::Ready,"renderer-torn-end");end=Now();
        Need(current.header.sequence==snapshot.header.sequence&&std::memcmp(&snapshot,&current,sizeof(snapshot))==0,"renderer-changed-during-structural-observation");
        Need(ValidateFresh(current,target.pid,target.creation,minimum,end,maxAge)==ReadResult::Ready,"renderer-stale-end");
        output.Write("{\"phase\":\"renderer.interval.end\",\"sequence\":\""+std::to_string(current.header.sequence)+"\",\"qpc\":\""+std::to_string(end)+"\",\"observedControls\":"+std::to_string(observed.size())+",\"receiptComplete\":false}");
        Need(missing.empty(),"renderer-missing-visible-observation");
    }
};
inline void SelfTest() {
    Snapshot source{},copy{};auto& h=source.header;
    h.magic=Magic;h.version=Version;h.headerBytes=sizeof(Header);h.recordBytes=sizeof(Record);h.capacity=Capacity;
    h.processId=41;h.processCreation=123;h.qpcFrequency=1000;h.sequence=4;h.publishedQpc=100;
    Need(CopyStable(&source,copy)==ReadResult::Ready,"test-renderer-stable");
    h.sequence=3;Need(CopyStable(&source,copy)==ReadResult::Torn,"test-renderer-torn");h.sequence=4;
    h.flags=Overflow;Need(CopyStable(&source,copy)==ReadResult::Overflowed,"test-renderer-overflow");h.flags=0;
    Need(ValidateFresh(source,42,123,2,100,50)==ReadResult::WrongProcess,"test-renderer-foreign");
    Need(ValidateFresh(source,41,123,6,100,50)==ReadResult::Stale,"test-renderer-old-sequence");
    Need(ValidateFresh(source,41,123,2,200,50)==ReadResult::Stale,"test-renderer-old-time");
    Record r{};r.hwnd=1;r.registration=1;r.sequence=4;r.qpc=100;r.dpi=96;r.kind=Kind::GdiMeasurement;r.units=Units::PhysicalClientPixels;
    Need(ValidateRecord(r,2,100,50)==ReadResult::Ready,"test-renderer-record");r.qpc=1;
    Need(ValidateRecord(r,2,100,50)==ReadResult::Stale,"test-renderer-stale-record");
    const NativeLayoutObservation::Rect area{0,0,100,40},overflow{0,0,101,40};
    Need(!Contains(area,overflow),"test-renderer-bounds");
    Handle mapping;mapping.h=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Snapshot),nullptr);
    Need(mapping.h!=nullptr,"test-renderer-section");
    std::printf("Renderer transport checks: 9 passed; section requested=%zu observed=%llu; synthetic only.\n",sizeof(Snapshot),SectionBytes(mapping.h));
    Handle token;Need(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.h)!=FALSE,"test-reader-token");
    DWORD count=0;GetTokenInformation(token.h,TokenUser,nullptr,0,&count);std::vector<BYTE> tokenBytes(count);
    Need(GetTokenInformation(token.h,TokenUser,tokenBytes.data(),count,&count)!=FALSE,"test-reader-user");
    LPWSTR sid=nullptr;Need(ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(tokenBytes.data())->User.Sid,&sid)!=FALSE,"test-reader-sid");
    std::wstring sddl=L"D:P(A;;GA;;;"+std::wstring(sid)+L")";LocalFree(sid);PSECURITY_DESCRIPTOR sd=nullptr;
    Need(ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&sd,nullptr)!=FALSE,"test-reader-security");
    SECURITY_ATTRIBUTES sa{sizeof(sa),sd,FALSE};
    const auto name=L"Local\\TrafficMonitor.LayoutReaderTest."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64());
    Handle creator;creator.h=CreateFileMappingW(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE,0,sizeof(Snapshot),name.c_str());
    Handle wrong;wrong.h=CreateFileMappingW(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE,0,sizeof(Snapshot)+65536,(name+L".wrong").c_str());
    LocalFree(sd);Need(creator.h&&wrong.h,"test-reader-create");
    auto* writable=static_cast<Snapshot*>(MapViewOfFile(creator.h,FILE_MAP_WRITE,0,0,sizeof(Snapshot)));Need(writable!=nullptr,"test-reader-fill");
    *writable=source;UnmapViewOfFile(writable);
    Session reader;reader.OpenRead(name);Need(CopyStable(reader.view,copy)==ReadResult::Ready,"test-consumer-read");
    void* forbidden=MapViewOfFile(reader.mapping.h,FILE_MAP_WRITE,0,0,sizeof(Snapshot));
    if(forbidden)UnmapViewOfFile(forbidden);Need(forbidden==nullptr,"test-consumer-write-access");
    bool rejected=false;try{Session other;other.OpenRead(name+L".wrong");}catch(const Failure& f){rejected=std::string(f.phase)=="renderer-section-extent";}
    Need(rejected,"test-consumer-wrong-extent");rejected=false;
    try{Session other;other.OpenRead(name+L".missing");}catch(const Failure& f){rejected=std::string(f.phase)=="renderer-mapping-missing";}
    Need(rejected,"test-consumer-missing");
    std::puts("Renderer consumer checks: 4 passed; named mapping read, write denial, wrong extent and missing mapping.");
}
}
