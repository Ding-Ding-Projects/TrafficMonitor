#pragma once
#include "Theme.h"
#include <afxstr.h>
namespace NativeMaterial
{
    enum class ButtonVariant { Primary,Tonal,Outlined,Text,Selector };
    void DrawButton(ID2D1RenderTarget*,const Palette&,const CString&,ButtonVariant,bool enabled,bool pressed,bool hover,bool focused);
}
