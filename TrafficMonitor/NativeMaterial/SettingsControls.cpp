#include "../stdafx.h"
#include "SettingsControls.h"
namespace NativeMaterial
{
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
        dc.FillSolidRect(rect, color(palette.container));
        const CRect box(px(14), rect.CenterPoint().y - px(10), px(34), rect.CenterPoint().y + px(10));
        dc.FillSolidRect(box, color(GetCheck() == BST_CHECKED ? palette.primary : palette.surface));
        dc.Draw3dRect(box, color(palette.outline), color(palette.outline));
        if (GetCheck() == BST_CHECKED)
        {
            CPen pen(PS_SOLID, px(2), color(palette.onPrimary)); auto* old = dc.SelectObject(&pen);
            dc.MoveTo(box.left + px(4), box.top + px(10)); dc.LineTo(box.left + px(8), box.top + px(15));
            dc.LineTo(box.right - px(3), box.top + px(4)); dc.SelectObject(old);
        }
        CString text; GetWindowText(text); CRect label(rect); label.left = px(48); label.right -= px(12);
        label.top += px(12); label.bottom -= px(6);
        CFont* oldFont = dc.SelectObject(GetFont()); dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(color(IsWindowEnabled() ? palette.onSurface : palette.secondaryText));
        dc.DrawText(text, label, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX); dc.SelectObject(oldFont);
        if (GetFocus() == this) { rect.DeflateRect(px(3), px(3)); dc.DrawFocusRect(rect); }
    }
    void SettingsCheck::OnLButtonUp(UINT flags, CPoint point) { CButton::OnLButtonUp(flags, point); Invalidate(); }
    void SettingsCheck::OnKeyUp(UINT key, UINT repeat, UINT flags) { CButton::OnKeyUp(key, repeat, flags); Invalidate(); }
    void SettingsCheck::OnSetFocus(CWnd* old) { CButton::OnSetFocus(old); Invalidate(); }
    void SettingsCheck::OnKillFocus(CWnd* next) { CButton::OnKillFocus(next); Invalidate(); }
}
