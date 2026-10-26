#pragma once
#include "InputControls.h"
#include "FeedbackVisuals.h"
#include <functional>
#include <memory>

// Self-drawn Material 3 chips: assist, filter, input and suggestion. Each chip is a real child HWND painted with
// Direct2D, one tab stop, keyboard operable and exposed through UI Automation (a button; a filter chip is a toggle).
//
//   ChipSet chips;  chips.Attach(parentWindow);
//   chips.Add(ChipKind::Filter, L"Unread", 201);          // selectable, shows a check when selected
//   chips.Add(ChipKind::Input, L"alice@example.com", 202); // removable (close icon, Delete or Backspace)
//   int height = chips.Layout(CPoint(16, 16), widthInPixels);   // wraps onto rows; call from WM_SIZE
//
// Parent notifications (WM_COMMAND, the chip id in the low word):
//   BN_CLICKED         assist/suggestion chip activated, filter chip toggled (read IsSelected), input chip body activated
//   ChipNotifyRemoved  an input chip was removed (it is hidden at once and destroyed by the next ChipSet::Layout or Prune)
// Programmatic setters never notify. Keys: Space/Enter activate, Delete/Backspace remove an input chip,
// Left/Right/Home/End move to the neighbouring chip of the set. A chip window is 40 DIP high (the 32 DIP container
// plus its state layer and focus ring); chips have no leading icons other than the filter check.
namespace NativeMaterial
{
    constexpr UINT ChipNotifyRemoved = 0x7B01;

    class Chip final : public InputWindow
    {
    public:
        // The chip is created at topLeft (parent client pixels) and sized to its label.
        BOOL Create(ChipKind kind, const CString& label, CWnd* parent, UINT id, CPoint topLeft = CPoint(0, 0));
        ChipKind Kind() const { return m_kind; }
        void SetLabel(const CString& label);
        CString Label() const;
        // Filter and input chips only; silent. A selected filter chip shows the check and gets wider.
        void SetSelected(bool selected);
        bool IsSelected() const { return m_selected; }
        CSize PreferredSize() const;                 // physical pixels at the current DPI
        void SizeToContent();                        // keeps the top-left corner
        bool IsRemoved() const { return m_removed; }
        // Same as the user removing an input chip. Returns false for other kinds or an already removed chip.
        bool Remove();
        CRect PartRect(ChipPart part) const;         // client pixels
        ChipPart PartAtPoint(CPoint point) const { return PartAt(point); }
        // Called after a change of size or removal so a set can arrange its chips again.
        void SetChangedHandler(std::function<void(Chip&)> handler) { m_changed = std::move(handler); }
        // delta: -1 previous, +1 next, -2 first, +2 last.
        void SetNavigationHandler(std::function<void(Chip&, int delta)> handler) { m_navigate = std::move(handler); }
    protected:
        void Paint(ID2D1RenderTarget*, const Palette&, const InputState&) override;
        CSize MinimumLogicalSize() const override;
        void FillAccess(InputData&) const override;
        void PointerPressed(CPoint) override;
        void PointerReleased(CPoint, bool) override;
        void PointerMoved(CPoint) override;
        void PointerLeft() override;
        void PointerCanceled() override;
        bool KeyPressed(UINT, bool) override;
        bool KeyReleased(UINT) override;
        bool WantsKey(UINT) const override;
        UINT DialogCode() const override { return DLGC_BUTTON | DLGC_WANTARROWS; }
        void DpiChanged() override;
        void PerformAction(InputActionId, int) override;
    private:
        ChipPart PartAt(CPoint) const;
        ChipLayout CurrentLayout() const;
        void ActivateBody();
        bool Filled() const { return m_selected && (m_kind == ChipKind::Filter || m_kind == ChipKind::Input); }
        ChipKind m_kind{ ChipKind::Assist };
        bool m_selected{}, m_removed{};
        ChipPart m_hoverPart{ ChipPart::None }, m_pressedPart{ ChipPart::None };
        std::function<void(Chip&)> m_changed;
        std::function<void(Chip&, int)> m_navigate;
    };

    // Owns chips and lays them out left to right, wrapping onto new rows. A plain helper, not a window.
    class ChipSet
    {
    public:
        void Attach(CWnd* parent) { m_parent = parent; }
        Chip* Add(ChipKind kind, const CString& label, UINT id, bool selected = false);
        Chip* Find(UINT id) const;
        int Count() const;                           // chips that are not removed
        Chip* At(int index) const;                   // index among the chips that are not removed
        std::vector<UINT> SelectedIds() const;
        // Destroys removed chips. Never call from inside a chip notification.
        void Prune();
        // Prunes, then places every chip inside widthPixels starting at origin. Returns the total height in pixels.
        int Layout(CPoint origin, int widthPixels);
        // Places chips with the origin and width of the last Layout; used internally after a chip changed.
        void Arrange();
    private:
        std::vector<Chip*> Visible() const;
        CWnd* m_parent{};
        std::vector<std::unique_ptr<Chip>> m_chips;
        CPoint m_origin{};
        int m_width{ 1 };
        int m_height{};
    };
}
