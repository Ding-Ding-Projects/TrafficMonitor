#include "../stdafx.h"
#include "InputControls.Internal.h"
#include "Chip.h"
#include "LayoutObservations.h"
#include <algorithm>

namespace NativeMaterial
{
    // ------------------------------------------------------------------ Chip
    BOOL Chip::Create(ChipKind kind, const CString& label, CWnd* parent, UINT id, CPoint topLeft)
    {
        m_kind = kind;
        if (!CreateInput(InputKind::Chip, label, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, CRect(topLeft, CSize(48, 40)), parent, id)) return FALSE;
        SizeToContent();
        return TRUE;
    }

    CString Chip::Label() const { CString text; GetWindowText(text); return text; }

    void Chip::SetLabel(const CString& label)
    {
        SetWindowText(label);
        SizeToContent();
        Publish();
        Invalidate(FALSE);
    }

    void Chip::SetSelected(bool selected)
    {
        if (m_selected == selected || (m_kind != ChipKind::Filter && m_kind != ChipKind::Input)) return;
        m_selected = selected;
        SizeToContent();
        Publish();
        if (m_hWnd) Invalidate(FALSE);
    }

    CSize Chip::PreferredSize() const
    {
        return CSize(Px(ChipWidth(Label(), m_kind, Filled())), Px(FeedbackMetrics::ChipTarget));
    }

    void Chip::SizeToContent()
    {
        if (!m_hWnd) return;
        CRect bounds;
        GetWindowRect(bounds);
        if (CWnd* parent = GetParent()) parent->ScreenToClient(bounds);
        const CSize size = PreferredSize();
        if (bounds.Width() != size.cx || bounds.Height() != size.cy)
            SetWindowPos(nullptr, bounds.left, bounds.top, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
        NativeLayoutObservation::Invalidate(m_hWnd);
    }

    ChipLayout Chip::CurrentLayout() const
    {
        CRect client;
        GetClientRect(client);
        return LayoutChip(Dip(client.Width()), Dip(client.Height()), m_kind, Filled());
    }

    CRect Chip::PartRect(ChipPart part) const
    {
        if (part == ChipPart::None) return CRect();
        const auto l = CurrentLayout();
        if (part == ChipPart::Body) return CRect(Px(l.body.left), Px(l.body.top), Px(l.body.right), Px(l.body.bottom));
        if (!l.hasClose) return CRect();
        // The whole trailing end of an input chip removes it, which is a larger target than the 18 DIP icon.
        return CRect(Px(l.close.left - FeedbackMetrics::ChipIconPadding), Px(l.body.top), Px(l.body.right), Px(l.body.bottom));
    }

    ChipPart Chip::PartAt(CPoint point) const
    {
        if (PartRect(ChipPart::Close).PtInRect(point)) return ChipPart::Close;
        if (PartRect(ChipPart::Body).PtInRect(point)) return ChipPart::Body;
        return ChipPart::None;
    }

    bool Chip::Remove()
    {
        if (m_kind != ChipKind::Input || m_removed || !m_hWnd) return false;
        const bool hadFocus = GetFocus() == this;
        m_removed = true;
        Notify(ChipNotifyRemoved);
        ShowWindow(SW_HIDE);
        EnableWindow(FALSE);
        if (hadFocus && m_navigate) m_navigate(*this, 1);
        if (m_changed) m_changed(*this);
        return true;
    }

    void Chip::ActivateBody()
    {
        if (m_removed) return;
        if (m_kind == ChipKind::Filter)
        {
            m_selected = !m_selected;
            SizeToContent();
            Publish();
            Invalidate(FALSE);
            Notify(BN_CLICKED);
            if (m_changed) m_changed(*this);
            return;
        }
        Notify(BN_CLICKED);
    }

    void Chip::Paint(ID2D1RenderTarget* target, const Palette& palette, const InputState& state)
    {
        ChipModel model;
        model.kind = m_kind; model.label = Label(); model.selected = m_selected; model.hoverPart = m_hoverPart; model.pressedPart = m_pressedPart;
        DrawChip(target, palette, model, state);
    }

    CSize Chip::MinimumLogicalSize() const { return CSize(48, static_cast<int>(FeedbackMetrics::ChipTarget)); }

    void Chip::FillAccess(InputData& data) const
    {
        data.toggleable = m_kind == ChipKind::Filter;
        data.toggle = m_kind == ChipKind::Filter && m_selected ? 1 : 0;
        if (m_kind == ChipKind::Input)
        {
            data.shortcut = L"Delete";
            data.help = NativePreferences::Pair(L"Press Delete to remove", L"按 Delete 移除").GetString();
        }
    }

    void Chip::PointerPressed(CPoint point) { m_pressedPart = PartAt(point); }
    void Chip::PointerReleased(CPoint point, bool)
    {
        const auto pressed = m_pressedPart;
        m_pressedPart = ChipPart::None;
        if (pressed == ChipPart::None || PartAt(point) != pressed) return;
        if (pressed == ChipPart::Close) Remove(); else ActivateBody();
    }
    void Chip::PointerMoved(CPoint point) { m_hoverPart = PartAt(point); }
    void Chip::PointerLeft() { m_hoverPart = ChipPart::None; }
    void Chip::PointerCanceled() { m_pressedPart = ChipPart::None; }

    bool Chip::WantsKey(UINT key) const
    {
        return key == VK_SPACE || key == VK_RETURN || key == VK_DELETE || key == VK_BACK || key == VK_LEFT || key == VK_RIGHT || key == VK_HOME || key == VK_END;
    }

    bool Chip::KeyPressed(UINT key, bool)
    {
        if (key == VK_SPACE || key == VK_RETURN) { m_pressed = true; m_pressedPart = ChipPart::Body; return true; }
        if (key == VK_DELETE || key == VK_BACK) { if (m_kind == ChipKind::Input) Remove(); return true; }
        const int delta = key == VK_LEFT ? -1 : key == VK_RIGHT ? 1 : key == VK_HOME ? -2 : key == VK_END ? 2 : 0;
        if (!delta) return false;
        if (m_navigate) m_navigate(*this, delta);
        return true;
    }

    bool Chip::KeyReleased(UINT key)
    {
        if ((key != VK_SPACE && key != VK_RETURN) || !m_pressed) return false;
        m_pressed = false;
        m_pressedPart = ChipPart::None;
        ActivateBody();
        return true;
    }

    void Chip::DpiChanged() { SizeToContent(); }

    void Chip::PerformAction(InputActionId action, int)
    {
        if (action == InputActionId::Toggle) ActivateBody();
    }

    // ---------------------------------------------------------------- ChipSet
    Chip* ChipSet::Add(ChipKind kind, const CString& label, UINT id, bool selected)
    {
        if (!m_parent || !m_parent->GetSafeHwnd()) return nullptr;
        auto chip = std::make_unique<Chip>();
        if (!chip->Create(kind, label, m_parent, id)) return nullptr;
        if (selected) chip->SetSelected(true);
        Chip* raw = chip.get();
        raw->SetChangedHandler([this](Chip&) { Arrange(); });
        raw->SetNavigationHandler([this](Chip& from, int delta)
        {
            const auto visible = Visible();
            if (visible.empty()) return;
            int index = -1;
            for (size_t i = 0; i < visible.size(); ++i) if (visible[i] == &from) index = static_cast<int>(i);
            int target = delta == -2 ? 0 : delta == 2 ? static_cast<int>(visible.size()) - 1 : index + delta;
            if (index < 0) target = delta < 0 ? static_cast<int>(visible.size()) - 1 : 0;
            target = (std::max)(0, (std::min)(static_cast<int>(visible.size()) - 1, target));
            if (visible[target] != &from) visible[target]->SetFocus();
        });
        m_chips.push_back(std::move(chip));
        Arrange();
        return raw;
    }

    Chip* ChipSet::Find(UINT id) const
    {
        for (const auto& chip : m_chips) if (!chip->IsRemoved() && static_cast<UINT>(chip->GetDlgCtrlID()) == id) return chip.get();
        return nullptr;
    }

    std::vector<Chip*> ChipSet::Visible() const
    {
        std::vector<Chip*> chips;
        for (const auto& chip : m_chips) if (!chip->IsRemoved()) chips.push_back(chip.get());
        return chips;
    }

    int ChipSet::Count() const { return static_cast<int>(Visible().size()); }
    Chip* ChipSet::At(int index) const
    {
        const auto chips = Visible();
        return index >= 0 && index < static_cast<int>(chips.size()) ? chips[index] : nullptr;
    }

    std::vector<UINT> ChipSet::SelectedIds() const
    {
        std::vector<UINT> ids;
        for (auto* chip : Visible()) if (chip->IsSelected()) ids.push_back(static_cast<UINT>(chip->GetDlgCtrlID()));
        return ids;
    }

    void ChipSet::Prune()
    {
        m_chips.erase(std::remove_if(m_chips.begin(), m_chips.end(), [](const std::unique_ptr<Chip>& chip) { return chip->IsRemoved(); }), m_chips.end());
    }

    int ChipSet::Layout(CPoint origin, int widthPixels)
    {
        Prune();
        m_origin = origin;
        m_width = (std::max)(1, widthPixels);
        Arrange();
        return m_height;
    }

    void ChipSet::Arrange()
    {
        const auto chips = Visible();
        m_height = 0;
        if (chips.empty()) return;
        // The chip window includes a 2 DIP inset each side, so a 4 DIP window gap gives the 8 DIP visual gap.
        const int gap = chips.front()->Dpi() * static_cast<int>(FeedbackMetrics::ChipGap - 2 * FeedbackMetrics::ChipInset) / 96;
        int x = m_origin.x, y = m_origin.y, rowHeight = 0;
        for (auto* chip : chips)
        {
            const CSize size = chip->PreferredSize();
            if (x > m_origin.x && x + size.cx > m_origin.x + m_width) { x = m_origin.x; y += rowHeight; rowHeight = 0; }
            chip->SetWindowPos(nullptr, x, y, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
            x += size.cx + gap;
            rowHeight = (std::max)(rowHeight, static_cast<int>(size.cy));
        }
        m_height = y + rowHeight - m_origin.y;
    }
}
