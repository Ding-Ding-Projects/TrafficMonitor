#pragma once
#include "../NativeMaterial/Theme.h"
#include "../NativeMonitoringSnapshot.h"
#include <string>

namespace NativeDashboard
{
    std::wstring Rate(const std::optional<double>& value, bool chinese);
    std::wstring RateStateText(NativeMonitoring::RateState state, bool chinese);
    std::wstring TimeText(std::uint64_t fileTime);
    std::wstring Details(const NativeMonitoring::AdapterSnapshot* row, bool selected, bool chinese);
    void DrawData(NativeMaterial::Canvas& canvas, const NativeMaterial::Palette& colors,
        float width, float chartTop, float chartBottom,
        const NativeMonitoring::AdapterSnapshot* row, bool chinese);
}
