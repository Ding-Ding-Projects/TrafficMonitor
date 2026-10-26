#pragma once
#include "../TrafficMonitor.h"
#include <functional>
namespace NativeSettings
{
    // UI-thread query. Every active settings editor blocks automatic restart, even without edits.
    void OpenInterface(CWnd* owner);
    bool IsSafeToRestart() noexcept;
    struct Values
    {
        int interval{1000};
        bool tray{true};
        bool automaticCheck{true}, automaticRestart{false};
        bool floatingCompact{}, floatingBytes{true}, floatingSpace{true}, floatingTips{true}, top{}, locked{};
        bool taskbarCompact{}, taskbarBytes{true}, taskbarSpace{true}, taskbarTips{true}, right{}, horizontal{true};
        bool trafficNotice{}, memoryNotice{}, cpuNotice{}, gpuNotice{}, diskNotice{}, boardNotice{};
        int trafficLimit{200}, trafficUnit{}, memoryLimit{80}, cpuLimit{80}, gpuLimit{80}, diskLimit{80}, boardLimit{80};
        static Values Current();
        void Patch(MainWndSettingData& main, TaskBarSettingData& taskbar, GeneralSettingData& general) const;
    };
    // Returns true only for explicit navigation to the complete legacy settings dialog.
    bool Show(CWnd* owner, int initialPage, const std::function<void(const Values&)>& apply);
}
