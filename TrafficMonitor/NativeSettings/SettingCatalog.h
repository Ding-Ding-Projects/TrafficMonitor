#pragma once
#include "Settings.h"
#include "DraftValidation.h"
#include <vector>
#include <string>

namespace NativeSettings
{
    // Semantic identity is independent of row ordering, control IDs and translations.
    struct Setting
    {
        const wchar_t* id;
        int group;
        const wchar_t* english;
        const wchar_t* chinese;
        const wchar_t* explanation;
        const wchar_t* chineseExplanation;
        bool Values::* enabled{};
        int Values::* number{};
        int minimum{}, maximum{}, step{1};
        const wchar_t* unit{};
        bool trafficUnits{}, temperature{};
        bool Available() const;
        void Copy(Values& destination, const Values& source) const;
    };
    const std::vector<Setting>& Catalog();
    const wchar_t* GroupName(int group, bool chinese);
    std::wstring FormatValue(const Setting&, const Values&, bool chinese);
}
