#pragma once
#include "Theme.h"
#include "Controls.h"
#include "InputControls.h"
#include <afxwin.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Base of the modal Material 3 picker dialogs (color, font). Like the alert dialog it is an own frameless
// popup with a rounded surface and its own message loop, so no #32770 dialog, no stock button and no common
// dialog is involved. A derived dialog creates its content children, lays them out inside ContentRectPx() and
// paints any decoration behind them. The action row (Cancel, OK) belongs to the base.
//
// Keyboard: Tab / Shift+Tab walk FocusOrder(), Enter accepts (unless CanAccept() is false), Esc cancels.
namespace NativeMaterial
{
    // Same-thread hooks used only by the verification program. `shown` runs once the dialog is visible and
    // before the message loop starts; calling Finish there ends the dialog without any real interaction.
    struct PickerTestHooks
    {
        void (*shown)(HWND dialog, void* context){};
        void* context{};
        bool forceAppearance{};
        Appearance appearance{ Appearance::Light };
    };
    void SetPickerTestHooks(const PickerTestHooks* hooks);

    class PickerDialog : public CWnd
    {
    public:
        PickerDialog();
        ~PickerDialog() override;
        // Shows the dialog modally on the calling thread. Returns IDOK or IDCANCEL.
        int RunModal(HWND owner, const CString& title, int widthDip, int heightDip);
        void Finish(int result);
        bool Finished() const { return m_done; }
        int Result() const { return m_result; }
        const CString& Title() const { return m_title; }
        HWND OkWindow() const { return m_ok ? m_ok->GetSafeHwnd() : nullptr; }
        HWND CancelWindow() const { return m_cancel ? m_cancel->GetSafeHwnd() : nullptr; }
        void SetOkEnabled(bool enabled);
        bool IsOkEnabled() const;
        Appearance GetAppearance() const { return m_appearance; }
        COLORREF SurfaceRef() const;
        UINT DialogDpi() const { return m_dpi; }
        int Px(float dip) const;
        float Dip(int px) const;
        float WidthDip() const { return static_cast<float>(m_widthDip); }
        float HeightDip() const { return static_cast<float>(m_heightDip); }
        // The area between the title and the action row, in client pixels and in DIPs.
        CRect ContentRectPx() const;
        D2D1_RECT_F ContentRectDip() const;
        // Handles Esc, Enter and Tab exactly as the modal loop does. Returns whether the key was consumed.
        bool HandleKey(UINT virtualKey, bool shift);
        // Focusable windows in Tab order: the content, then Cancel and OK when usable.
        std::vector<HWND> FocusOrder() const;
        // Paints the whole dialog surface (also the WM_PRINTCLIENT route).
        HRESULT PaintTo(HDC dc);
        static PickerDialog* FromWindow(HWND window);
    protected:
        virtual void CreateContent() = 0;
        virtual void LayoutContent() = 0;
        virtual void PaintContent(ID2D1RenderTarget*, const Palette&) {}
        virtual std::vector<HWND> ContentFocusOrder() const = 0;
        virtual bool CanAccept() const { return true; }
        // Applies typed but uncommitted input (for example a stepper edit) before the dialog accepts; false keeps it open.
        virtual bool CommitPending() { return true; }
        virtual HWND InitialFocus() const { return nullptr; }
        // Called for WM_COMMAND from the content children; return true when consumed.
        virtual bool ContentCommand(UINT /*id*/, UINT /*code*/) { return false; }
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
        // Applies the dialog appearance and surface color to a content child.
        template<class T> void Style(T& child)
        {
            child.SetAppearance(m_appearance);
            child.SetSurfaceColor(SurfaceRef());
        }
        void MovePx(CWnd& child, float x, float y, float w, float h);
    private:
        void LayoutAll(bool keepCenter);
        void MoveFocus(int step);
        bool PretranslateKey(const MSG& msg);

        std::unique_ptr<Button> m_ok, m_cancel;
        CString m_title;
        Appearance m_appearance{ Appearance::Light };
        UINT m_dpi{ 96 };
        RECT m_work{};
        int m_widthDip{}, m_heightDip{}, m_result{ IDCANCEL };
        bool m_done{};
        bool m_okEnabled{ true };
        struct Access
        {
            std::mutex mutex;
            HWND window{};
            std::wstring name;
        };
        std::shared_ptr<Access> m_access;
    };
}
