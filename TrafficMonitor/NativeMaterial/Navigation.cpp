#include "../stdafx.h"
#include "Navigation.h"
namespace NativeMaterial
{
    namespace {COLORREF Rgb(D2D1_COLOR_F c){return RGB(BYTE(c.r*255),BYTE(c.g*255),BYTE(c.b*255));}}
    BOOL ResultList::Create(const RECT& r,CWnd* parent,UINT id,bool navigation)
    {
        m_navigation=navigation;if(!CListBox::Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|LBS_NOTIFY|LBS_HASSTRINGS|LBS_OWNERDRAWVARIABLE|LBS_NOINTEGRALHEIGHT,r,parent,id))return FALSE;
        if(!m_rail.Create(this,id+900))return FALSE;m_rail.SetStep(1);m_rail.changed=[this](int value){SetTopIndex(value);SyncScroll();};return TRUE;
    }
    void ResultList::SetAppearance(Appearance value){m_appearance=value;m_rail.SetAppearance(value);Invalidate();}
    void ResultList::MeasureItem(LPMEASUREITEMSTRUCT item)
    {
        CString text;if(item->itemID!=static_cast<UINT>(-1)&&item->itemID<static_cast<UINT>(GetCount()))GetText(item->itemID,text);
        const int dpi=GetDpiForWindow(m_hWnd);CRect r;GetClientRect(r);CClientDC dc(this);auto* font=GetFont();auto* old=font?dc.SelectObject(font):nullptr;
        CRect measure(0,0,(std::max)(40,r.Width()-MulDiv(m_navigation?48:36,dpi,96)),0);dc.DrawText(text,measure,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);if(old)dc.SelectObject(old);
        item->itemHeight=(std::max)(MulDiv(m_navigation?48:52,dpi,96),measure.Height()+MulDiv(20,dpi,96));
    }
    void ResultList::DrawItem(LPDRAWITEMSTRUCT item)
    {
        if(item->itemID==static_cast<UINT>(-1))return;CDC dc;dc.Attach(item->hDC);CRect r(item->rcItem);const auto c=Palette::Resolve(m_appearance);const bool selected=(item->itemState&ODS_SELECTED)!=0;const UINT dpi=GetDpiForWindow(m_hWnd);auto px=[&](int n){return MulDiv(n,dpi,96);};
        dc.FillSolidRect(r,Rgb(c.surface));CRect shape=r;shape.DeflateRect(px(4),px(3));shape.right-=px(12);CBrush brush(Rgb(selected?c.container:c.surface));auto* oldBrush=dc.SelectObject(&brush);auto* oldPen=dc.SelectStockObject(NULL_PEN);dc.RoundRect(shape,CPoint(px(16),px(16)));dc.SelectObject(oldBrush);dc.SelectObject(oldPen);
        if(selected&&m_navigation)dc.FillSolidRect(shape.left,shape.top+px(8),px(3),shape.Height()-px(16),Rgb(c.primary));
        CString label;GetText(item->itemID,label);CRect text=r;text.DeflateRect(px(m_navigation?36:12),px(10));text.right=r.right-px(24);auto* oldFont=GetFont()?dc.SelectObject(GetFont()):nullptr;dc.SetBkMode(TRANSPARENT);dc.SetTextColor(Rgb(selected?c.primary:c.onSurface));dc.DrawText(label,text,DT_WORDBREAK|DT_NOPREFIX);if(oldFont)dc.SelectObject(oldFont);
        if (m_navigation)
        {
            CPen pen(PS_SOLID, (std::max)(1, px(1)), Rgb(selected ? c.primary : c.secondaryText));
            auto* previousPen = dc.SelectObject(&pen);
            auto* previousBrush = dc.SelectStockObject(NULL_BRUSH);
            const int x = r.left + px(13), y = r.top + px(16);
            auto line = [&](int x1, int y1, int x2, int y2) {
                dc.MoveTo(x + px(x1), y + px(y1));
                dc.LineTo(x + px(x2), y + px(y2));
            };
            switch (item->itemID)
            {
            case 0: // Live monitoring waveform.
                line(0, 9, 3, 9); line(3, 9, 6, 2); line(6, 2, 9, 14);
                line(9, 14, 12, 6); line(12, 6, 16, 6);
                break;
            case 1: // Floating window in front of its owner.
                dc.Rectangle(x, y, x + px(11), y + px(10));
                dc.Rectangle(x + px(5), y + px(5), x + px(17), y + px(15));
                line(7, 8, 15, 8);
                break;
            case 2: // Monitor with a distinct bottom taskbar.
                dc.Rectangle(x, y, x + px(17), y + px(13));
                line(1, 9, 16, 9); line(8, 13, 8, 16); line(4, 16, 13, 16);
                break;
            case 3: // Notification bell and clapper.
                dc.Arc(x + px(3), y + px(1), x + px(14), y + px(13), x + px(14), y + px(7), x + px(3), y + px(7));
                line(3, 7, 2, 13); line(14, 7, 15, 13); line(2, 13, 15, 13);
                dc.Ellipse(x + px(7), y + px(14), x + px(11), y + px(17));
                break;
            case 4:
                dc.Ellipse(x, y, x + px(16), y + px(16));
                line(8, 0, 8, 16); line(0, 8, 16, 8);
                break;
            case 5:
                dc.Rectangle(x, y, x + px(7), y + px(7));
                dc.Rectangle(x + px(9), y + px(9), x + px(16), y + px(16));
                break;
            }
            dc.SelectObject(previousBrush);
            dc.SelectObject(previousPen);
        }
        if(item->itemState&ODS_FOCUS){shape.DeflateRect(1,1);dc.DrawFocusRect(shape);}dc.Detach();
    }
    void ResultList::SyncScroll()
    {
        if(!m_rail.GetSafeHwnd()||m_sync)return;m_sync=true;CRect r;GetClientRect(r);int visible=0,used=0;const int top=(std::max)(0,GetTopIndex());for(int i=top;i<GetCount();++i){const int h=GetItemHeight(i);if(h<=0||used+h>r.Height())break;used+=h;++visible;}visible=(std::max)(1,visible);m_rail.MoveWindow(r.right-MulDiv(12,GetDpiForWindow(m_hWnd),96),0,MulDiv(12,GetDpiForWindow(m_hWnd),96),r.Height());m_rail.SetMetrics(GetCount(),visible,top);m_rail.BringWindowToTop();m_sync=false;
    }
    LRESULT ResultList::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_MOUSEWHEEL){m_wheel+=GET_WHEEL_DELTA_WPARAM(w);while(abs(m_wheel)>=WHEEL_DELTA){const int direction=m_wheel>0?1:-1;SetTopIndex((std::max)(0,GetTopIndex()-direction*3));m_wheel-=direction*WHEEL_DELTA;}SyncScroll();return TRUE;}
        if(message==WM_ERASEBKGND){CRect r;GetClientRect(r);CDC::FromHandle(reinterpret_cast<HDC>(w))->FillSolidRect(r,Rgb(Palette::Resolve(m_appearance).surface));return TRUE;}
        const auto result=CListBox::WindowProc(message,w,l);
        if(message==WM_SIZE||message==WM_SETFONT||message==LB_ADDSTRING){for(int i=0;i<GetCount();++i){MEASUREITEMSTRUCT measure{};measure.itemID=i;MeasureItem(&measure);SetItemHeight(i,measure.itemHeight);}}
        if(message==WM_SIZE||message==WM_SETFONT||message==WM_KEYDOWN||message==LB_ADDSTRING||message==LB_RESETCONTENT||message==LB_SETTOPINDEX)SyncScroll();
        return result;
    }
}
