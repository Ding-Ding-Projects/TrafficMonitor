#include "../stdafx.h"
#include "ButtonVisual.h"
#include "LayoutObservations.h"
namespace NativeMaterial
{
    void DrawButton(ID2D1RenderTarget* target,const Palette& c,const CString& label,ButtonVariant variant,bool enabled,bool pressed,bool hover,bool focused)
    {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        if(FAILED(target->CreateSolidColorBrush(c.primary,brush.GetAddressOf())))return;
        auto color=[&](D2D1_COLOR_F value){brush->SetColor(value);return brush.Get();};
        const auto size=target->GetSize();
        if(size.width<=6||size.height<=6)return;
        const bool selector=variant==ButtonVariant::Selector,primary=variant==ButtonVariant::Primary,tonal=variant==ButtonVariant::Tonal;
        const auto rect=D2D1::RectF(3,3,size.width-3,size.height-3);
        const float radius=selector?4.f:(rect.bottom-rect.top)/2;
        const auto shape=D2D1::RoundedRect(rect,radius,radius);
        const COLORREF gray=GetSysColor(COLOR_GRAYTEXT);
        const auto disabled=c.highContrast?D2D1::ColorF(GetRValue(gray)/255.f,GetGValue(gray)/255.f,GetBValue(gray)/255.f):BlendColor(c.onSurface,c.surface,.38f);
        const auto ink=!enabled?disabled:primary?c.onPrimary:tonal?c.onSecondaryContainer:selector?c.onSurface:c.primary;
        if(primary||tonal)
            target->FillRoundedRectangle(shape,color(enabled?(primary?c.primary:c.secondaryContainer):c.highContrast?c.surface:BlendColor(c.onSurface,c.surface,.12f)));
        if(variant==ButtonVariant::Outlined||selector||c.highContrast)
            target->DrawRoundedRectangle(shape,color(enabled?(selector&&focused?c.primary:c.outline):disabled),selector&&focused?2.f:1.f);
        if(enabled&&(pressed||focused||hover))
        {
            auto layer=ink;layer.a=pressed||focused?.12f:.08f;
            target->FillRoundedRectangle(shape,color(layer));
        }
        if(focused)target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(1,1,size.width-1,size.height-1),radius+2,radius+2),color(c.primary),2);
        Microsoft::WRL::ComPtr<IDWriteFactory> factory;Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
        if(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(factory.GetAddressOf())))&&
            SUCCEEDED(factory->CreateTextFormat(L"Segoe UI",nullptr,DWRITE_FONT_WEIGHT_MEDIUM,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,14,L"",format.GetAddressOf())))
        {
            format->SetTextAlignment(selector?DWRITE_TEXT_ALIGNMENT_LEADING:DWRITE_TEXT_ALIGNMENT_CENTER);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            float inset=selector?16.f:variant==ButtonVariant::Text?12.f:24.f;
            const float rightInset=selector?36.f:inset;
            // Existing owners have not all adopted the larger horizontal inset.
            // Keep their former text width when the full label needs that space.
            if(!selector&&variant!=ButtonVariant::Text)
            {
                Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
                if(SUCCEEDED(factory->CreateTextLayout(label,label.GetLength(),format.Get(),(std::max)(1.f,size.width-inset-rightInset),(std::max)(1.f,size.height-10),layout.GetAddressOf())))
                {
                    DWRITE_TEXT_METRICS metrics{};layout->GetMetrics(&metrics);
                    float minimum{};layout->DetermineMinWidth(&minimum);
                    if(metrics.height>size.height-10||minimum>size.width-inset-rightInset)inset=15.f;
                }
            }
            const auto text=D2D1::RectF(inset,5,size.width-(selector?36.f:inset),size.height-5);
            Microsoft::WRL::ComPtr<IDWriteTextLayout> actualLayout;
            if(SUCCEEDED(factory->CreateTextLayout(label,label.GetLength(),format.Get(),(std::max)(0.f,text.right-text.left),(std::max)(0.f,text.bottom-text.top),actualLayout.GetAddressOf())))
            {
                const auto origin=D2D1::Point2F(text.left,text.top);
                target->DrawTextLayout(origin,actualLayout.Get(),color(ink));
                NativeLayoutObservation::ObserveDirectWrite(target,actualLayout.Get(),format.Get(),origin);
            }
        }
        if(selector)
        {
            const float x=size.width-23,y=size.height/2;
            target->DrawLine(D2D1::Point2F(x-4,y-2),D2D1::Point2F(x,y+2),color(ink),1.5f);
            target->DrawLine(D2D1::Point2F(x,y+2),D2D1::Point2F(x+4,y-2),color(ink),1.5f);
        }
    }
}
