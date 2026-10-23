#include "../stdafx.h"
#include "SearchSelector.h"
#include "../NativeMenus/SearchableMenu.h"
namespace NativeMaterial
{
    BOOL SearchSelector::Create(const RECT& rect, CWnd* parent, UINT id)
    {
        if (!CreateEx(WS_EX_CONTROLPARENT, AfxRegisterWndClass(0), L"", WS_CHILD | WS_VISIBLE,
            rect, parent, id)) return FALSE;
        m_button.Create(L"", CRect(0, 0, 1, 1), this, 1);
        m_button.SetVariant(ButtonVariant::Selector);
        return TRUE;
    }
    void SearchSelector::SetAppearance(Appearance value) { m_button.SetAppearance(value); }
    void SearchSelector::SetPrompt(const CString& prompt)
    {
        m_prompt = prompt;
        m_button.SetLabel(m_selected >= 0 ? m_items[m_selected] : m_prompt);
    }
    void SearchSelector::ResetContent()
    {
        m_items.clear();
        m_selected = -1;
        m_button.SetLabel(m_prompt);
    }
    void SearchSelector::AddString(const CString& value) { m_items.push_back(value); }
    void SearchSelector::Choose()
    {
        CMenu menu;
        menu.CreatePopupMenu();
        for (size_t i = 0; i < m_items.size(); ++i)
            menu.AppendMenu(MF_STRING | (m_selected == static_cast<int>(i) ? MF_CHECKED : 0),
                static_cast<UINT_PTR>(i + 1), m_items[i]);
        if (m_items.empty()) menu.AppendMenu(MF_STRING | MF_GRAYED, 1, m_prompt);
        CRect bounds;
        GetWindowRect(bounds);
        const UINT selected = NativeMenus::Track(&menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
            bounds.left, bounds.bottom, this, &m_filter);
        if (selected > 0 && selected <= m_items.size())
        {
            m_selected = selected - 1;
            m_button.SetLabel(m_items[m_selected]);
            GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CBN_SELCHANGE),
                reinterpret_cast<LPARAM>(m_hWnd));
        }
    }
    LRESULT SearchSelector::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == WM_COMMAND && LOWORD(w) == 1 && HIWORD(w) == BN_CLICKED)
        {
            Choose();
            return 0;
        }
        if (message == WM_SIZE && m_button.GetSafeHwnd())
            m_button.MoveWindow(0, 0, LOWORD(l), HIWORD(l));
        if (message == WM_ENABLE && m_button.GetSafeHwnd()) m_button.EnableWindow(w != 0);
        if (message == WM_SETFONT && m_button.GetSafeHwnd()) m_button.SendMessage(message, w, l);
        return CWnd::WindowProc(message, w, l);
    }
}
