#pragma once
#include "InputControls.h"

namespace NativeMaterial
{
    // Routes the dropdown through the application searchable menu (NativeMenus::Track) so the list gets the
    // same search field and anchored full regex builder as every other menu. Application builds only: the
    // menu belongs to the main project and is not part of the focused input verification.
    void UseSearchableMenuPopup(Dropdown& dropdown);
}
