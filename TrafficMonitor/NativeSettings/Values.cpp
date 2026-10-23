#include "../stdafx.h"
#include "Settings.h"
namespace NativeSettings
{
    Values Values::Current()
    {
        Values v; const auto& m = theApp.m_main_wnd_data; const auto& t = theApp.m_taskbar_data;
        v.interval = theApp.m_general_data.monitor_time_span; v.tray = theApp.m_general_data.show_notify_icon;
        v.floatingCompact=m.speed_short_mode; v.floatingBytes=m.unit_byte; v.floatingSpace=m.separate_value_unit_with_space;
        v.floatingTips=m.show_tool_tip; v.top=m.m_always_on_top; v.locked=m.m_lock_window_pos;
        v.taskbarCompact=t.speed_short_mode; v.taskbarBytes=t.unit_byte; v.taskbarSpace=t.separate_value_unit_with_space;
        v.taskbarTips=t.show_tool_tip; v.right=t.value_right_align; v.horizontal=t.horizontal_arrange;
        return v;
    }
    void Values::Patch(MainWndSettingData& m, TaskBarSettingData& t, GeneralSettingData& g) const
    {
        g.monitor_time_span=interval; g.show_notify_icon=tray;
        m.speed_short_mode=floatingCompact; m.unit_byte=floatingBytes; m.separate_value_unit_with_space=floatingSpace;
        m.show_tool_tip=floatingTips; m.m_always_on_top=top; m.m_lock_window_pos=locked;
        t.speed_short_mode=taskbarCompact; t.unit_byte=taskbarBytes; t.separate_value_unit_with_space=taskbarSpace;
        t.show_tool_tip=taskbarTips; t.value_right_align=right; t.horizontal_arrange=horizontal;
    }
}
