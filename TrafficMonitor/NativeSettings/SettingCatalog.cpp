#include "../stdafx.h"
#include "SettingCatalog.h"

namespace NativeSettings
{
    bool Setting::Available() const
    {
#ifdef WITHOUT_TEMPERATURE
        return !temperature;
#else
        return true;
#endif
    }
    void Setting::Copy(Values& to, const Values& from) const
    {
        if (!Available()) return;
        if (enabled) to.*enabled=from.*enabled;
        if (number) to.*number=from.*number;
        if (trafficUnits) to.trafficUnit=from.trafficUnit;
    }
    const wchar_t* GroupName(int group,bool chinese)
    {
        static const wchar_t* en[]={L"Monitoring",L"Floating monitor",L"Taskbar",L"Notifications",L"Interface",L"All groups"};
        static const wchar_t* zh[]={L"監察",L"浮動監察器",L"工作列",L"通知",L"介面",L"所有組別"};
        return (chinese?zh:en)[group>=0&&group<6?group:0];
    }
    std::wstring FormatValue(const Setting& s,const Values& v,bool chinese)
    {
        std::wstring text;
        if(s.enabled)text=v.*s.enabled?(chinese?L"已啟用":L"Enabled"):(chinese?L"已停用":L"Disabled");
        if(s.number)
        {
            if(!text.empty())text+=L"; ";
            text+=std::to_wstring(v.*s.number)+L" ";
            if(s.trafficUnits)text+=v.trafficUnit==0?L"MB":v.trafficUnit==1?L"GB":(chinese?L"無效單位":L"Invalid unit");
            else if(s.unit)text+=s.unit;
        }
        return text;
    }
    const std::vector<Setting>& Catalog()
    {
        static const std::vector<Setting> rows={
            {L"interface.language",4,L"Interface language",L"介面語言",L"Choose English, Cantonese or bilingual text for native monitoring surfaces.",L"為原生監察介面選擇英文、廣東話或雙語文字。"},
            {L"interface.appearance",4,L"Appearance",L"外觀",L"Choose System, Light or Dark for native monitoring surfaces.",L"為原生監察介面選擇跟隨系統、淺色或深色。"},
            {L"monitor.interval",0,L"Sample interval",L"採樣間隔",L"Time between monitoring samples. 200 to 30000 ms, in steps of 100. Shorter intervals use more resources.",L"監察採樣之間嘅時間。200 至 30000 毫秒，以 100 為倍數；間隔越短，用嘅資源越多。",nullptr,&Values::interval,200,30000,100,L"ms"},
            {L"monitor.tray",0,L"Show notification-area icon",L"顯示通知區域圖示",L"Keep commands available in the notification area. Recovery may require the icon to remain visible.",L"喺通知區域保留操作入口；復原操作可能需要繼續顯示圖示。",&Values::tray},
            {L"monitor.update-check",0,L"Check for updates automatically",L"自動檢查更新",L"Checks every five minutes. Disabling this stops automatic checks, not monitoring.",L"每五分鐘檢查一次；停用後只停止自動檢查，監察照常。",&Values::automaticCheck},
            {L"monitor.update-restart",0,L"Restart automatically when idle",L"閒置時自動重新啟動",L"Off by default. When on, a ready update may restart the app by itself, only while its interface is idle and never after Later. Otherwise the update notice offers Restart to install update and Later.",L"預設關閉。開啟後，更新就緒而介面閒置時，程式可自行重新啟動，揀咗「稍後」就唔會。否則由更新通知提供「重新啟動以安裝更新」同「稍後」。",&Values::automaticRestart},
            {L"floating.compact",1,L"Compact network-speed text",L"精簡網速文字",L"Shortens rate formatting in the floating monitor.",L"縮短浮動監察器嘅網速文字。",&Values::floatingCompact},
            {L"floating.bytes",1,L"Use bytes instead of bits",L"以位元組取代位元",L"Selects byte-based units for floating-monitor rates.",L"浮動監察器嘅網速改用位元組單位。",&Values::floatingBytes},
            {L"floating.space",1,L"Separate values and units",L"數值同單位分開",L"Adds a space between floating-monitor values and their units.",L"喺浮動監察器數值同單位之間加空格。",&Values::floatingSpace},
            {L"floating.tips",1,L"Show monitor tooltips",L"顯示監察器工具提示",L"Shows the existing floating-monitor tooltip when pointing at it.",L"指向浮動監察器時顯示原有工具提示。",&Values::floatingTips},
            {L"floating.top",1,L"Keep floating monitor above other windows",L"浮動監察器保持最上層",L"Uses the existing always-on-top behavior for the floating monitor.",L"使用原有最上層功能顯示浮動監察器。",&Values::top},
            {L"floating.lock",1,L"Lock floating monitor position",L"鎖定浮動監察器位置",L"Prevents moving the floating monitor until this setting is cleared.",L"啟用後無法移動浮動監察器，取消設定先可以再移動。",&Values::locked},
            {L"taskbar.compact",2,L"Compact network-speed text",L"精簡網速文字",L"Shortens rate formatting in the taskbar monitor.",L"縮短工作列監察器嘅網速文字。",&Values::taskbarCompact},
            {L"taskbar.bytes",2,L"Use bytes instead of bits",L"以位元組取代位元",L"Selects byte-based units for taskbar rates.",L"工作列嘅網速改用位元組單位。",&Values::taskbarBytes},
            {L"taskbar.space",2,L"Separate values and units",L"數值同單位分開",L"Adds a space between taskbar values and units.",L"喺工作列數值同單位之間加空格。",&Values::taskbarSpace},
            {L"taskbar.tips",2,L"Show taskbar tooltips",L"顯示工作列工具提示",L"Shows the existing taskbar monitor tooltip.",L"顯示原有工作列監察器工具提示。",&Values::taskbarTips},
            {L"taskbar.right",2,L"Align taskbar values to the right",L"工作列數值靠右對齊",L"Aligns values within the existing taskbar monitor layout.",L"喺原有工作列監察器版面內將數值靠右對齊。",&Values::right},
            {L"taskbar.horizontal",2,L"Arrange taskbar values horizontally",L"工作列數值橫向排列",L"Uses the existing horizontal taskbar arrangement.",L"使用原有橫向工作列排列。",&Values::horizontal},
            {L"notice.traffic",3,L"Daily traffic notification",L"每日流量通知",L"Notifies when daily traffic crosses the threshold. Applying a value does not send a notification. Range 1 to 32767 MB or GB.",L"每日流量超過門檻時通知；套用數值唔會即時發出通知。範圍 1 至 32767 MB 或 GB。",&Values::trafficNotice,&Values::trafficLimit,1,32767,1,nullptr,true},
            {L"notice.memory",3,L"Memory-use notification",L"記憶體使用量通知",L"Uses the existing threshold-crossing notification and notification interval. Range 1 to 100 percent.",L"使用原有超標通知同通知間隔，範圍 1 至 100%。",&Values::memoryNotice,&Values::memoryLimit,1,100,1,L"%"},
            {L"notice.cpu",3,L"CPU temperature notification",L"CPU 溫度通知",L"Notifies on threshold crossing only when CPU temperature monitoring is available and enabled. Range 1 to 120 Celsius. Does not enable hardware monitoring.",L"CPU 溫度監察可用並已啟用時先會發出超標通知，範圍 1 至 120 °C。唔會自動啟用硬件監察。",&Values::cpuNotice,&Values::cpuLimit,1,120,1,L"°C",false,true},
            {L"notice.gpu",3,L"GPU temperature notification",L"GPU 溫度通知",L"Uses existing GPU threshold crossing and notification interval. Requires enabled temperature monitoring; range 1 to 120 Celsius.",L"使用原有 GPU 超標通知同通知間隔；需要啟用溫度監察，範圍 1 至 120 °C。",&Values::gpuNotice,&Values::gpuLimit,1,120,1,L"°C",false,true},
            {L"notice.disk",3,L"Disk temperature notification",L"磁碟溫度通知",L"Uses existing disk threshold crossing and notification interval. Requires enabled temperature monitoring; range 1 to 120 Celsius.",L"使用原有磁碟超標通知同通知間隔；需要啟用溫度監察，範圍 1 至 120 °C。",&Values::diskNotice,&Values::diskLimit,1,120,1,L"°C",false,true},
            {L"notice.board",3,L"Mainboard temperature notification",L"主機板溫度通知",L"Uses existing mainboard threshold crossing and notification interval. Requires enabled temperature monitoring; range 1 to 120 Celsius.",L"使用原有主機板超標通知同通知間隔；需要啟用溫度監察，範圍 1 至 120 °C。",&Values::boardNotice,&Values::boardLimit,1,120,1,L"°C",false,true}
        };
        return rows;
    }
}
