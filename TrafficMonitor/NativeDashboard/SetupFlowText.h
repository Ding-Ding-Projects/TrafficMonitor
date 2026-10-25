#pragma once
#include <afxstr.h>

namespace NativeDashboard::Setup
{
    enum class Copy { Title, LanguageStep, AdapterStep, StartupStep, ReviewStep, Language, EnglishStyle, CantoneseStyle,
        ModeLabel, AdapterLabel, TaskbarLabel, StartupLabel, Keep, Enable, Disable, Automatic, All, One,
        Back, Next, Apply, SaveClose, Refresh, KeepRemaining, LanguageInfo, AdapterInfo, StartupInfo,
        ReviewInfo, SaveFailed, AdapterChanged, Incomplete, Applying, NotStarted, Applied, Failed, Complete,
        CurrentOn, CurrentOff, NoAdapters, Resume, Protected, CloseUnsaved, StyleScope, Count };
    inline CString Text(Copy id, int language)
    {
        struct Pair { const wchar_t* en; const wchar_t* zh; };
        static const Pair entries[] = {
            {L"TrafficMonitor setup",L"TrafficMonitor 設定精靈"},
            {L"1 of 4 · Language and message style",L"第 1／4 步 · 語言及訊息風格"},
            {L"2 of 4 · Choose your monitoring scope",L"第 2／4 步 · 選擇監察範圍"},
            {L"3 of 4 · Review taskbar and login startup",L"第 3／4 步 · 檢視工作列及登入啟動"},
            {L"4 of 4 · Review and apply",L"第 4／4 步 · 檢視及套用"},
            {L"Setup language",L"設定精靈語言"}, {L"English message style (1 serious, 5 playful)",L"英文訊息風格（1 嚴肅，5 輕鬆）"},
            {L"Cantonese message style (1 serious, 5 playful)",L"廣東話訊息風格（1 嚴肅，5 輕鬆）"},
            {L"Monitoring mode",L"監察模式"}, {L"One adapter",L"單一網絡介面"},
            {L"Taskbar display",L"工作列顯示"}, {L"Start at login",L"登入時啟動"},
            {L"Keep current",L"保留目前設定"}, {L"Enable",L"啟用"}, {L"Disable",L"停用"},
            {L"Automatic",L"自動選擇"}, {L"All eligible adapters",L"所有合資格介面"}, {L"One adapter",L"單一介面"},
            {L"Back",L"上一步"}, {L"Next",L"下一步"}, {L"Apply",L"套用"}, {L"Save and close",L"儲存並關閉"},
            {L"Refresh adapters",L"重新整理介面"}, {L"Keep remaining current values",L"其餘保留目前設定"},
            {L"Choose your language and appearance, then apply those preferences. Next keeps monitoring changes for the final step.",
             L"選擇語言及外觀，再套用介面偏好。下一步會保留監察更改，等候最後一步套用。"},
            {L"Choose a live adapter identity. Automatic and All retain existing monitor semantics. A removed or changed adapter must be selected again. History and accounting are preserved.",
             L"請選擇目前存在的介面。自動及全部模式沿用現有監察行為。介面移除或變更後必須重新選擇。流量記錄及統計保留不變。"},
            {L"Keep current is the initial choice for both controls. Login startup uses the existing supported registration. Closing this wizard never enables startup.",
             L"兩項預設均保留目前設定。登入啟動使用現有支援的登記方式。關閉精靈不會啟用登入啟動。"},
            {L"Apply checks each operation separately. Successful effects are retained if another operation fails. Retry verifies existing results before applying anything again.",
             L"套用會逐項檢查操作。即使另一項未完成，已成功的更改仍會保留。重試先核對現有結果，才決定是否需要再次套用。"},
            {L"The setup draft could not be saved. Keep this window open and retry. Completion has not been recorded.",L"未能儲存設定草稿。請保留此視窗並重試，尚未記錄完成。"},
            {L"Adapter discovery changed or the chosen adapter is missing. Refresh and choose again; no new operation was applied.",L"介面資料已變更，或所選介面已不存在。請重新整理並選擇，尚未套用新操作。"},
            {L"Setup remains incomplete. Successful operations are retained. Retry, change the choice, or keep remaining current values.",L"設定尚未完成。已成功的操作保留不變。請重試、更改選項，或將其餘項目保留目前設定。"},
            {L"Applying and checking saved results",L"正在套用及核對儲存結果"}, {L"Not started",L"尚未開始"},
            {L"Applied and read back",L"已套用並核對"}, {L"Incomplete",L"未完成"}, {L"Setup complete",L"設定完成"},
            {L"Currently enabled",L"目前已啟用"}, {L"Currently disabled",L"目前已停用"},
            {L"No eligible adapter available",L"目前沒有合資格介面"}, {L"Resume setup",L"繼續設定"},
            {L"This setup file is unreadable or uses an unsupported format. It has been preserved. Close without saving, then restore a supported file before continuing setup.",L"設定檔無法讀取或格式尚未支援，原檔已保留。請關閉而不儲存，還原支援的檔案後再繼續設定。"},
            {L"Close without saving",L"關閉而不儲存"},
            {L"Message styles affect this setup wizard only. Each language has its own range.",L"訊息風格只影響此設定精靈；每種語言均有獨立範圍。"}
        };
        static_assert(sizeof(entries)/sizeof(entries[0]) == static_cast<size_t>(Copy::Count));
        const auto& entry = entries[static_cast<size_t>(id)];
        if (language == 1) return entry.zh;
        if (language == 2) return CString(entry.en) + L" / " + entry.zh;
        return entry.en;
    }
    inline CString StyleNote(int language, int english, int cantonese)
    {
        static const wchar_t* en[] = {L"Changes wait for Apply.", L"Your choices are ready when you are.", L"The draft is waiting patiently for Apply.", L"The draft has its checklist, but keeps its hands off the switches.", L"The draft has a clipboard, not permission to press your switches."};
        static const wchar_t* zh[] = {L"更改會等候套用。", L"選項已準備好，等你套用。", L"草稿耐心等候，不會搶先套用。", L"草稿已寫好清單，未到套用不碰開關。", L"草稿只是拿着清單，未獲套用就不會擅自開工。"};
        if(language==1)return zh[cantonese-1];
        if(language==2)return CString(en[english-1])+L"\r\n"+zh[cantonese-1];
        return en[english-1];
    }
}
