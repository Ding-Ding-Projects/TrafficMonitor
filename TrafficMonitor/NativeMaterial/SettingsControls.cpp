#include "../stdafx.h"
#include "SettingsControls.h"
namespace NativeMaterial
{
    COLORREF SettingsDisabledColor(Appearance appearance,bool background)
    {
        const auto c=Palette::Resolve(appearance);
        if(c.highContrast)return GetSysColor(background?COLOR_WINDOW:COLOR_GRAYTEXT);
        const float alpha=background?.12f:.38f;
        return RGB(BYTE((c.onSurface.r*alpha+c.surface.r*(1-alpha))*255),
            BYTE((c.onSurface.g*alpha+c.surface.g*(1-alpha))*255),
            BYTE((c.onSurface.b*alpha+c.surface.b*(1-alpha))*255));
    }
    void SettingsLabel::PaintDisabled(HDC target)
    {
        CDC dc;dc.Attach(target);const int saved=dc.SaveDC();CRect rect;GetClientRect(rect);
        const auto c=Palette::Resolve(m_appearance);
        dc.FillSolidRect(rect,RGB(BYTE(c.surface.r*255),BYTE(c.surface.g*255),BYTE(c.surface.b*255)));
        dc.SetBkMode(TRANSPARENT);dc.SetTextColor(SettingsDisabledColor(m_appearance));
        if(auto* font=GetFont())dc.SelectObject(font);
        CString text;GetWindowText(text);dc.DrawText(text,rect,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
        dc.RestoreDC(saved);dc.Detach();
    }
    LRESULT SettingsLabel::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(m_grouped&&!IsWindowEnabled())
        {
            if(message==WM_PAINT){CPaintDC dc(this);PaintDisabled(dc.GetSafeHdc());return 0;}
            if(message==WM_PRINTCLIENT&&w){PaintDisabled(reinterpret_cast<HDC>(w));return 0;}
        }
        const auto result=CStatic::WindowProc(message,w,l);
        if(message==WM_ENABLE&&m_grouped)Invalidate();
        return result;
    }
    HBRUSH SettingsNumber::Color(CDC* dc)
    {
        if(!m_grouped||IsWindowEnabled())return TextField::Color(dc);
        const auto background=SettingsDisabledColor(m_appearance,true);
        m_disabledBackground.DeleteObject();m_disabledBackground.CreateSolidBrush(background);
        dc->SetTextColor(SettingsDisabledColor(m_appearance));dc->SetBkColor(background);
        return static_cast<HBRUSH>(m_disabledBackground.GetSafeHandle());
    }
    void SettingsNumber::PaintDisabled(HDC target)
    {
        CDC dc;dc.Attach(target);CRect rect;GetClientRect(rect);
        dc.FillSolidRect(rect,SettingsDisabledColor(m_appearance,true));
        dc.SetBkMode(TRANSPARENT);dc.SetTextColor(SettingsDisabledColor(m_appearance));
        auto* font=GetFont();auto* previous=font?dc.SelectObject(font):nullptr;
        CString value;GetWindowText(value);dc.DrawText(value,rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        if(previous)dc.SelectObject(previous);dc.Detach();
    }
    LRESULT SettingsNumber::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        // Disabled EDIT controls normally force system GrayText. Paint their
        // retained value explicitly while leaving the actual HWND disabled.
        if(m_grouped&&!IsWindowEnabled())
        {
            if(message==WM_PAINT){CPaintDC dc(this);PaintDisabled(dc.GetSafeHdc());return 0;}
            if(message==WM_PRINTCLIENT&&w){PaintDisabled(reinterpret_cast<HDC>(w));return 0;}
        }
        if(message==WM_NCPAINT&&m_grouped&&!IsWindowEnabled())
        {
            CWindowDC dc(this);CRect window;GetWindowRect(window);CRect rect=window;rect.OffsetRect(-window.TopLeft());
            CRect client;GetClientRect(client);ClientToScreen(client);client.OffsetRect(-window.TopLeft());dc.ExcludeClipRect(client);
            dc.FillSolidRect(rect,SettingsDisabledColor(m_appearance,true));
            const auto ink=SettingsDisabledColor(m_appearance);dc.Draw3dRect(rect,ink,ink);return 0;
        }
        const auto result=TextField::WindowProc(message,w,l);
        if(message==WM_ENABLE&&m_grouped)RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME);
        return result;
    }
    BEGIN_MESSAGE_MAP(SettingsCheck, CButton)
        ON_WM_PAINT()
        ON_WM_LBUTTONUP()
        ON_WM_KEYUP()
        ON_WM_SETFOCUS()
        ON_WM_KILLFOCUS()
    END_MESSAGE_MAP()
    BOOL SettingsCheck::Create(const CString& text, CWnd* parent, UINT id)
    {
        return CButton::Create(text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(0, 0, 100, 48), parent, id);
    }
    void SettingsCheck::OnPaint()
    {
        CPaintDC dc(this); CRect rect; GetClientRect(rect);
        const auto palette = Palette::Resolve(m_appearance);
        auto color = [](D2D1_COLOR_F value) { return RGB(BYTE(value.r * 255), BYTE(value.g * 255), BYTE(value.b * 255)); };
        const UINT dpi = GetDpiForWindow(m_hWnd);
        auto px = [&](int value) { return MulDiv(value, dpi ? dpi : 96, 96); };
        const bool disabled=m_grouped&&!IsWindowEnabled();
        dc.FillSolidRect(rect, color(m_grouped?palette.surface:palette.container));
        const CRect box(m_grouped?px(2):px(14),rect.CenterPoint().y-px(m_grouped?9:10),m_grouped?px(20):px(34),rect.CenterPoint().y+px(m_grouped?9:10));
        dc.FillSolidRect(box,disabled?SettingsDisabledColor(m_appearance,true):color(GetCheck()==BST_CHECKED?palette.primary:palette.surface));
        const auto outline=disabled?SettingsDisabledColor(m_appearance):color(palette.outline);
        dc.Draw3dRect(box,outline,outline);
        if (GetCheck() == BST_CHECKED)
        {
            CPen pen(PS_SOLID, px(2), disabled?SettingsDisabledColor(m_appearance):color(palette.onPrimary)); auto* old = dc.SelectObject(&pen);
            dc.MoveTo(box.left + px(4), box.top + px(10)); dc.LineTo(box.left + px(8), box.top + px(15));
            dc.LineTo(box.right - px(3), box.top + px(4)); dc.SelectObject(old);
        }
        CString text; GetWindowText(text); CRect label(rect); label.left = px(m_grouped?32:48); label.right -= px(12);
        label.top += px(12); label.bottom -= px(6);
        CFont* oldFont = dc.SelectObject(GetFont()); dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(disabled?SettingsDisabledColor(m_appearance):color(IsWindowEnabled()?palette.onSurface:palette.secondaryText));
        dc.DrawText(text, label, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX); dc.SelectObject(oldFont);
        if (GetFocus() == this) { rect.DeflateRect(px(3), px(3)); dc.DrawFocusRect(rect); }
    }
    void SettingsCheck::OnLButtonUp(UINT flags, CPoint point) { CButton::OnLButtonUp(flags, point); Invalidate(); }
    void SettingsCheck::OnKeyUp(UINT key, UINT repeat, UINT flags) { CButton::OnKeyUp(key, repeat, flags); Invalidate(); }
    void SettingsCheck::OnSetFocus(CWnd* old) { CButton::OnSetFocus(old); Invalidate(); }
    void SettingsCheck::OnKillFocus(CWnd* next) { CButton::OnKillFocus(next); Invalidate(); }
}
