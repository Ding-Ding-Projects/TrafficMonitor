#include "../stdafx.h"
#include "Panel.h"
namespace NativeMaterial
{
    namespace {COLORREF Rgb(D2D1_COLOR_F c){return RGB(BYTE(c.r*255),BYTE(c.g*255),BYTE(c.b*255));}}
    BOOL Panel::Create(CWnd* parent,UINT id){return CreateEx(WS_EX_CONTROLPARENT,AfxRegisterWndClass(0),L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,CRect(0,0,100,100),parent,id);}
    void Panel::SetAppearance(Appearance value){m_appearance=value;const auto c=Palette::Resolve(value);m_fill.DeleteObject();m_fill.CreateSolidBrush(c.highContrast?GetSysColor(COLOR_WINDOW):value==Appearance::Dark?RGB(27,33,41):RGB(255,255,255));Invalidate();}
    LRESULT Panel::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_SETFONT){m_font=reinterpret_cast<HFONT>(w);return 0;}if(message==WM_GETFONT)return reinterpret_cast<LRESULT>(m_font);
        if(message==WM_COMMAND||message==WM_CONTEXTMENU)return GetParent()->SendMessage(message,w,l);
        if(message==WM_CTLCOLOREDIT)return GetParent()->SendMessage(message,w,l);
        if(message==WM_CTLCOLORSTATIC)
        {
            wchar_t className[32]{};
            ::GetClassNameW(reinterpret_cast<HWND>(l),className,_countof(className));
            if(_wcsicmp(className,L"EDIT")==0)return GetParent()->SendMessage(message,w,l);
        }
        if(message==WM_CTLCOLORSTATIC){auto* dc=CDC::FromHandle(reinterpret_cast<HDC>(w));dc->SetTextColor(Rgb(Palette::Resolve(m_appearance).onSurface));dc->SetBkMode(TRANSPARENT);return reinterpret_cast<LRESULT>(m_fill.GetSafeHandle());}
        if(message==WM_ERASEBKGND)return TRUE;
        if(message==WM_PAINT){CPaintDC dc(this);CRect r;GetClientRect(r);const auto c=Palette::Resolve(m_appearance);dc.FillSolidRect(r,Rgb(c.surface));auto* b=dc.SelectObject(&m_fill);auto* p=dc.SelectStockObject(NULL_PEN);const int radius=MulDiv(32,GetDpiForWindow(m_hWnd),96);dc.RoundRect(r,CPoint(radius,radius));dc.SelectObject(b);dc.SelectObject(p);return 0;}
        return CWnd::WindowProc(message,w,l);
    }
}
