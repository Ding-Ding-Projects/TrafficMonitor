#pragma once
#include "Theme.h"
#include "ButtonVisual.h"
#include <afxwin.h>
#include <vector>

namespace NativeMaterial
{
    // Material 3 alert dialog. It replaces the operating-system message box everywhere in the
    // application: own frameless popup, rounded surface, optional icon, headline, wrapping and
    // scrolling supporting text, and one to three right-aligned actions.
    enum class AlertIcon { None, Information, Warning, Error, Question };

    struct AlertAction
    {
        int result{ IDOK };          // value returned when this action is chosen
        CString label;               // already localized
        bool destructive{};          // drawn with the error color
    };

    struct AlertSpec
    {
        HWND owner{};                // may be null (startup errors before any window exists)
        CString headline;            // empty selects a localized default for the icon
        CString body;
        AlertIcon icon{ AlertIcon::None };
        std::vector<AlertAction> actions;
        int defaultIndex{};          // action that receives the initial focus and Enter
        int dismissResult{};         // result for Esc and close; 0 selects the last action
        bool forceAppearance{};      // otherwise the shared appearance preference decides
        Appearance appearance{ Appearance::Light };
    };

    // Computed geometry in pixels, relative to the dialog client area.
    struct AlertLayout
    {
        SIZE size{};
        RECT icon{}, headline{}, bodyView{};
        std::vector<RECT> actions;
        int bodyContentHeight{};     // full wrapped height of the supporting text
        int bodyLines{};
        int bodyTextWidth{};         // widest wrapped line
        bool scrolls{};
        bool stacked{};              // actions placed one under another
    };

    // Shows the dialog modally on the calling thread and returns the chosen action result.
    int ShowAlert(const AlertSpec& spec);

    // MessageBox-shaped entry points. type accepts MB_OK, MB_OKCANCEL, MB_ABORTRETRYIGNORE,
    // MB_YESNOCANCEL, MB_YESNO, MB_RETRYCANCEL, MB_CANCELTRYCONTINUE, the MB_ICON* values and
    // MB_DEFBUTTON1..3. Esc and close return IDCANCEL, or IDNO / IDOK / IDIGNORE when the set
    // has no Cancel.
    AlertSpec SpecFromMessageBox(HWND owner, const CString& text, const CString& caption, UINT type);
    int Alert(HWND owner, const CString& text, const CString& caption, UINT type);
    inline int Alert(HWND owner, const CString& text, UINT type) { return Alert(owner, text, CString(), type); }
    // Owner is the application main window when it exists (works from worker threads too).
    int Alert(const CString& text, UINT type);

    // Pure geometry, used by the dialog and by the focused verification program.
    AlertLayout MeasureAlert(const AlertSpec& spec, UINT dpi, SIZE workArea, bool forceGdi);

    // Same-thread inspection and hooks used only by the verification program.
    struct AlertSnapshot
    {
        std::vector<int> results;
        std::vector<CString> labels;
        std::vector<int> variants;   // ButtonVariant per action
        std::vector<HWND> buttons;
        int focusedIndex{ -1 };      // logical focus
        int systemFocusIndex{ -1 };  // action whose window owns the keyboard focus
        int defaultIndex{};
        int dismissResult{};
        CString headline, body;
        AlertIcon icon{ AlertIcon::None };
        AlertLayout layout;
        bool gdi{};
        UINT dpi{ 96 };
        int scroll{};
    };
    bool InspectAlert(HWND dialog, AlertSnapshot& out);
    struct AlertTestHooks
    {
        void (*shown)(HWND dialog, void* context){};
        void* context{};
        bool forceGdi{};
    };
    void SetAlertTestHooks(const AlertTestHooks* hooks);
}
