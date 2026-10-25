#include "LayoutObservations.h"
#include <sddl.h>
#include <array>
#include <mutex>
#include <string>
#include <vector>
#pragma comment(lib,"advapi32.lib")
namespace NativeLayoutObservation
{
namespace {
    struct Registration { HWND window{},root{};std::uint64_t identity{}; };
    std::uint64_t Ticks(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return value.QuadPart;}
    std::uint64_t FileTime(FILETIME value){return (std::uint64_t(value.dwHighDateTime)<<32)|value.dwLowDateTime;}
    struct Store
    {
        HANDLE handle{};Snapshot* data{};std::mutex mutex;
        std::array<Registration,Capacity> registrations{};std::uint64_t nextRegistration{},nextPaint{};
        Store() noexcept
        {
            try {
                wchar_t executable[32768]{};const DWORD length=GetModuleFileNameW(nullptr,executable,_countof(executable));
                if(!length||length>=_countof(executable))return;
                const std::wstring marker=std::wstring(executable,length)+MarkerSuffix;
                const DWORD attributes=GetFileAttributesW(marker.c_str());
                if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))return;
                FILETIME creation{},exit{},kernel{},user{};if(!GetProcessTimes(GetCurrentProcess(),&creation,&exit,&kernel,&user))return;
                HANDLE token{};if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return;
                DWORD bytes{};GetTokenInformation(token,TokenUser,nullptr,0,&bytes);
                std::vector<BYTE> storage(bytes);const BOOL obtained=GetTokenInformation(token,TokenUser,storage.data(),bytes,&bytes);CloseHandle(token);if(!obtained)return;
                LPWSTR sid{};if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(storage.data())->User.Sid,&sid))return;
                const std::wstring sddl=L"D:P(A;;GR;;;"+std::wstring(sid)+L")";LocalFree(sid);
                PSECURITY_DESCRIPTOR descriptor{};
                if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&descriptor,nullptr))return;
                SECURITY_ATTRIBUTES security{sizeof(security),descriptor,FALSE};
                const std::wstring name=std::wstring(MappingPrefix)+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(FileTime(creation));
                handle=CreateFileMappingW(INVALID_HANDLE_VALUE,&security,PAGE_READWRITE,0,sizeof(Snapshot),name.c_str());
                const DWORD result=GetLastError();LocalFree(descriptor);
                if(!handle)return;if(result==ERROR_ALREADY_EXISTS){CloseHandle(handle);handle=nullptr;return;}
                data=static_cast<Snapshot*>(MapViewOfFile(handle,FILE_MAP_WRITE,0,0,sizeof(Snapshot)));
                if(!data){CloseHandle(handle);handle=nullptr;return;}
                LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
                data->header={Magic,Version,sizeof(Header),sizeof(Record),Capacity,0,0,GetCurrentProcessId(),FileTime(creation),static_cast<std::uint64_t>(frequency.QuadPart),0,0};
                Begin();End();
            }catch(...){if(data){UnmapViewOfFile(data);data=nullptr;}if(handle){CloseHandle(handle);handle=nullptr;}}
        }
        ~Store(){if(data)UnmapViewOfFile(data);if(handle)CloseHandle(handle);}
        void Begin(){InterlockedIncrement64(reinterpret_cast<volatile LONG64*>(&data->header.sequence));MemoryBarrier();}
        void End(){data->header.publishedQpc=Ticks();MemoryBarrier();InterlockedIncrement64(reinterpret_cast<volatile LONG64*>(&data->header.sequence));}
        std::uint64_t Find(HWND window){for(const auto& r:registrations)if(r.window==window&&reinterpret_cast<std::uint64_t>(GetPropW(window,RegistrationProperty))==r.identity)return r.identity;return 0;}
        void DropRecords(HWND window)
        {
            for(std::uint32_t i=0;i<data->header.count;)
            {if(data->records[i].hwnd==reinterpret_cast<std::uint64_t>(window)){data->records[i]=data->records[--data->header.count];data->records[data->header.count]={};}else ++i;}
        }
        void Publish(Record& record)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(!data||!record.registration||Find(reinterpret_cast<HWND>(record.hwnd))!=record.registration)return;
            Begin();std::uint32_t index=0;
            while(index<data->header.count&&(data->records[index].hwnd!=record.hwnd||data->records[index].kind!=record.kind))++index;
            if(index==Capacity){data->header.flags|=Overflow;End();return;}
            if(index==data->header.count)++data->header.count;
            record.sequence=data->header.sequence+1;record.qpc=Ticks();std::memcpy(&data->records[index],&record,sizeof(record));End();
        }
    };
    Store& State(){static Store store;return store;}
    thread_local PaintScope* painting{};
    bool Base(HWND window,Record& record)
    {
        auto& state=State();if(!state.data||!window)return false;
        std::lock_guard<std::mutex> lock(state.mutex);const auto registration=state.Find(window);if(!registration)return false;
        // Clear padding as well as fields before anything can enter transport.
        std::memset(&record,0,sizeof(record));
        DWORD pid{};const DWORD thread=GetWindowThreadProcessId(window,&pid);RECT client{};
        if(pid!=GetCurrentProcessId()||!GetClientRect(window,&client))return false;
        record.hwnd=reinterpret_cast<std::uint64_t>(window);record.parentHwnd=reinterpret_cast<std::uint64_t>(GetParent(window));
        record.registration=registration;record.controlId=GetDlgCtrlID(window);record.threadId=thread;record.dpi=GetDpiForWindow(window);
        record.visible=IsWindowVisible(window)!=FALSE;record.enabled=IsWindowEnabled(window)!=FALSE;
        record.clientPixels={float(client.left),float(client.top),float(client.right),float(client.bottom)};
        record.validity=Geometry;record.units=Units::PhysicalClientPixels;return record.dpi!=0;
    }
    void GdiState(Record& record,HDC dc)
    {
        record.mappingMode=GetMapMode(dc);RECT clip{};
        const int type=GetClipBox(dc,&clip);
        if(type!=ERROR&&record.mappingMode==MM_TEXT)
        {POINT points[2]{{clip.left,clip.top},{clip.right,clip.bottom}};if(LPtoDP(dc,points,2)){record.clipBox={float(points[0].x),float(points[0].y),float(points[1].x),float(points[1].y)};record.clipSource=ClipSource::GdiClipBox;record.validity|=Clip;}}
        POINT viewport{},window{};XFORM transform{1,0,0,1,0,0};
        const bool world=GetGraphicsMode(dc)==GM_COMPATIBLE||GetWorldTransform(dc,&transform);
        if(record.mappingMode==MM_TEXT&&world&&GetViewportOrgEx(dc,&viewport)&&GetWindowOrgEx(dc,&window))
        {record.transform[0]=transform.eM11;record.transform[1]=transform.eM12;record.transform[2]=transform.eM21;record.transform[3]=transform.eM22;record.transform[4]=transform.eDx+viewport.x-window.x;record.transform[5]=transform.eDy+viewport.y-window.y;record.validity|=Transform;}
    }
    void GdiFont(Record& record,HDC dc)
    {
        TEXTMETRICW metrics{};LOGFONTW font{};const HGDIOBJ selected=GetCurrentObject(dc,OBJ_FONT);
        if(!selected||!GetTextMetricsW(dc,&metrics)||!GetObjectW(selected,sizeof(font),&font))return;
        record.font.source=FontSource::SelectedGdiFont;record.font.weight=metrics.tmWeight;record.font.italic=metrics.tmItalic;record.font.characterSet=metrics.tmCharSet;
        record.font.size=float(abs(font.lfHeight));record.font.height=float(metrics.tmHeight);record.font.ascent=float(metrics.tmAscent);record.font.descent=float(metrics.tmDescent);record.font.externalLeading=float(metrics.tmExternalLeading);
        const int faceLength=GetTextFaceW(dc,0,nullptr);
        if(faceLength>0&&faceLength<=_countof(record.font.family))GetTextFaceW(dc,_countof(record.font.family),record.font.family);
        else record.validity|=MetricsUnavailable;
        record.validity|=Font|FallbackUnavailable;
    }
}
    bool Enabled() noexcept{return State().data!=nullptr;}
    std::uint64_t Register(HWND window) noexcept
    {
        auto& state=State();if(!state.data||!IsWindow(window))return 0;
        DWORD pid{};GetWindowThreadProcessId(window,&pid);if(pid!=GetCurrentProcessId())return 0;
        std::lock_guard<std::mutex> lock(state.mutex);if(const auto existing=state.Find(window))return existing;
        // Window properties disappear on destruction. A reused numeric HWND
        // cannot inherit the prior registration even if destruction was missed.
        for(auto& r:state.registrations)if(r.window&&!state.Find(r.window)){state.Begin();state.DropRecords(r.window);r={};state.End();}
        for(auto& r:state.registrations)if(!r.window)
        {
            const auto identity=++state.nextRegistration;
            if(!SetPropW(window,RegistrationProperty,reinterpret_cast<HANDLE>(identity)))return 0;
            r={window,GetAncestor(window,GA_ROOT),identity};return identity;
        }
        state.Begin();state.data->header.flags|=Overflow;state.End();return 0;
    }
    void Unregister(HWND window) noexcept
    {
        auto& state=State();if(!state.data||!window)return;
        std::lock_guard<std::mutex> lock(state.mutex);state.Begin();
        for(auto& r:state.registrations)if(r.window&&(r.window==window||r.root==window))
        {
            if(reinterpret_cast<std::uint64_t>(GetPropW(r.window,RegistrationProperty))==r.identity)RemovePropW(r.window,RegistrationProperty);
            state.DropRecords(r.window);r={};
        }
        state.End();
    }
    void RegisterChild(HWND window,HWND parent) noexcept
    {Record record{};if(Base(parent,record))Register(window);}
    void Invalidate(HWND window) noexcept
    {auto& state=State();if(!state.data||!window)return;std::lock_guard<std::mutex> lock(state.mutex);if(!state.Find(window))return;state.Begin();state.DropRecords(window);state.End();}
    void ObserveGdiMeasurement(HWND window,HDC dc,const RECT& area,const RECT& measured) noexcept
    {
        Record record{};if(!Base(window,record)||!dc)return;
        record.kind=Kind::GdiMeasurement;record.validity|=MeasurementOnly|NativePaintUnobserved|TextArea|TextMetrics;
        record.textArea={float(area.left),float(area.top),float(area.right),float(area.bottom)};
        record.measured={float(measured.left),float(measured.top),float(measured.right),float(measured.bottom)};
        GdiState(record,dc);GdiFont(record,dc);State().Publish(record);
    }
    void ObserveNativeText(HWND window) noexcept
    {
        Record record{};if(!Base(window,record))return;
        HDC dc=GetDC(window);if(!dc)return;
        const auto font=reinterpret_cast<HFONT>(SendMessageW(window,WM_GETFONT,0,0));const auto previous=font?SelectObject(dc,font):nullptr;
        record.kind=Kind::NativeTextMetrics;record.validity|=NativePaintUnobserved|MeasurementOnly;
        GdiState(record,dc);if(font&&previous&&previous!=HGDI_ERROR)GdiFont(record,dc);
        RECT area{};GetClientRect(window,&area);wchar_t className[32]{};GetClassNameW(window,className,_countof(className));
        const bool edit=_wcsicmp(className,L"EDIT")==0,staticText=_wcsicmp(className,L"STATIC")==0;
        if(edit)SendMessageW(window,EM_GETRECT,0,reinterpret_cast<LPARAM>(&area));
        if(edit||staticText){record.textArea={float(area.left),float(area.top),float(area.right),float(area.bottom)};record.validity|=TextArea;}
        // This observes the selected font and the native text rectangle only.
        // The native control's glyph fallback, line layout and paint remain unknown.
        record.validity|=MetricsUnavailable;State().Publish(record);
        if(previous&&previous!=HGDI_ERROR)SelectObject(dc,previous);ReleaseDC(window,dc);
    }
    void ObserveRail(HWND window,int content,int page,int position) noexcept
    {
        Record record{};if(!Base(window,record))return;record.kind=Kind::RailMetrics;record.units=Units::LogicalScrollUnits;
        record.font.source=FontSource::NotApplicable;record.validity|=Scroll|FontNotApplicable;
        record.content=content;record.page=page;record.position=position;State().Publish(record);
    }
    PaintScope::PaintScope(HWND window,HDC dc,Route route) noexcept
    {
        // An unregistered nested control must not inherit another control's
        // observation scope, even if painting is reentrant.
        m_previous=painting;painting=nullptr;
        if(!Base(window,m_record))return;
        auto& state=State();{std::lock_guard<std::mutex> lock(state.mutex);m_record.paintSequence=++state.nextPaint;}
        m_record.kind=Kind::NonTextPaint;m_record.font.source=FontSource::NotApplicable;m_record.validity|=FontNotApplicable;
        m_record.route=route;if(dc)GdiState(m_record,dc);painting=this;m_active=true;
    }
    PaintScope::~PaintScope(){if(m_active&&!m_complete)Finish(Outcome::NotReported,E_PENDING);else if(!m_active)painting=m_previous;}
    void PaintScope::Finish(Outcome outcome,HRESULT result) noexcept
    {if(!m_active||m_complete)return;m_record.outcome=outcome;m_record.paintResult=result;State().Publish(m_record);painting=m_previous;m_complete=true;}
    void PaintScope::Complete(HRESULT result) noexcept{Finish(SUCCEEDED(result)?Outcome::Succeeded:Outcome::Failed,result);}
    void PaintScope::NotReported() noexcept{Finish(Outcome::NotReported,E_PENDING);}
    void ObserveDirectWrite(ID2D1RenderTarget* target,IDWriteTextLayout* layout,IDWriteTextFormat* format,D2D1_POINT_2F origin) noexcept
    {
        if(!painting||!layout||!format||!target)return;auto& record=painting->m_record;
        record.kind=Kind::DirectWritePaint;record.units=Units::DeviceIndependentPixels;
        target->GetDpi(&record.renderDpiX,&record.renderDpiY);
        record.validity&=~FontNotApplicable;record.validity|=FallbackUnavailable;
        record.font.source=FontSource::RequestedDirectWriteFormat;record.font.size=format->GetFontSize();record.font.weight=format->GetFontWeight();record.font.italic=format->GetFontStyle()!=DWRITE_FONT_STYLE_NORMAL;
        if(format->GetFontFamilyNameLength()<_countof(record.font.family)&&SUCCEEDED(format->GetFontFamilyName(record.font.family,_countof(record.font.family))))record.validity|=Font;
        DWRITE_TEXT_METRICS metrics{};
        if(SUCCEEDED(layout->GetMetrics(&metrics)))
        {record.measured={origin.x+metrics.left,origin.y+metrics.top,origin.x+metrics.left+metrics.width,origin.y+metrics.top+metrics.height};record.layoutWidth=metrics.layoutWidth;record.layoutHeight=metrics.layoutHeight;record.widthIncludingTrailingWhitespace=metrics.widthIncludingTrailingWhitespace;record.lineCount=metrics.lineCount;record.bidiDepth=metrics.maxBidiReorderingDepth;record.textArea={origin.x,origin.y,origin.x+metrics.layoutWidth,origin.y+metrics.layoutHeight};record.validity|=TextMetrics|TextArea;}
        DWRITE_OVERHANG_METRICS overhang{};if(SUCCEEDED(layout->GetOverhangMetrics(&overhang))){record.overhang={overhang.left,overhang.top,overhang.right,overhang.bottom};record.validity|=Overhang;}
        D2D1_MATRIX_3X2_F transform{};target->GetTransform(&transform);record.transform[0]=transform._11;record.transform[1]=transform._12;record.transform[2]=transform._21;record.transform[3]=transform._22;record.transform[4]=transform._31;record.transform[5]=transform._32;record.validity|=Transform;
    }
}
