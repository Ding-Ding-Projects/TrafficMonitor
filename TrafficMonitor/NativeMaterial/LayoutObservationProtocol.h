#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <cmath>
#include <initializer_list>

// Numeric-only, x64 Windows wire format. This is a partial renderer observation
// protocol, not a claim that the external strict layout schema is satisfied.
namespace NativeLayoutObservation
{
    constexpr std::uint32_t Magic=0x314f4c54, Version=1, Capacity=256;
    constexpr wchar_t MarkerSuffix[]=L".layout-observations";
    constexpr wchar_t MappingPrefix[]=L"Local\\TrafficMonitor.LayoutObservations.";
    constexpr wchar_t RegistrationProperty[]=L"TrafficMonitor.LayoutObservationRegistration";
    // Name: MappingPrefix + decimal PID + "." + decimal process creation FILETIME.
    enum class Kind : std::uint32_t { DirectWritePaint=1,GdiMeasurement=2,NativeTextMetrics=3,NonTextPaint=4,RailMetrics=5 };
    enum class Units : std::uint32_t { PhysicalClientPixels=1,DeviceIndependentPixels=2,LogicalScrollUnits=3 };
    enum class Route : std::uint32_t { NotPaint=0,WindowPaint=1,PrintClient=2 };
    enum class Outcome : std::uint32_t { NotPaint=0,Succeeded=1,Failed=2,NotReported=3 };
    enum class FontSource : std::uint32_t { Unavailable=0,SelectedGdiFont=1,RequestedDirectWriteFormat=2,NotApplicable=3 };
    enum class ClipSource : std::uint32_t { Unavailable=0,GdiClipBox=1 };
    enum Validity : std::uint32_t {
        Geometry=1,Font=2,TextMetrics=4,Overhang=8,Transform=16,Clip=32,Scroll=64,
        FallbackUnavailable=128,NativePaintUnobserved=256,FontNotApplicable=512,
        TextArea=1024,MeasurementOnly=2048,MetricsUnavailable=4096
    };
    enum TransportFlags : std::uint32_t { Overflow=1 };
    struct Rect { float left{},top{},right{},bottom{}; };
    struct FontMetrics {
        FontSource source{};std::uint32_t weight{},italic{},characterSet{};
        float size{},height{},ascent{},descent{},externalLeading{};
        // Font identity only. Never place control text, user wording or a path here.
        wchar_t family[32]{};
    };
    struct Record {
        std::uint64_t hwnd{},parentHwnd{},registration{},sequence{},paintSequence{},qpc{};
        std::uint32_t controlId{},threadId{},dpi{},validity{};
        Kind kind{};Units units{};Route route{};Outcome outcome{};
        std::int32_t paintResult{};std::uint32_t visible{},enabled{},reserved{};
        // clientPixels and clipBox always use physical device pixels. textArea
        // and measured use units. DirectWrite measured bounds already include
        // the submitted drawing origin, in control-client DIPs. GDI measurement boxes are measurement-DC
        // coordinates, not assertions about the native control's painted text.
        // DirectWrite overhang contains edge distances, not rectangle corners.
        Rect clientPixels{},textArea{},measured{},overhang{},clipBox{};
        // DirectWrite transform is the observed target DIP transform. GDI's
        // transform is logical-to-device for observed MM_TEXT contexts only.
        float transform[6]{};ClipSource clipSource{};std::int32_t mappingMode{};
        FontMetrics font{};
        float layoutWidth{},layoutHeight{},widthIncludingTrailingWhitespace{};
        std::uint32_t lineCount{},bidiDepth{},reserved2{};
        std::int64_t content{},page{},position{};
    };
    struct alignas(8) Header {
        std::uint32_t magic{},version{},headerBytes{},recordBytes{},capacity{},count{},flags{},processId{};
        std::uint64_t processCreation{},qpcFrequency{},sequence{},publishedQpc{};
    };
    struct Snapshot { Header header;Record records[Capacity]; };
    static_assert(sizeof(wchar_t)==2,"The wire format requires Windows UTF-16.");
    static_assert(sizeof(void*)==8,"This protocol currently supports x64 only.");
    static_assert(std::is_trivially_copyable<Snapshot>::value,"Wire records must remain plain data.");
    static_assert(offsetof(Header,sequence)%8==0,"Sequence must be naturally aligned.");
    enum class ReadResult { Ready,InvalidHeader,Torn,Overflowed,WrongProcess,Stale,InvalidRecord };
    // Caller opens FILE_MAP_READ, verifies exact ABI/header/record/capacity bytes
    // and the measured section extent rounded up to the actual system page size.
    // The backing section need not report the unrounded sizeof(Snapshot).
    // Every completed publish advances sequence to an even value; records carry
    // that completed even sequence. Registration is one HWND lifetime across
    // all record kinds; compare the live RegistrationProperty value and reject
    // mismatches (including reused handles). The producer removes that property
    // on unregister and a new lifetime receives a new monotonic registration.
    // The reader separately
    // revalidates the real process/HWND identities. An odd/changing sequence is
    // rejected. Never substitute a prior snapshot when this call fails.
    inline ReadResult CopyStable(const Snapshot* view,Snapshot& copy)
    {
        const auto before=*reinterpret_cast<const volatile std::uint64_t*>(&view->header.sequence);
        if(!before||(before&1))return ReadResult::Torn;
        MemoryBarrier();std::memcpy(&copy,view,sizeof(copy));MemoryBarrier();
        const auto after=*reinterpret_cast<const volatile std::uint64_t*>(&view->header.sequence);
        if(before!=after||(after&1)||copy.header.sequence!=after)return ReadResult::Torn;
        const auto& h=copy.header;
        if(h.magic!=Magic||h.version!=Version||h.headerBytes!=sizeof(Header)||h.recordBytes!=sizeof(Record)||h.capacity!=Capacity||h.count>Capacity||!h.qpcFrequency||(h.flags&~Overflow))return ReadResult::InvalidHeader;
        return h.flags&Overflow?ReadResult::Overflowed:ReadResult::Ready;
    }
    inline ReadResult ValidateFresh(const Snapshot& copy,std::uint32_t pid,std::uint64_t creation,
        std::uint64_t minimumSequence,std::uint64_t now,std::uint64_t maximumAge)
    {
        const auto& h=copy.header;
        if(h.processId!=pid||h.processCreation!=creation)return ReadResult::WrongProcess;
        if(h.sequence<minimumSequence||h.publishedQpc>now||now-h.publishedQpc>maximumAge)return ReadResult::Stale;
        return ReadResult::Ready;
    }
    inline ReadResult ValidateRecord(const Record& r,std::uint64_t minimumSequence,std::uint64_t now,std::uint64_t maximumAge)
    {
        if(!r.hwnd||!r.registration||!r.sequence||!r.dpi||r.kind<Kind::DirectWritePaint||r.kind>Kind::RailMetrics||r.units<Units::PhysicalClientPixels||r.units>Units::LogicalScrollUnits)return ReadResult::InvalidRecord;
        if(r.route>Route::PrintClient||r.outcome>Outcome::NotReported||r.font.source>FontSource::NotApplicable||r.clipSource>ClipSource::GdiClipBox||r.font.family[31]!=0||(r.validity&~8191u))return ReadResult::InvalidRecord;
        for(const auto* box:{&r.clientPixels,&r.textArea,&r.measured,&r.overhang,&r.clipBox})if(!std::isfinite(box->left)||!std::isfinite(box->top)||!std::isfinite(box->right)||!std::isfinite(box->bottom))return ReadResult::InvalidRecord;
        for(float value:r.transform)if(!std::isfinite(value))return ReadResult::InvalidRecord;
        if(!std::isfinite(r.font.size)||!std::isfinite(r.font.height)||!std::isfinite(r.font.ascent)||!std::isfinite(r.font.descent)||!std::isfinite(r.font.externalLeading)||!std::isfinite(r.layoutWidth)||!std::isfinite(r.layoutHeight)||!std::isfinite(r.widthIncludingTrailingWhitespace))return ReadResult::InvalidRecord;
        if(r.kind==Kind::RailMetrics&&(r.content<0||r.page<0||r.position<0||r.position>(r.content>r.page?r.content-r.page:0)))return ReadResult::InvalidRecord;
        if(r.sequence<minimumSequence||r.qpc>now||now-r.qpc>maximumAge)return ReadResult::Stale;
        return ReadResult::Ready;
    }
}
