#pragma once
#include <afxwin.h>
#include <vector>
#include "Theme.h"
namespace NativeMaterial
{
    COLORREF SettingsDisabledColor(Appearance appearance,bool background=false);

    // One window found by FindStockControls.
    struct StockControl
    {
        HWND window{};
        CString className;
    };
    // True for the window classes of stock Windows controls that a Material surface must not contain:
    // BUTTON, EDIT, COMBOBOX, LISTBOX, SCROLLBAR, RICHEDIT and the common controls (msctls_*, Sys*).
    // STATIC is deliberately not stock-flagged: read-only text may stay a themed STATIC.
    bool IsStockControlClass(const CString& className);
    // Walks every descendant of root and returns the stock controls among them. An EDIT is not reported
    // when the Material component that owns it is its direct parent (NumberStepper or LabeledTextField:
    // the borderless entry field that keeps IME, selection and clipboard). A window listed in allowed is
    // skipped together with its descendants; the owner names why (for example the custom result list).
    std::vector<StockControl> FindStockControls(HWND root,const std::vector<HWND>& allowed={});
}
