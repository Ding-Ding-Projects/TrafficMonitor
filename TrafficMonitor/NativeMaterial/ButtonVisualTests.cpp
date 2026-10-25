#include "../stdafx.h"
#include "ButtonVisual.h"
#include <cstdio>
#include <cmath>
namespace {
CWinApp application;int failures{};
void Check(bool value,const char* name){std::printf("%s %s\n",value?"PASS":"FAIL",name);if(!value)++failures;}
COLORREF Rgb(D2D1_COLOR_F c){return RGB(BYTE(std::lround(c.r*255)),BYTE(std::lround(c.g*255)),BYTE(std::lround(c.b*255)));}
bool Near(COLORREF a,COLORREF b){return abs(int(GetRValue(a))-GetRValue(b))<=2&&abs(int(GetGValue(a))-GetGValue(b))<=2&&abs(int(GetBValue(a))-GetBValue(b))<=2;}
double Luminance(D2D1_COLOR_F c){auto linear=[](double n){return n<=.04045?n/12.92:std::pow((n+.055)/1.055,2.4);};return .2126*linear(c.r)+.7152*linear(c.g)+.0722*linear(c.b);}
double Contrast(D2D1_COLOR_F a,D2D1_COLOR_F b){const auto x=Luminance(a),y=Luminance(b);return ((std::max)(x,y)+.05)/((std::min)(x,y)+.05);}
COLORREF Render(const NativeMaterial::Palette& colors,NativeMaterial::ButtonVariant variant,bool enabled,int x,int y)
{
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=160;info.bmiHeader.biHeight=-56;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* pixels{};HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto previous=SelectObject(dc,bitmap);
    Microsoft::WRL::ComPtr<ID2D1Factory> factory;Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
    HRESULT result=D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,factory.GetAddressOf());
    const auto properties=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE),96,96);
    if(SUCCEEDED(result))result=factory->CreateDCRenderTarget(&properties,target.GetAddressOf());
    RECT bounds{0,0,160,56};if(SUCCEEDED(result))result=target->BindDC(dc,&bounds);
    if(SUCCEEDED(result)){target->BeginDraw();target->Clear(colors.surface);NativeMaterial::DrawButton(target.Get(),colors,L"",variant,enabled,false,false,false);result=target->EndDraw();}
    Check(SUCCEEDED(result),"real Direct2D button render succeeds");GdiFlush();const COLORREF pixel=GetPixel(dc,x,y);
    target.Reset();factory.Reset();SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);return pixel;
}
}
int wmain()
{
    if(!AfxWinInit(GetModuleHandle(nullptr),nullptr,GetCommandLine(),0))return 2;
    using namespace NativeMaterial;
    for(auto appearance:{Appearance::Light,Appearance::Dark})
    {
        auto c=Palette::Resolve(appearance);
        if(!c.highContrast)
        {
            Check(Near(Rgb(c.surface),appearance==Appearance::Light?RGB(249,249,255):RGB(17,19,24)),"reviewed surface color exact");
            Check(Near(Rgb(c.primaryContainer),appearance==Appearance::Light?RGB(214,227,255):RGB(25,70,127)),"reviewed selection container exact");
            const double primary=Contrast(c.primary,c.onPrimary),tonal=Contrast(c.secondaryContainer,c.onSecondaryContainer),selected=Contrast(c.primaryContainer,c.onPrimaryContainer);
            std::printf("CONTRAST primary=%.3f tonal=%.3f selected=%.3f\n",primary,tonal,selected);
            Check(primary>=4.5&&tonal>=4.5&&selected>=4.5,"action and selection text pairs meet 4.5 contrast");
        }
        else Check(Near(Rgb(c.surface),GetSysColor(COLOR_WINDOW)),"high contrast surface follows system color");
        Check(Near(Render(c,ButtonVariant::Primary,true,80,28),Rgb(c.primary)),"filled action uses primary container");
        Check(Near(Render(c,ButtonVariant::Tonal,true,80,28),Rgb(c.secondaryContainer)),"tonal action uses separate secondary container");
        Check(Near(Render(c,ButtonVariant::Outlined,true,80,28),Rgb(c.surface)),"outlined action has no tonal fill");
        Check(Near(Render(c,ButtonVariant::Text,true,80,28),Rgb(c.surface)),"text action has no container fill");
        Check(!Near(Render(c,ButtonVariant::Selector,true,3,28),Rgb(c.surface)),"selector has visible outline");
        Check(Near(Render(c,ButtonVariant::Primary,true,10,8),Rgb(c.surface)),"filled action keeps full rounded ends");
        Check(Near(Render(c,ButtonVariant::Primary,false,80,28),Rgb(c.highContrast?c.surface:BlendColor(c.onSurface,c.surface,.12f))),"disabled filled action uses bounded background role");
    }
    std::printf("RESULT failures=%d\n",failures);return failures?1:0;
}
