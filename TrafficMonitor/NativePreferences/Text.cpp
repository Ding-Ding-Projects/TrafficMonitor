#include "../stdafx.h"
#include "Text.h"
#include <array>
namespace NativePreferences
{
    CString Pair(const wchar_t* english,const wchar_t* cantonese,Language language)
    {if(language==Language::Cantonese)return cantonese&&*cantonese?cantonese:english;if(language==Language::Bilingual&&cantonese&&*cantonese)return CString(english)+L"\r\n"+cantonese;return english;}
    CString Text(TextId id,Language language)
    {
        struct Entry{const wchar_t* english;const wchar_t* cantonese;};
        static const std::array<Entry,static_cast<size_t>(TextId::Count)> entries{{
            {L"Language and appearance",L"語言同外觀"},{L"Interface language",L"介面語言"},{L"Appearance",L"外觀"},
            {L"Apply interface preferences",L"套用介面偏好"},{L"Revert interface changes",L"還原介面修改"},{L"Reset interface defaults",L"重設介面預設值"},
            {L"Use saved preferences",L"使用已儲存偏好"},{L"Reapply retained draft",L"重新套用保留草稿"},
            {L"Interface draft is not saved.",L"介面草稿未儲存。"},{L"Interface preferences saved and read back.",L"介面偏好已儲存並讀回。"},
            {L"Saved preferences changed. Use saved values or deliberately reapply this retained draft.",L"已儲存偏好有更改。可使用已儲存值，或者明確重新套用保留草稿。"},
            {L"Preferences file is protected or invalid. Original file retained; no changes were saved.",L"偏好檔受保護或無效。原檔已保留，未儲存任何修改。"},
            {L"Could not save interface preferences. Draft retained; retry or revert.",L"未能儲存介面偏好。草稿已保留，可重試或還原。"},
            {L"Save verification is uncertain. Reload saved preferences before editing again.",L"未能確定儲存驗證結果。請先重新載入偏好再修改。"},
            {L"Applies to the native workspace, setup, settings, menus and search editor. Original dialogs and plugins retain their own wording.",L"套用至原生工作區、設定精靈、設定、選單同搜尋編輯器。原有對話框同外掛程式保留原有文字。"},
            {L"Preview: monitoring ready",L"預覽：監察已就緒"},{L"English",L"English"},{L"Cantonese",L"廣東話"},{L"English + Cantonese",L"English + 廣東話"},
            {L"System",L"跟隨系統"},{L"Light",L"淺色"},{L"Dark",L"深色"},{L"Saved interface preferences loaded. No changes were written.",L"已載入介面偏好，未寫入任何修改。"},{L"Using startup defaults or migrated setup choices. No native preference file has been saved.",L"使用啟動預設或已轉移嘅精靈選項，尚未儲存原生偏好檔。"}
        }};
        const auto index=static_cast<size_t>(id);if(index>=entries.size())return L"";const auto& entry=entries[index];return Pair(entry.english,entry.cantonese,language);
    }
}
