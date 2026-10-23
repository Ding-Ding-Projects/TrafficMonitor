#include "../stdafx.h"
#include "ScrollViewport.h"
namespace NativeMaterial
{
    BOOL ScrollViewport::Create(CWnd* parent,UINT id)
    {if(!CreateEx(WS_EX_CONTROLPARENT,AfxRegisterWndClass(0,::LoadCursor(nullptr,IDC_ARROW)),L"Settings controls",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,CRect(0,0,100,100),parent,id))return FALSE;if(!m_rail.Create(this,53998))return FALSE;m_rail.changed=[this](int value){m_position=value;if(scroll)scroll(SB_THUMBTRACK,value);};return TRUE;}
    void ScrollViewport::SetAppearance(Appearance value){m_rail.SetAppearance(value);Invalidate();}
    void ScrollViewport::SetMetrics(int content,int page,int position)
    {m_position=position;CRect r;GetClientRect(r);const int width=MulDiv(12,GetDpiForWindow(m_hWnd),96);m_rail.MoveWindow(r.right-width,0,width,r.Height());m_rail.SetMetrics(content,page,position);m_rail.BringWindowToTop();}
    LRESULT ScrollViewport::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_SETFONT){m_font=reinterpret_cast<HFONT>(w);return 0;}if(message==WM_GETFONT)return reinterpret_cast<LRESULT>(m_font);
        if(message==WM_COMMAND||message==WM_CONTEXTMENU||(message>=WM_CTLCOLORMSGBOX&&message<=WM_CTLCOLORSTATIC))return GetParent()->SendMessage(message,w,l);
        if(message==WM_MOUSEWHEEL){m_wheel+=GET_WHEEL_DELTA_WPARAM(w);while(abs(m_wheel)>=WHEEL_DELTA){const int direction=m_wheel>0?1:-1;if(scroll)scroll(direction>0?SB_LINEUP:SB_LINEDOWN,0);m_wheel-=direction*WHEEL_DELTA;}return TRUE;}
        if(message==WM_ERASEBKGND&&background){RECT r{};GetClientRect(&r);::FillRect(reinterpret_cast<HDC>(w),&r,background());return TRUE;}
        return CWnd::WindowProc(message,w,l);
    }
}
