#include "../stdafx.h"
#include "SettingsControls.h"
#include "InputControls.h"
namespace NativeMaterial
{
    COLORREF SettingsDisabledColor(Appearance appearance,bool background)
    {
        const auto c=Palette::Resolve(appearance);
        if(c.highContrast)return GetSysColor(background?COLOR_WINDOW:COLOR_GRAYTEXT);
        const float alpha=background?.12f:.38f;
        return RGB(BYTE((c.onSurface.r*alpha+c.surface.r*(1-alpha))*255),
            BYTE((c.onSurface.g*alpha+c.surface.g*(1-alpha))*255),
            BYTE((c.onSurface.b*alpha+c.surface.b*(1-alpha))*255));
    }
    bool IsStockControlClass(const CString& className)
    {
        static const wchar_t* const exact[]={L"Button",L"Edit",L"ComboBox",L"ComboLBox",L"ListBox",L"ScrollBar",L"ComboBoxEx32",L"ReBarWindow32",L"ToolbarWindow32",L"SysLink",L"RichEdit20W",L"RICHEDIT50W"};
        for(const auto* name:exact)if(className.CompareNoCase(name)==0)return true;
        return className.Left(6).CompareNoCase(L"msctls")==0||className.Left(3).CompareNoCase(L"Sys")==0||className.Left(9).CompareNoCase(L"tooltips_")==0;
    }
    namespace
    {
        bool OwnsEntryField(HWND parent)
        {
            CWnd* window=CWnd::FromHandlePermanent(parent);
            return window&&(dynamic_cast<NumberStepper*>(window)||dynamic_cast<LabeledTextField*>(window));
        }
        void Collect(HWND parent,const std::vector<HWND>& allowed,std::vector<StockControl>& found)
        {
            for(HWND child=::GetWindow(parent,GW_CHILD);child;child=::GetWindow(child,GW_HWNDNEXT))
            {
                bool skip=false;for(HWND item:allowed)skip=skip||item==child;
                if(skip)continue;
                wchar_t name[64]{};::GetClassNameW(child,name,static_cast<int>(std::size(name)));const CString className(name);
                if(IsStockControlClass(className)&&!(className.CompareNoCase(L"Edit")==0&&OwnsEntryField(parent)))found.push_back({child,className});
                Collect(child,allowed,found);
            }
        }
    }
    std::vector<StockControl> FindStockControls(HWND root,const std::vector<HWND>& allowed)
    {
        std::vector<StockControl> found;if(root&&::IsWindow(root))Collect(root,allowed,found);return found;
    }
}
