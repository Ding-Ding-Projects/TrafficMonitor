#include "../stdafx.h"
#include "InputControls.Internal.h"
#include <regex>

namespace NativeMaterial
{
    std::vector<int> FilterDropdownItems(const std::vector<CString>& items, const CString& query, bool regex, CString* error)
    {
        std::vector<int> rows;
        if (error) error->Empty();
        if (query.IsEmpty())
        {
            for (size_t i = 0; i < items.size(); ++i) rows.push_back(static_cast<int>(i));
            return rows;
        }
        if (!regex)
        {
            CString needle(query);
            needle.MakeLower();
            for (size_t i = 0; i < items.size(); ++i)
            {
                CString hay(items[i]);
                hay.MakeLower();
                if (hay.Find(needle) >= 0) rows.push_back(static_cast<int>(i));
            }
            return rows;
        }
        // The standard regex engine cannot be interrupted, so both inputs are bounded before it runs.
        if (query.GetLength() > 256)
        {
            if (error) *error = NativePreferences::Pair(L"Pattern is too long.", L"樣式太長。");
            return rows;
        }
        try
        {
            const std::wregex pattern(query.GetString(), std::regex_constants::ECMAScript | std::regex_constants::icase);
            for (size_t i = 0; i < items.size(); ++i)
            {
                if (items[i].GetLength() > 512) continue;
                const std::wstring subject(items[i].GetString());
                if (std::regex_search(subject, pattern)) rows.push_back(static_cast<int>(i));
            }
        }
        catch (const std::regex_error&)
        {
            rows.clear();
            if (error) *error = NativePreferences::Pair(L"Invalid regular expression.", L"正規式無效。");
        }
        return rows;
    }

    // The search field inside the popup is a borderless EDIT (IME, clipboard, undo stay native).
    class PopupEdit final : public CEdit
    {
    public:
        DropdownPopup* owner{};
    protected:
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override;
    };

    class DropdownPopup final : public InputWindow
    {
    public:
        explicit DropdownPopup(Dropdown* owner) : m_owner(owner) {}
        bool Open(const CRect& anchor, bool show)
        {
            const UINT dpi = m_owner->Dpi();
            const float widthDip = (std::max)(220.f, static_cast<float>(anchor.Width()) * 96.f / dpi);
            const auto layout = LayoutPopup(widthDip, static_cast<int>(m_owner->m_items.size()), true);
            const CSize size(DipToPx(widthDip, dpi), DipToPx(layout.height, dpi));
            MONITORINFO monitor{ sizeof(monitor) };
            GetMonitorInfo(MonitorFromRect(anchor, MONITOR_DEFAULTTONEAREST), &monitor);
            int x = anchor.left, y = anchor.bottom;
            if (x + size.cx > monitor.rcWork.right) x = (std::max)(monitor.rcWork.left, static_cast<int>(monitor.rcWork.right) - size.cx);
            if (y + size.cy > monitor.rcWork.bottom && anchor.top - size.cy >= monitor.rcWork.top) y = anchor.top - size.cy;
            const auto className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW, LoadCursor(nullptr, IDC_ARROW));
            CWnd* root = CWnd::FromHandle(::GetAncestor(m_owner->m_hWnd, GA_ROOT));
            if (!CWnd::CreateEx(WS_EX_TOOLWINDOW, className, L"", WS_POPUP, CRect(x, y, x + size.cx, y + size.cy), root, 0)) return FALSE;
            SetAppearance(m_owner->GetAppearance());
            CreateInputFont(m_font, dpi);
            m_edit.owner = this;
            const CRect editRect(DipToPx(layout.edit.left, dpi), DipToPx(layout.edit.top, dpi), DipToPx(layout.edit.right, dpi), DipToPx(layout.edit.bottom, dpi));
            if (!m_edit.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, editRect, this, 1)) return FALSE;
            m_edit.SetCueBanner(NativePreferences::Pair(L"Search", L"搜尋"), TRUE);
            PlaceCenteredEdit(m_edit, m_font, layout.edit, dpi);
            AnnotateAccessibleName(m_edit.GetSafeHwnd(), NativePreferences::Pair(L"Search", L"搜尋"));
            m_brush.CreateSolidBrush(ToColorRef(Palette::Resolve(GetAppearance()).surface));
            Refilter();
            m_shown = show;
            if (show)
            {
                ShowWindow(SW_SHOW);
                m_edit.SetFocus();
            }
            return TRUE;
        }
        void Closing() { m_closing = true; }
        bool OwnsFocus() const
        {
            const HWND focus = ::GetFocus();
            return focus && m_hWnd && (focus == m_hWnd || ::IsChild(m_hWnd, focus));
        }
        HWND EditHwnd() const { return m_edit.GetSafeHwnd(); }
        const std::vector<int>& Visible() const { return m_visible; }
        int Hot() const { return m_hot; }
        int HotItem() const { return m_hot >= 0 && m_hot < static_cast<int>(m_visible.size()) ? m_visible[m_hot] : -1; }
        bool Regex() const { return m_regex; }
        const CString& Error() const { return m_error; }
        const CString& Query() const { return m_query; }

        void Refilter()
        {
            if (m_edit.GetSafeHwnd()) m_edit.GetWindowText(m_query);
            m_visible = FilterDropdownItems(m_owner->m_items, m_query, m_regex, &m_error);
            m_first = 0;
            m_hot = m_visible.empty() ? -1 : 0;
            if (m_query.IsEmpty())
                for (size_t i = 0; i < m_visible.size(); ++i) if (m_visible[i] == m_owner->m_selected) { m_hot = static_cast<int>(i); break; }
            EnsureHotVisible();
            if (m_hWnd) Invalidate(FALSE);
        }
        void SetRegex(bool regex) { m_regex = regex; Refilter(); }

        bool HandleKey(UINT key)
        {
            const int rows = (std::max)(1, ViewRows());
            switch (key)
            {
            case VK_DOWN: Move(1); return true;
            case VK_UP: Move(-1); return true;
            case VK_NEXT: Move(rows); return true;
            case VK_PRIOR: Move(-rows); return true;
            case VK_RETURN:
                if (m_hot >= 0) Commit(m_hot);
                else if (!m_visible.empty()) Commit(0);
                return true;
            case VK_ESCAPE:
                // Escape clears the search first and only then closes.
                if (!m_query.IsEmpty() && m_edit.GetSafeHwnd()) m_edit.SetWindowText(L"");
                else m_owner->Close(false);
                return true;
            case VK_TAB:
            case VK_F4:
                m_owner->Close(false);
                return true;
            default: return false;
            }
        }
    protected:
        void Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState&) override
        {
            PopupModel model;
            for (const int item : m_visible) model.rows.push_back(m_owner->m_items[item]);
            model.selectedRow = -1;
            for (size_t i = 0; i < m_visible.size(); ++i) if (m_visible[i] == m_owner->m_selected) model.selectedRow = static_cast<int>(i);
            model.hotRow = m_hot;
            model.firstRow = m_first;
            model.regex = m_regex;
            model.regexHot = m_regexHot;
            model.queryEmpty = m_query.IsEmpty();
            model.error = m_error;
            model.emptyText = NativePreferences::Pair(L"No matches", L"冇符合項目");
            model.regexLabel = NativePreferences::Pair(L"Regex", L"正規式");
            DrawDropdownPopup(target, palette, model);
        }
        CSize MinimumLogicalSize() const override { return CSize(220, 120); }
        UINT DialogCode() const override { return 0; }
        bool IsFocused() const override { return OwnsFocus(); }
        void PointerPressed(CPoint point) override
        {
            m_pressedRow = RowAt(point);
            if (InRegex(point)) { SetRegex(!m_regex); m_pressedRow = -1; }
        }
        void PointerReleased(CPoint point, bool inside) override
        {
            const int row = RowAt(point);
            const int pressed = m_pressedRow;
            m_pressedRow = -1;
            if (inside && row >= 0 && row == pressed) Commit(row);
        }
        void PointerMoved(CPoint point) override
        {
            const int row = RowAt(point);
            if (row >= 0) m_hot = row;
            m_regexHot = InRegex(point);
        }
        void PointerLeft() override { m_regexHot = false; }
        bool KeyPressed(UINT key, bool) override { return HandleKey(key); }
        bool MouseWheel(int delta) override
        {
            const int most = (std::max)(0, static_cast<int>(m_visible.size()) - ViewRows());
            m_first = (std::max)(0, (std::min)(most, m_first - (delta > 0 ? 1 : -1) * 3));
            return true;
        }
        LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) override
        {
            if (message == WM_COMMAND && HIWORD(w) == EN_CHANGE && reinterpret_cast<HWND>(l) == m_edit.GetSafeHwnd()) { Refilter(); return 0; }
            if ((message == WM_CTLCOLOREDIT || message == WM_CTLCOLORSTATIC) && reinterpret_cast<HWND>(l) == m_edit.GetSafeHwnd())
            {
                const auto palette = Palette::Resolve(GetAppearance());
                auto* dc = CDC::FromHandle(reinterpret_cast<HDC>(w));
                dc->SetTextColor(ToColorRef(palette.onSurface));
                dc->SetBkColor(ToColorRef(palette.surface));
                return reinterpret_cast<LRESULT>(m_brush.GetSafeHandle());
            }
            if (message == WM_ACTIVATE && LOWORD(w) == WA_INACTIVE && m_shown && !m_closing && m_owner->GetSafeHwnd())
                ::PostMessage(m_owner->GetSafeHwnd(), DropdownPopupDeactivated, 0, 0);
            return InputWindow::WindowProc(message, w, l);
        }
    private:
        int ViewRows() const { return (std::min)(InputMetrics::PopupMaximumRows, static_cast<int>(m_visible.size())); }
        PopupLayout Layout() const
        {
            CRect client;
            GetClientRect(client);
            return LayoutPopup(Dip(client.Width()), static_cast<int>(m_visible.size()), !m_error.IsEmpty());
        }
        int RowAt(CPoint point) const
        {
            const auto layout = Layout();
            const float x = Dip(point.x), y = Dip(point.y);
            if (x < layout.list.left || x >= layout.list.right || y < layout.list.top || y >= layout.list.top + layout.rows * InputMetrics::RowHeight) return -1;
            const int row = m_first + static_cast<int>((y - layout.list.top) / InputMetrics::RowHeight);
            return row >= 0 && row < static_cast<int>(m_visible.size()) ? row : -1;
        }
        bool InRegex(CPoint point) const
        {
            const auto r = Layout().regex;
            const float x = Dip(point.x), y = Dip(point.y);
            return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
        }
        void EnsureHotVisible()
        {
            const int rows = ViewRows();
            if (m_hot < 0 || rows == 0) { m_first = 0; return; }
            if (m_hot < m_first) m_first = m_hot;
            if (m_hot >= m_first + rows) m_first = m_hot - rows + 1;
            m_first = (std::max)(0, m_first);
        }
        void Move(int delta)
        {
            const int count = static_cast<int>(m_visible.size());
            if (!count) return;
            m_hot = m_hot < 0 ? (delta > 0 ? 0 : count - 1) : (std::max)(0, (std::min)(count - 1, m_hot + delta));
            EnsureHotVisible();
            Invalidate(FALSE);
        }
        void Commit(int row)
        {
            if (row >= 0 && row < static_cast<int>(m_visible.size())) m_owner->Choose(m_visible[row]);
        }

        Dropdown* m_owner;
        PopupEdit m_edit;
        CFont m_font;
        CBrush m_brush;
        std::vector<int> m_visible;
        int m_hot{ -1 }, m_first{}, m_pressedRow{ -1 };
        bool m_regex{}, m_regexHot{}, m_shown{}, m_closing{};
        CString m_query, m_error;
    };

    LRESULT PopupEdit::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (owner)
        {
            if (message == WM_KEYDOWN && owner->HandleKey(static_cast<UINT>(w))) return 0;
            if (message == WM_SYSKEYDOWN && w == VK_DOWN && owner->HandleKey(VK_F4)) return 0;
            if (message == WM_CHAR && (w == VK_RETURN || w == VK_ESCAPE || w == VK_TAB)) return 0;
            if (message == WM_GETDLGCODE)
            {
                auto result = CEdit::WindowProc(message, w, l);
                return result | DLGC_WANTALLKEYS;
            }
        }
        return CEdit::WindowProc(message, w, l);
    }

    // ---------------------------------------------------------------- Dropdown
    Dropdown::Dropdown() = default;
    Dropdown::~Dropdown() = default;

    BOOL Dropdown::Create(const CString& label, const RECT& bounds, CWnd* parent, UINT id)
    {
        m_label = label;
        return CreateInput(InputKind::Dropdown, label, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, bounds, parent, id);
    }
    void Dropdown::ResetContent()
    {
        Close(false);
        m_items.clear();
        m_selected = -1;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    int Dropdown::AddItem(const CString& item)
    {
        m_items.push_back(item);
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        return static_cast<int>(m_items.size()) - 1;
    }
    void Dropdown::SetItems(const std::vector<CString>& items)
    {
        Close(false);
        m_items = items;
        if (m_selected >= static_cast<int>(m_items.size())) m_selected = -1;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    CString Dropdown::GetItemText(int index) const { return index >= 0 && index < static_cast<int>(m_items.size()) ? m_items[index] : CString(); }
    CString Dropdown::GetCurText() const { return GetItemText(m_selected); }
    void Dropdown::SetCurSel(int index)
    {
        index = (std::max)(-1, (std::min)(index, static_cast<int>(m_items.size()) - 1));
        if (index == m_selected) return;
        m_selected = index;
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }
    void Dropdown::SetLabel(const CString& label) { m_label = label; SetWindowText(label); Publish(); Invalidate(FALSE); }
    void Dropdown::SetPrompt(const CString& prompt) { m_prompt = prompt; Publish(); if (m_hWnd) Invalidate(FALSE); }
    void Dropdown::SetSupportingText(const CString& text) { m_supporting = text; Publish(); if (m_hWnd) Invalidate(FALSE); }

    bool Dropdown::IsOpen() const { return m_open; }
    HWND Dropdown::PopupWindow() const { return m_open && m_popup ? m_popup->GetSafeHwnd() : nullptr; }
    HWND Dropdown::PopupEditWindow() const { return m_open && m_popup ? m_popup->EditHwnd() : nullptr; }
    std::vector<int> Dropdown::PopupVisibleItems() const { return m_open && m_popup ? m_popup->Visible() : std::vector<int>(); }
    int Dropdown::PopupHotItem() const { return m_open && m_popup ? m_popup->HotItem() : -1; }
    bool Dropdown::PopupRegexMode() const { return m_open && m_popup && m_popup->Regex(); }
    CString Dropdown::PopupError() const { return m_open && m_popup ? m_popup->Error() : CString(); }
    CString Dropdown::PopupQuery() const { return m_open && m_popup ? m_popup->Query() : CString(); }

    bool Dropdown::Open()
    {
        if (m_open || !m_hWnd || !IsWindowEnabled() || m_items.empty()) return false;
        CRect anchor;
        GetWindowRect(anchor);
        anchor.top += Px(InputMetrics::FieldTop);
        anchor.bottom = anchor.top + Px(InputMetrics::FieldHeight);
        if (m_presenter)
        {
            m_open = true;
            Publish();
            const int chosen = m_presenter(*this, m_items, m_selected, anchor);
            m_open = false;
            Publish();
            if (chosen >= 0) Choose(chosen);
            if (IsWindowVisible()) SetFocus();
            return true;
        }
        m_popup = std::make_unique<DropdownPopup>(this);
        // A dropdown inside an unshown window keeps its popup logic but never flashes a real window.
        if (!m_popup->Open(anchor, IsWindowVisible() != FALSE)) { m_popup.reset(); return false; }
        m_open = true;
        Publish();
        Invalidate(FALSE);
        return true;
    }
    void Dropdown::Close(bool commit)
    {
        if (!m_open || m_presenter) return;
        const int item = commit && m_popup ? m_popup->HotItem() : -1;
        bool refocus = false;
        if (m_popup && m_popup->GetSafeHwnd())
        {
            refocus = m_popup->OwnsFocus();
            m_popup->Closing();
            // The popup object stays alive until the next open: Close can run from inside its own message handler.
            m_popup->DestroyWindow();
        }
        m_open = false;
        m_closedByDeactivation = false;
        m_closedAt = GetTickCount();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
        if (refocus && m_hWnd && IsWindowVisible()) SetFocus();
        if (item >= 0) Choose(item);
    }
    void Dropdown::Choose(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_items.size())) return;
        Close(false);
        const bool changed = index != m_selected;
        m_selected = index;
        Publish();
        Invalidate(FALSE);
        if (changed) Notify(CBN_SELCHANGE);
    }
    bool Dropdown::Typeahead(wchar_t ch)
    {
        const DWORD now = GetTickCount();
        if (now - m_typedAt > 1000) m_typed.Empty();
        m_typedAt = now;
        m_typed.AppendChar(ch);
        const int count = static_cast<int>(m_items.size());
        if (!count) return false;
        CString needle(m_typed);
        needle.MakeLower();
        const int start = m_typed.GetLength() == 1 ? m_selected + 1 : (std::max)(m_selected, 0);
        for (int k = 0; k < count; ++k)
        {
            const int index = (start + k) % count;
            CString text(m_items[index]);
            text.MakeLower();
            if (text.Left(needle.GetLength()) == needle)
            {
                if (index != m_selected)
                {
                    m_selected = index;
                    Publish();
                    Invalidate(FALSE);
                    Notify(CBN_SELCHANGE);
                }
                return true;
            }
        }
        return false;
    }

    void Dropdown::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        const bool placeholder = m_selected < 0;
        DrawDropdownField(target, palette, m_label, placeholder ? m_prompt : GetCurText(), m_supporting, placeholder, m_open, state);
    }
    CSize Dropdown::MinimumLogicalSize() const
    {
        return CSize(200, static_cast<int>(InputMetrics::FieldTop + InputMetrics::FieldHeight + InputMetrics::MessageHeight));
    }
    void Dropdown::FillAccess(InputData& data) const
    {
        data.items.clear();
        for (const auto& item : m_items) data.items.emplace_back(item.GetString());
        data.selected = m_selected;
        data.text = GetCurText().GetString();
        data.expanded = m_open;
        data.name = (m_label.IsEmpty() ? m_prompt : m_label).GetString();
        data.help = m_supporting.GetString();
    }

    void Dropdown::PointerPressed(CPoint) {}
    void Dropdown::PointerReleased(CPoint, bool inside)
    {
        if (!inside) return;
        if (m_open) { Close(false); return; }
        // A click that deactivated the popup also lands here; it must not reopen the list it just closed.
        if (m_closedByDeactivation && GetTickCount() - m_closedAt < 300) return;
        Open();
    }
    void Dropdown::PointerMoved(CPoint) {}
    void Dropdown::PointerLeft() {}

    bool Dropdown::KeyPressed(UINT key, bool alt)
    {
        if (m_open && m_popup) return m_popup->HandleKey(key);
        const int count = static_cast<int>(m_items.size());
        const bool typing = !m_typed.IsEmpty() && GetTickCount() - m_typedAt <= 1000;
        auto select = [&](int index)
        {
            index = (std::max)(0, (std::min)(count - 1, index));
            if (count && index != m_selected)
            {
                m_selected = index;
                Publish();
                Invalidate(FALSE);
                Notify(CBN_SELCHANGE);
            }
        };
        if ((alt && key == VK_DOWN) || key == VK_F4 || key == VK_RETURN || (key == VK_SPACE && !typing)) { Open(); return true; }
        if (key == VK_DOWN) { select(m_selected < 0 ? 0 : m_selected + 1); return true; }
        if (key == VK_UP) { select(m_selected < 0 ? count - 1 : m_selected - 1); return true; }
        if (key == VK_HOME) { select(0); return true; }
        if (key == VK_END) { select(count - 1); return true; }
        return false;
    }
    bool Dropdown::CharTyped(UINT ch)
    {
        if (m_open && m_popup && m_popup->EditHwnd())
        {
            ::SendMessage(m_popup->EditHwnd(), WM_CHAR, ch, 0);
            return true;
        }
        if (ch < 32) return true;
        const bool typing = !m_typed.IsEmpty() && GetTickCount() - m_typedAt <= 1000;
        if (ch == L' ' && !typing) return true;
        Typeahead(static_cast<wchar_t>(ch));
        return true;
    }
    void Dropdown::PerformAction(InputActionId action, int value)
    {
        if (action == InputActionId::Expand) Open();
        else if (action == InputActionId::Collapse) Close(false);
        else if (action == InputActionId::Select) Choose(value);
    }
    bool Dropdown::WantsKey(UINT key) const { return key == VK_RETURN || (key == VK_ESCAPE && m_open); }

    LRESULT Dropdown::WindowProc(UINT message, WPARAM w, LPARAM l)
    {
        if (message == DropdownPopupDeactivated)
        {
            if (m_open) { Close(false); m_closedByDeactivation = true; }
            return 0;
        }
        if (message == WM_DESTROY || (message == WM_ENABLE && w == 0) || (message == WM_SHOWWINDOW && w == 0)) Close(false);
        if (message == WM_DESTROY && m_popup && m_popup->GetSafeHwnd()) m_popup->DestroyWindow();
        return InputWindow::WindowProc(message, w, l);
    }
}
