#include "../stdafx.h"
#include "ButtonVisual.h"
namespace NativeMaterial
{
    void DrawButton(ID2D1RenderTarget* target,const Palette& c,const CString& label,ButtonVariant variant,bool enabled,bool pressed,bool hover,bool focused)
    {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;if(FAILED(target->CreateSolidColorBrush(c.primary,brush.GetAddressOf())))return;auto color=[&](D2D1_COLOR_F v){brush->SetColor(v);return brush.Get();};
        const auto size=target->GetSize();const auto rect=D2D1::RectF(3,3,size.width-3,size.height-3);const float radius=variant==ButtonVariant::Selector?8.f:12.f;const auto shape=D2D1::RoundedRect(rect,radius,radius);
        const bool primary=variant==ButtonVariant::Primary;const auto ink=enabled?(primary?c.onPrimary:variant==ButtonVariant::Text?c.primary:c.onSurface):c.secondaryText;
        if(variant!=ButtonVariant::Text)target->FillRoundedRectangle(shape,color(primary&&enabled?c.primary:c.container));
        if(variant==ButtonVariant::Outlined||c.highContrast)target->DrawRoundedRectangle(shape,color(c.outline),1);
        if((pressed||hover)&&enabled){auto state=primary?c.onPrimary:c.primary;state.a=pressed?.12f:.08f;target->FillRoundedRectangle(shape,color(state));}
        if(focused)target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(1,1,size.width-1,size.height-1),radius+2,radius+2),color(c.primary),2);
        Microsoft::WRL::ComPtr<IDWriteFactory> factory;Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
        if(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(factory.GetAddressOf())))&&SUCCEEDED(factory->CreateTextFormat(L"Segoe UI",nullptr,DWRITE_FONT_WEIGHT_MEDIUM,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,14,L"",format.GetAddressOf())))
        {format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);target->DrawText(label,label.GetLength(),format.Get(),D2D1::RectF(15,5,size.width-(variant==ButtonVariant::Selector?36:15),size.height-5),color(ink));}
        if(variant==ButtonVariant::Selector){const float x=size.width-23,y=size.height/2;target->DrawLine(D2D1::Point2F(x-4,y-2),D2D1::Point2F(x,y+2),color(ink),1.5f);target->DrawLine(D2D1::Point2F(x,y+2),D2D1::Point2F(x+4,y-2),color(ink),1.5f);}
    }
}
