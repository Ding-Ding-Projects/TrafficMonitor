#pragma once
#include <afxwin.h>
#include "../NativeRegex/Engine.h"

namespace NativeMenus
{
    // The caller retains the menu and all item/bitmap ownership.
    // TPM_RETURNCMD returns the selected ID; otherwise the owner receives WM_COMMAND.
    UINT Track(CMenu* menu, UINT flags, int x, int y, CWnd* owner, NativeRegex::Options* retainedSearch = nullptr);
}
