#include "../stdafx.h"
#include "TextField.h"
namespace NativeMaterial
{
    IMPLEMENT_DYNAMIC(TextField,CEdit)
    namespace {COLORREF Rgb(D2D1_COLOR_F c){return RGB(BYTE(c.r*255),BYTE(c.g*255),BYTE(c.b*255));}}
    void TextField::SetAppearance(Appearance value)
    {m_appearance=value;m_background.DeleteObject();m_background.CreateSolidBrush(Rgb(Palette::Resolve(value).container));ModifyStyleEx(WS_EX_CLIENTEDGE,0,SWP_FRAMECHANGED);ModifyStyle(WS_BORDER,0,SWP_FRAMECHANGED);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME);}
    HBRUSH TextField::Color(CDC* dc)
    {const auto c=Palette::Resolve(m_appearance);if(!m_background.GetSafeHandle())m_background.CreateSolidBrush(Rgb(c.container));dc->SetTextColor(Rgb(IsWindowEnabled()?c.onSurface:c.secondaryText));dc->SetBkColor(Rgb(c.container));return static_cast<HBRUSH>(m_background.GetSafeHandle());}
    void TextField::Frame()
    {
        CWindowDC dc(this);CRect r;GetWindowRect(r);r.OffsetRect(-r.TopLeft());const auto c=Palette::Resolve(m_appearance);const int pad=MulDiv(8,GetDpiForWindow(m_hWnd),96);
        CRect client;GetClientRect(client);ClientToScreen(client);CRect window;GetWindowRect(window);client.OffsetRect(-window.TopLeft());dc.ExcludeClipRect(client);
        dc.FillSolidRect(r,Rgb(c.container));CPen pen(PS_SOLID,GetFocus()==this?2:1,Rgb(GetFocus()==this?c.primary:c.outline));auto* old=dc.SelectObject(&pen);dc.MoveTo(pad,r.bottom-1);dc.LineTo(r.right-pad,r.bottom-1);dc.SelectObject(old);
    }
    LRESULT TextField::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_NCCALCSIZE){const auto result=CEdit::WindowProc(message,w,l);RECT* r=w?&reinterpret_cast<NCCALCSIZE_PARAMS*>(l)->rgrc[0]:reinterpret_cast<RECT*>(l);const int x=MulDiv(12,GetDpiForWindow(m_hWnd),96),y=MulDiv(8,GetDpiForWindow(m_hWnd),96);if(r->right-r->left>x*2&&r->bottom-r->top>y*2){r->left+=x;r->right-=x;r->top+=y;r->bottom-=y;}return result;}
        if(message==WM_NCPAINT){Frame();return 0;}
        if(message==WM_SETFOCUS||message==WM_KILLFOCUS){const auto result=CEdit::WindowProc(message,w,l);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME);return result;}
        if(message==WM_CONTEXTMENU&&GetParent())return GetParent()->SendMessage(message,reinterpret_cast<WPARAM>(m_hWnd),l);
        return CEdit::WindowProc(message,w,l);
    }
}
