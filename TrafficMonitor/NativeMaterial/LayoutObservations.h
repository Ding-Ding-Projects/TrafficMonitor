#pragma once
#include "LayoutObservationProtocol.h"
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
namespace NativeLayoutObservation
{
    bool Enabled() noexcept;
    std::uint64_t Register(HWND window) noexcept;
    void Unregister(HWND window) noexcept;
    void Invalidate(HWND window) noexcept;
    void RegisterChild(HWND window,HWND registeredParent) noexcept;
    void ObserveNativeText(HWND window) noexcept;
    // Collects GDI measurements during layout and publishes them only after the
    // measured windows have been placed, so each record carries final client geometry.
    class MeasurementBatch final
    {
    public:
        MeasurementBatch() noexcept=default;
        ~MeasurementBatch();
        void Measure(HWND window,HDC dc,const RECT& area,const RECT& measured) noexcept;
        void Publish() noexcept;
        MeasurementBatch(const MeasurementBatch&)=delete;
        MeasurementBatch& operator=(const MeasurementBatch&)=delete;
    private:
        std::vector<Record> m_pending;
    };
    void ObserveRail(HWND window,int content,int page,int position) noexcept;
    class PaintScope final
    {
    public:
        PaintScope(HWND window,HDC dc,Route route) noexcept;
        ~PaintScope();
        void Complete(HRESULT result) noexcept;
        void NotReported() noexcept;
        PaintScope(const PaintScope&)=delete;
        PaintScope& operator=(const PaintScope&)=delete;
    private:
        friend void ObserveDirectWrite(ID2D1RenderTarget*,IDWriteTextLayout*,IDWriteTextFormat*,D2D1_POINT_2F) noexcept;
        Record m_record{};PaintScope* m_previous{};bool m_active{},m_complete{},m_hasText{};
        void Finish(Outcome outcome,HRESULT result) noexcept;
    };
    void ObserveDirectWrite(ID2D1RenderTarget* target,IDWriteTextLayout* layout,IDWriteTextFormat* format,D2D1_POINT_2F origin) noexcept;
}
