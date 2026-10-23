#pragma once
#include <afxwin.h>
#include <functional>
namespace NativeSettings
{
    class SettingEdit final : public CEdit
    {
    protected:
        LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) override
        {
            if(message==WM_CONTEXTMENU)return GetParent()->SendMessage(message,reinterpret_cast<WPARAM>(m_hWnd),l);
            return CEdit::WindowProc(message,w,l);
        }
    };
    class SettingsViewport final : public CWnd
    {
    public:
        std::function<void(UINT,int)> scroll;
        std::function<HBRUSH()> background;
        BOOL Create(CWnd* owner)
        {return CreateEx(WS_EX_CONTROLPARENT,AfxRegisterWndClass(0,::LoadCursor(nullptr,IDC_ARROW)),L"Settings controls",WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_CLIPCHILDREN,CRect(0,0,100,100),owner,53999);}
    protected:
        LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) override
        {
            if(message==WM_COMMAND||message==WM_CONTEXTMENU||(message>=WM_CTLCOLORMSGBOX&&message<=WM_CTLCOLORSTATIC))return GetParent()->SendMessage(message,w,l);
            if(message==WM_VSCROLL){SCROLLINFO info{sizeof(info),SIF_TRACKPOS};GetScrollInfo(SB_VERT,&info);if(scroll)scroll(LOWORD(w),info.nTrackPos);return 0;}
            if(message==WM_MOUSEWHEEL){if(scroll)scroll(GET_WHEEL_DELTA_WPARAM(w)>0?SB_LINEUP:SB_LINEDOWN,0);return TRUE;}
            if(message==WM_ERASEBKGND&&background){RECT r{};GetClientRect(&r);::FillRect(reinterpret_cast<HDC>(w),&r,background());return TRUE;}
            return CWnd::WindowProc(message,w,l);
        }
    };
}
