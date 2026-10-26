#include "../stdafx.h"
#include "DropdownSearchableMenu.h"
#include "../NativeMenus/SearchableMenu.h"

namespace NativeMaterial
{
    void UseSearchableMenuPopup(Dropdown& dropdown)
    {
        // The retained search survives across openings of this dropdown, like every other searchable menu.
        auto retained = std::make_shared<NativeRegex::Options>();
        dropdown.SetPresenter([retained](Dropdown& owner, const std::vector<CString>& items, int selected, const CRect& anchor) -> int
        {
            CMenu menu;
            menu.CreatePopupMenu();
            for (size_t i = 0; i < items.size(); ++i)
            {
                CString text(items[i]);
                text.Replace(L"&", L"&&");
                menu.AppendMenu(MF_STRING | (static_cast<int>(i) == selected ? MF_CHECKED : 0), static_cast<UINT_PTR>(i + 1), text);
            }
            const UINT chosen = NativeMenus::Track(&menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, anchor.left, anchor.bottom, &owner, retained.get());
            return chosen > 0 && chosen <= items.size() ? static_cast<int>(chosen) - 1 : -1;
        });
    }
}
