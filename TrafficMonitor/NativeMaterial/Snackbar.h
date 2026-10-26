#pragma once
#include "InputControls.h"
#include "FeedbackVisuals.h"
#include <deque>
#include <functional>

// Self-drawn Material 3 snackbar: a non-blocking message anchored to the bottom-left of a host window.
//
//   Snackbar bar;  bar.Create(hostWindow, id);            // hidden until a message is shown
//   bar.Show({ L"Saved", L"Undo", true });                // queued when another message is showing
//   bar.Reposition();                                     // from the host's WM_SIZE
//   host PreTranslateMessage:  if (bar.TranslateShortcut(*pMsg)) return TRUE;   // F6 focuses the snackbar
//
// Behaviour
//   * Never takes focus on its own and never activates a window. A click on it does not move focus either.
//     F6 (TranslateShortcut) or Tab moves keyboard focus to it; F6 or Esc leaves, and Esc also dismisses.
//     Left/Right move between the action and the close icon, Enter/Space activate the selected part.
//   * One message at a time; the rest wait in a queue of at most 16 (the oldest waiting message is dropped).
//   * Timeout: a message without an action stays 5 s, one with an action stays 10 s (the user needs time to reach
//     it); an explicit timeout is clamped to 4 to 10 s. persistent messages stay until dismissed. The countdown
//     is held while the pointer is over the snackbar or keyboard focus is inside it and restarts afterwards.
//   * Announced through the UI Automation notification event (polite, or important when persistent) and exposed as
//     a group with a polite live setting and Invoke (runs the action, or dismisses when there is none).
//   * The snackbar has no motion, so reduced-motion settings have nothing to remove. It has no elevation shadow.
// Parent notifications (WM_COMMAND, the control id in the low word):
//   SnackbarNotifyAction     the action was chosen (sent before the dismissal)
//   SnackbarNotifyDismissed  the message went away; LastResult() and LastResultId() say why and which
namespace NativeMaterial
{
    enum class SnackbarResult { None, Timeout, Action, Close, Dismissed, Dropped };
    constexpr UINT SnackbarNotifyAction = 0x7A01, SnackbarNotifyDismissed = 0x7A02;
    constexpr unsigned SnackbarMinimumTimeoutMs = 4000, SnackbarMaximumTimeoutMs = 10000, SnackbarMaximumQueued = 16;

    struct SnackbarMessage
    {
        CString text;                 // one or two lines; longer text ends with an ellipsis
        CString action;               // optional action label, for example "Undo"
        bool closable{};              // show the close icon
        bool persistent{};            // stay until dismissed (use for warnings and errors)
        unsigned timeoutMs{};         // 0 selects the default; otherwise clamped to 4000..10000
        std::function<void(unsigned id, SnackbarResult)> onResult;  // called once, when the message goes away
    };

    class Snackbar final : public InputWindow
    {
    public:
        // Creates the hidden window as a child of the host. The control id is used for the parent notifications.
        BOOL Create(CWnd* host, UINT id);
        // Returns the message id (never 0), or 0 when the window does not exist or the text is empty.
        unsigned Show(const SnackbarMessage& message);
        void Dismiss();               // the current message only; the next queued one appears
        void Clear();                 // the current message and the whole queue
        bool IsShowing() const { return m_showing; }
        int QueuedCount() const { return static_cast<int>(m_queue.size()); }
        unsigned CurrentId() const { return m_showing ? m_current.id : 0; }
        CString CurrentText() const { return m_showing ? m_current.message.text : CString(); }
        SnackbarResult LastResult() const { return m_lastResult; }
        unsigned LastResultId() const { return m_lastId; }
        // Places the snackbar at the bottom-left of the host client area; call from the host's size handler.
        void Reposition();
        // Pure policy: the time a message stays, in milliseconds (0 = until dismissed).
        static unsigned EffectiveTimeout(const SnackbarMessage& message);
        // Handles F6 for the host. Returns true when the message was consumed.
        bool TranslateShortcut(const MSG& message);
        bool FocusMessage();
        bool HasKeyboardFocus() const { return m_hasFocus; }
        // Evaluates the timeout against the clock; the internal timer calls this every 250 ms.
        void Tick();
        // Verification hooks.
        static void SetClock(std::function<ULONGLONG()> clock);     // milliseconds; an empty function restores the system clock
        using NotificationObserver = std::function<void(const CString& text, bool important)>;
        static void SetNotificationObserver(NotificationObserver observer);
        HRESULT LastNotificationResult() const { return m_notifyResult; }
        SnackbarPart FocusedPart() const { return m_focusPart; }
        CRect PartRect(SnackbarPart part) const;     // client pixels
        bool TwoLines() const { return m_twoLines; }
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
        UINT DialogCode() const override;
        bool IsFocused() const override { return m_hasFocus; }
        void DpiChanged() override;
        void PerformAction(InputActionId, int) override;
        LRESULT WindowProc(UINT, WPARAM, LPARAM) override;
    private:
        struct Entry { unsigned id{}; SnackbarMessage message; };
        void Present(Entry entry);
        void Finish(SnackbarResult result);
        void Activate(SnackbarPart part);
        SnackbarPart PartAt(CPoint) const;
        SnackbarLayout CurrentLayout() const;
        void Announce();
        void LeaveFocus();
        void ArmClock();
        std::deque<Entry> m_queue;
        Entry m_current;
        bool m_showing{}, m_hasFocus{}, m_held{}, m_twoLines{};
        unsigned m_nextId{ 1 }, m_lastId{}, m_timeout{};
        ULONGLONG m_deadline{};
        SnackbarResult m_lastResult{ SnackbarResult::None };
        SnackbarPart m_focusPart{ SnackbarPart::None }, m_hoverPart{ SnackbarPart::None }, m_pressedPart{ SnackbarPart::None };
        HWND m_returnFocus{};
        HRESULT m_notifyResult{ E_PENDING };
    };
}
