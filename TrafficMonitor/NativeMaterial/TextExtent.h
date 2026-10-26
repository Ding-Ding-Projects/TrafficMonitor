#pragma once
#include <windows.h>
#include <algorithm>
#include <vector>

namespace NativeMaterial
{
    // Height of word-wrapped text in 96-dpi units. The flags match a static control created with
    // SS_NOPREFIX, so the measured height equals what that control paints at the same pixel width.
    // DT_EDITCONTROL is required: the control breaks a word wider than the line (a long URL, for example),
    // while plain DT_WORDBREAK measures it as one overflowing line and under-reports the height.
    inline int WrappedTextHeight(HDC dc, const wchar_t* text, int widthPixels, UINT dpi)
    {
        RECT bounds{ 0, 0, (std::max)(1, widthPixels), 0 };
        DrawTextW(dc, text, -1, &bounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS | DT_EDITCONTROL);
        const int scale = static_cast<int>(dpi ? dpi : 96);
        return (bounds.bottom * 96 + scale - 1) / scale;
    }

    // Scrollable content must reach the lowest edge of every control actually placed in it.
    // Bottoms are in the same content units as the height, measured from the content origin.
    inline int CoveredContentHeight(int contentHeight, const std::vector<int>& placedBottoms)
    {
        int covered = contentHeight;
        for (const int bottom : placedBottoms) covered = (std::max)(covered, bottom);
        return covered;
    }
}
