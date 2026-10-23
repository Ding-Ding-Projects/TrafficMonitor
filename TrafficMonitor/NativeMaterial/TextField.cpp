#include "../stdafx.h"
#include "TextField.h"
#include "../NativeMenus/SearchableMenu.h"
#include "../NativePreferences/Text.h"
#include <windowsx.h>
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
        if(message==WM_CONTEXTMENU)
        {
            int first=0,last=0;GetSel(first,last);const bool selection=first!=last,editable=IsWindowEnabled()&&!(GetStyle()&ES_READONLY);CMenu menu;menu.CreatePopupMenu();
            auto add=[&](UINT id,const wchar_t* en,const wchar_t* zh,bool enabled){menu.AppendMenu(MF_STRING|(enabled?0:MF_GRAYED),id,NativePreferences::Pair(en,zh));};
            add(1,L"Undo",L"復原",editable&&CanUndo());add(2,L"Cut",L"剪下",editable&&selection);add(3,L"Copy",L"複製",selection);add(4,L"Paste",L"貼上",editable&&IsClipboardFormatAvailable(CF_UNICODETEXT));add(5,L"Delete",L"刪除",editable&&selection);add(6,L"Select all",L"全選",GetWindowTextLength()>0);
            CPoint point(GET_X_LPARAM(l),GET_Y_LPARAM(l));if(point==CPoint(-1,-1)){CRect bounds;GetWindowRect(bounds);point=CPoint(bounds.left,bounds.bottom);}
            const auto action=NativeMenus::Track(&menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,this);
            switch(action){case 1:Undo();break;case 2:Cut();break;case 3:Copy();break;case 4:Paste();break;case 5:Clear();break;case 6:SetSel(0,-1);break;default:break;}SetFocus();return 0;
        }
        return CEdit::WindowProc(message,w,l);
    }
}
