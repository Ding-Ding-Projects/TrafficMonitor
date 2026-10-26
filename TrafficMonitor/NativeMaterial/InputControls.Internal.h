#pragma once
#include "InputControls.h"
#include "../NativePreferences/Text.h"

// Helpers shared by the input component sources. Not part of the public component API.
namespace NativeMaterial
{
    inline COLORREF ToColorRef(D2D1_COLOR_F c)
    {
        return RGB(BYTE((std::min)(255.f, c.r * 255 + .5f)), BYTE((std::min)(255.f, c.g * 255 + .5f)), BYTE((std::min)(255.f, c.b * 255 + .5f)));
    }
    inline LONG DipToPx(float dip, UINT dpi) { return static_cast<LONG>(std::lround(dip * (dpi ? dpi : 96) / 96.0)); }

    // Publishes the visible label as the MSAA / UIA name of a native child (an EDIT) that has no label sibling.
    void AnnotateAccessibleName(HWND window, const CString& name);
    // Creates the Segoe UI font used inside drawn fields, sized for the DPI.
    void CreateInputFont(CFont& font, UINT dpi);
    // Moves a single-line edit so its text line is centered inside a DIP rectangle of its parent.
    void PlaceCenteredEdit(CWnd& edit, CFont& font, D2D1_RECT_F dipRect, UINT dpi);
    // Message posted to a dropdown by its popup when the popup was deactivated.
    constexpr UINT DropdownPopupDeactivated = WM_APP + 0x542;
}
