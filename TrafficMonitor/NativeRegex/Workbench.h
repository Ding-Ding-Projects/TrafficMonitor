#pragma once
#include "Engine.h"
#include "../NativeMaterial/Theme.h"
namespace NativeRegex
{
    bool Edit(HWND owner, HWND anchor, Options& accepted, const std::vector<std::wstring>& subjects, bool chinese,
        NativeMaterial::Appearance appearance = NativeMaterial::Appearance::Light);
}
