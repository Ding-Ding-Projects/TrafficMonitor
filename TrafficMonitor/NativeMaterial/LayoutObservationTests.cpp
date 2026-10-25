#include "../stdafx.h"
#include "ButtonVisual.h"
#include "LayoutObservations.h"
#include "ScrollRail.h"
#include <aclapi.h>
#include <cstdio>
#include <memory>
namespace NativeMaterial {void DrawButtonLegacy(ID2D1RenderTarget*,const Palette&,const CString&,ButtonVariant,bool,bool,bool,bool);}
namespace {
CWinApp application;int failures{};
void Check(bool value,const char* name){std::printf("%s %s\n",value?"PASS":"FAIL",name);if(!value)++failures;}
std::uint64_t Tick(){LARGE_INTEGER n{};QueryPerformanceCounter(&n);return n.QuadPart;}
std::uint64_t Creation(){FILETIME c{},e{},k{},u{};GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u);return (std::uint64_t(c.dwHighDateTime)<<32)|c.dwLowDateTime;}
std::vector<DWORD> Render(HWND window,bool legacy,const CString& label,NativeMaterial::ButtonVariant variant,bool enabled,bool focused,UINT dpi,int width)
{
    const int height=56;const int w=MulDiv(width,dpi,96),h=MulDiv(height,dpi,96);
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* pixels{};HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto previous=SelectObject(dc,bitmap);
    Microsoft::WRL::ComPtr<ID2D1Factory> factory;Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
    HRESULT result=D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,factory.GetAddressOf());
    const auto properties=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE),float(dpi),float(dpi));
    if(SUCCEEDED(result))result=factory->CreateDCRenderTarget(&properties,target.GetAddressOf());
    RECT bounds{0,0,w,h};if(SUCCEEDED(result))result=target->BindDC(dc,&bounds);
    if(SUCCEEDED(result))
    {
        NativeLayoutObservation::PaintScope observation(window,dc,NativeLayoutObservation::Route::PrintClient);
        target->BeginDraw();const auto c=NativeMaterial::Palette::Resolve(NativeMaterial::Appearance::Light);target->Clear(c.surface);
        if(legacy)NativeMaterial::DrawButtonLegacy(target.Get(),c,label,variant,enabled,false,true,focused);
        else NativeMaterial::DrawButton(target.Get(),c,label,variant,enabled,false,true,focused);
        result=target->EndDraw();observation.Complete(result);
    }
    Check(SUCCEEDED(result),"real render completes");GdiFlush();std::vector<DWORD> output(static_cast<DWORD*>(pixels),static_cast<DWORD*>(pixels)+w*h);
    target.Reset();factory.Reset();SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);return output;
}
}
int wmain(int argc,wchar_t**)
{
    using namespace NativeLayoutObservation;using namespace NativeMaterial;
    if(!AfxWinInit(GetModuleHandle(nullptr),nullptr,GetCommandLine(),0))return 2;
    const bool expected=argc>1;Check(Enabled()==expected,"explicit marker controls opt in");
    const auto name=std::wstring(MappingPrefix)+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(Creation());
    HANDLE mapping=OpenFileMappingW(FILE_MAP_READ|READ_CONTROL,FALSE,name.c_str());
    Check((mapping!=nullptr)==expected,"mapping absent without marker and readable with marker");
    CWnd parent;Check(parent.CreateEx(0,AfxRegisterWndClass(0),L"Observation test",WS_POPUP,0,0,500,500,nullptr,0)!=FALSE,"hidden fixture parent created");
    CStatic label;label.Create(L"fixture_sensitive_phrase_7B9A",WS_CHILD,CRect(0,0,160,56),&parent,100);
    const auto registration=Register(label.GetSafeHwnd());Check((registration!=0)==expected,"only opted in controls register");
    std::uint64_t checksum=1469598103934665603ull;int parityCases=0;
    for(UINT dpi:{96u,144u})for(int width:{96,200})for(auto variant:{ButtonVariant::Primary,ButtonVariant::Tonal,ButtonVariant::Outlined,ButtonVariant::Text,ButtonVariant::Selector})
    for(bool enabled:{false,true})for(bool focused:{false,true})
    for(const auto* text:{L"Apply",L"Details\r\n詳細資料",L"Long wrapped interface choice",L"fixture_sensitive_phrase_7B9A"})
    {
        const auto oldPixels=Render(label.GetSafeHwnd(),true,text,variant,enabled,focused,dpi,width);
        const auto newPixels=Render(label.GetSafeHwnd(),false,text,variant,enabled,focused,dpi,width);
        Check(oldPixels==newPixels,"original DrawText and actual DrawTextLayout pixel parity");++parityCases;
        for(DWORD pixel:newPixels){checksum^=pixel;checksum*=1099511628211ull;}
    }
    std::printf("PARITY cases=%d\nPIXEL_CHECKSUM %llu\n",parityCases,checksum);
    if(expected&&mapping)
    {
        auto* view=static_cast<const Snapshot*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(Snapshot)));Check(view!=nullptr,"read only transport view opens");
        if(view)
        {
            MEMORY_BASIC_INFORMATION region{};SYSTEM_INFO system{};GetSystemInfo(&system);VirtualQuery(view,&region,sizeof(region));
            const auto rounded=(sizeof(Snapshot)+system.dwPageSize-1)/system.dwPageSize*system.dwPageSize;
            Check(region.RegionSize==rounded,"section extent matches page rounded ABI size");
            auto snapshot=std::make_unique<Snapshot>();Check(CopyStable(view,*snapshot)==ReadResult::Ready,"completed snapshot copies without torn sequence");
            const Record* paint=nullptr;for(std::uint32_t i=0;i<snapshot->header.count;++i)if(snapshot->records[i].kind==Kind::DirectWritePaint)paint=&snapshot->records[i];
            Check(paint&&paint->registration==registration&&paint->outcome==Outcome::Succeeded&&(paint->validity&(TextMetrics|Overhang|FallbackUnavailable))==(TextMetrics|Overhang|FallbackUnavailable),"actual submitted layout publishes completed metrics and explicit fallback gap");
            Check(paint&&paint->font.source==FontSource::RequestedDirectWriteFormat&&paint->lineCount>0,"requested font stays distinct from resolved fallback");
            bool zeroPadding=true;for(std::uint32_t i=0;i<snapshot->header.count;++i){const auto* raw=reinterpret_cast<const BYTE*>(&snapshot->records[i]);for(size_t offset=offsetof(Record,renderDpiY)+sizeof(float);offset<offsetof(Record,content);++offset)zeroPadding=zeroPadding&&raw[offset]==0;}Check(zeroPadding,"wire padding contains only zero bytes");
            Check(paint&&paint->renderDpiX==144&&paint->renderDpiY==144&&paint->dpi==GetDpiForWindow(label.GetSafeHwnd()),"observed renderer DPI remains separate from HWND DPI");
            if(paint){Record malformed=*paint;malformed.sequence|=1;Check(ValidateRecord(malformed,0,Tick(),snapshot->header.qpcFrequency)==ReadResult::InvalidRecord,"odd completed record sequence rejected");malformed=*paint;malformed.transform[0]=std::numeric_limits<float>::quiet_NaN();Check(ValidateRecord(malformed,0,Tick(),snapshot->header.qpcFrequency)==ReadResult::InvalidRecord,"nonfinite renderer transform rejected");}
            const auto previousPaintSequence=paint?paint->sequence:0;
            CStatic unregistered;unregistered.Create(L"",WS_CHILD,CRect(0,0,160,56),&parent,102);
            {PaintScope outer(label.GetSafeHwnd(),nullptr,Route::WindowPaint);Render(unregistered.GetSafeHwnd(),false,L"Unregistered content",ButtonVariant::Primary,true,false,96,160);}
            CopyStable(view,*snapshot);bool contaminated=false;
            for(std::uint32_t i=0;i<snapshot->header.count;++i){const auto& r=snapshot->records[i];contaminated|=r.hwnd==reinterpret_cast<std::uint64_t>(unregistered.GetSafeHwnd());if(r.hwnd==reinterpret_cast<std::uint64_t>(label.GetSafeHwnd())&&r.kind==Kind::DirectWritePaint)contaminated|=r.sequence!=previousPaintSequence;}
            Check(!contaminated,"unregistered nested rendering cannot inherit parent observation");
            const auto now=Tick();Check(ValidateFresh(*snapshot,GetCurrentProcessId(),Creation(),snapshot->header.sequence,now,snapshot->header.qpcFrequency)==ReadResult::Ready,"fresh matching process accepted");
            Check(ValidateFresh(*snapshot,GetCurrentProcessId()+1,Creation(),0,now,snapshot->header.qpcFrequency)==ReadResult::WrongProcess,"wrong process rejected");
            Check(ValidateFresh(*snapshot,GetCurrentProcessId(),Creation(),snapshot->header.sequence+2,now,snapshot->header.qpcFrequency)==ReadResult::Stale,"stale sequence rejected");
            auto torn=std::make_unique<Snapshot>(*snapshot);torn->header.sequence|=1;Check(CopyStable(torn.get(),*snapshot)==ReadResult::Torn,"odd sequence rejected");
            torn->header.sequence+=1;torn->header.flags|=Overflow;Check(CopyStable(torn.get(),*snapshot)==ReadResult::Overflowed,"overflow snapshot rejected");
            torn->header.flags=0;torn->header.version=1;Check(CopyStable(torn.get(),*snapshot)==ReadResult::InvalidHeader,"previous protocol cannot impersonate renderer DPI observations");
            CopyStable(view,*snapshot);const std::wstring privateText=L"fixture_sensitive_phrase_7B9A";
            const auto* bytes=reinterpret_cast<const BYTE*>(snapshot.get());const auto* needle=reinterpret_cast<const BYTE*>(privateText.data());
            Check(std::search(bytes,bytes+sizeof(Snapshot),needle,needle+privateText.size()*sizeof(wchar_t))==bytes+sizeof(Snapshot),"transport contains no control text");
            CFont font;font.CreatePointFont(140,L"Segoe UI");label.SetFont(&font);ObserveNativeText(label.GetSafeHwnd());CopyStable(view,*snapshot);
            const Record* native=nullptr;for(std::uint32_t i=0;i<snapshot->header.count;++i)if(snapshot->records[i].kind==Kind::NativeTextMetrics)native=&snapshot->records[i];
            Check(native&&native->font.source==FontSource::SelectedGdiFont&&native->font.height>0&&(native->validity&NativePaintUnobserved),"local selected GDI metrics retain native paint exclusion");
            Invalidate(label.GetSafeHwnd());CopyStable(view,*snapshot);bool retained=false;for(std::uint32_t i=0;i<snapshot->header.count;++i)retained|=snapshot->records[i].hwnd==reinterpret_cast<std::uint64_t>(label.GetSafeHwnd());Check(!retained,"state invalidation removes stale observation kinds");
            Unregister(label.GetSafeHwnd());const auto next=Register(label.GetSafeHwnd());Check(next>registration&&reinterpret_cast<std::uint64_t>(GetPropW(label.GetSafeHwnd(),RegistrationProperty))==next,"new registration replaces prior lifetime identity");
            CopyStable(view,*snapshot);bool oldRecord=false;for(std::uint32_t i=0;i<snapshot->header.count;++i)oldRecord|=snapshot->records[i].registration==registration;Check(!oldRecord,"unregister removes every prior record kind");
            Register(parent.GetSafeHwnd());ScrollRail rail;rail.Create(&parent,101);rail.SetMetrics(300,100,99);CopyStable(view,*snapshot);
            const Record* scroll=nullptr;for(std::uint32_t i=0;i<snapshot->header.count;++i)if(snapshot->records[i].hwnd==reinterpret_cast<std::uint64_t>(rail.GetSafeHwnd())&&snapshot->records[i].kind==Kind::RailMetrics)scroll=&snapshot->records[i];
            Check(scroll&&scroll->content==300&&scroll->page==100&&scroll->position==99&&scroll->font.source==FontSource::NotApplicable,"actual custom rail state has no invented font");
            HANDLE writable=OpenFileMappingW(FILE_MAP_WRITE,FALSE,name.c_str());Check(!writable,"independent mapping handle cannot request write access");if(writable)CloseHandle(writable);
            PACL dacl{};PSECURITY_DESCRIPTOR descriptor{};const DWORD security=GetSecurityInfo(mapping,SE_KERNEL_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&dacl,nullptr,&descriptor);
            bool onlyCurrentUser=false;
            if(security==ERROR_SUCCESS&&dacl&&dacl->AceCount==1){void* ace{};GetAce(dacl,0,&ace);auto* allowed=static_cast<ACCESS_ALLOWED_ACE*>(ace);HANDLE token{};if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)){DWORD needed{};GetTokenInformation(token,TokenUser,nullptr,0,&needed);std::vector<BYTE> user(needed);if(GetTokenInformation(token,TokenUser,user.data(),needed,&needed))onlyCurrentUser=allowed->Header.AceType==ACCESS_ALLOWED_ACE_TYPE&&EqualSid(&allowed->SidStart,reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid);CloseHandle(token);}}
            Check(onlyCurrentUser,"transport DACL admits only the current user SID");if(descriptor)LocalFree(descriptor);
            std::vector<std::unique_ptr<CStatic>> controls;
            for(unsigned i=0;i<Capacity;++i){auto control=std::make_unique<CStatic>();control->Create(L"",WS_CHILD,CRect(0,0,1,1),&parent,200+i);Register(control->GetSafeHwnd());controls.push_back(std::move(control));}
            Check(CopyStable(view,*snapshot)==ReadResult::Overflowed,"real registration capacity overflow fails closed");
            UnmapViewOfFile(view);
        }
    }
    if(mapping)CloseHandle(mapping);Unregister(parent.GetSafeHwnd());parent.DestroyWindow();std::printf("RESULT failures=%d\n",failures);return failures?1:0;
}
