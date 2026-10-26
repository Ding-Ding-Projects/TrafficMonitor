#pragma once
#include "SilentUpdater.h"
#include <string>

// Pure presentation decisions for the update banner. Nothing here touches the
// network, the file system or any window, so the rules can be checked by a
// standalone program. The UI owns the HWND and feeds this layer a snapshot.
namespace NativeUpdate
{
    namespace Banner
    {
        struct Text { std::wstring english, chinese; };

        enum class Mode { Hidden, Checking, UpToDate, Ready, WaitingToRestart, Failed };

        struct UiState
        {
            bool manualActive{};                 // A manual Check for updates is awaiting its result.
            unsigned long long manualBaseline{}; // Status::completed when the manual check was requested.
            bool deferred{};                     // Later was chosen for this session.
            bool restartRequested{};             // Restart to install update was chosen.
            bool dismissed{};                    // The manual-check result was dismissed.
            bool restartFailed{};                // A chosen restart could not be prepared; keep the error visible.
            unsigned long long waitedMilliseconds{}; // Time since the manual check was requested.
        };

        // A manual check that the updater never acknowledges is reported, not left as a spinner.
        constexpr unsigned long long UnavailableAfterMilliseconds = 3000;

        constexpr const char* ReleasePage = "https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/";

        // The tag is already validated by the release parser; this guard only
        // keeps a malformed value from ever becoming part of a link.
        inline std::string ReleaseNotesUrl(const std::string& tag)
        {
            if (tag.empty() || tag.size() > 64) return {};
            for (char c : tag)
            {
                const bool allowed = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '-' || c == '_';
                if (!allowed) return {};
            }
            return std::string(ReleasePage) + tag;
        }

        inline bool ResultAvailable(const Status& status, const UiState& ui)
        {
            return ui.manualActive && !ui.dismissed && status.completed > ui.manualBaseline;
        }

        inline Mode ChooseMode(const Status& status, const UiState& ui)
        {
            const bool manual = ui.manualActive && !ui.dismissed;
            switch (status.phase)
            {
            case Phase::Ready:
                if (ui.restartRequested) return Mode::WaitingToRestart;
                return ui.deferred ? Mode::Hidden : Mode::Ready;
            case Phase::Applying:
                return Mode::WaitingToRestart;
            case Phase::Failed:
                // A failure the person asked about, or that interrupted a restart
                // they chose, is always shown with its exact error.
                if (ui.restartRequested || ui.restartFailed || ResultAvailable(status, ui)) return Mode::Failed;
                return Mode::Hidden;
            case Phase::Checking:
            case Phase::Downloading:
                return manual ? Mode::Checking : Mode::Hidden;
            case Phase::Idle:
                return ResultAvailable(status, ui) ? Mode::UpToDate : (manual ? Mode::Checking : Mode::Hidden);
            default:
                if (ui.restartFailed) return Mode::Failed;
                if (!manual) return Mode::Hidden;
                if (ResultAvailable(status, ui)) return Mode::UpToDate;
                return ui.waitedMilliseconds >= UnavailableAfterMilliseconds ? Mode::Failed : Mode::Checking;
            }
        }

        // Restart is never initiated without the person's choice unless they turned
        // automatic restart on and have not postponed this session. Both routes
        // still require a quiet user interface.
        inline bool ShouldPrepareRestart(const Status& status, const UiState& ui, bool automaticRestartPreference, bool quietInterface)
        {
            if (status.phase != Phase::Ready || !quietInterface) return false;
            if (ui.restartRequested) return true;
            return automaticRestartPreference && !ui.deferred;
        }

        inline Text UnsignedWarning()
        {
            return { L"This update is not code-signed. Windows may show an unknown publisher warning. It is verified against the SHA-256 hashes published with the release.",
                     L"呢個更新冇程式碼簽署，Windows 可能顯示未知發行者警告。已根據發行版本公開嘅 SHA-256 雜湊值驗證。" };
        }

        inline Text FailureDetail(const Status& status, const std::wstring& systemMessage)
        {
            const std::wstring stage = status.stage.empty() ? L"unknown" : status.stage;
            const std::wstring code = std::to_wstring(status.error);
            const std::wstring http = std::to_wstring(status.httpStatus);
            Text text;
            text.english = L"Stage: " + stage + L". Error code " + code + L": " + (systemMessage.empty() ? L"no system description" : systemMessage) + L". HTTP status " + http + L".";
            text.chinese = L"階段：" + stage + L"。錯誤代碼 " + code + L"：" + (systemMessage.empty() ? L"冇系統描述" : systemMessage) + L"。HTTP 狀態 " + http + L"。";
            return text;
        }

        inline Text Headline(Mode mode, const Status& status)
        {
            const std::wstring version = status.version;
            switch (mode)
            {
            case Mode::Ready:
                return { L"TrafficMonitor " + version + L" is ready to install", L"TrafficMonitor " + version + L" 已準備好安裝" };
            case Mode::WaitingToRestart:
                return { L"Restarting to install TrafficMonitor " + version, L"正在重新啟動以安裝 TrafficMonitor " + version };
            case Mode::Checking:
                if (status.phase == Phase::Downloading)
                    return { L"Downloading TrafficMonitor " + version, L"正在下載 TrafficMonitor " + version };
                return { L"Checking for updates", L"正在檢查更新" };
            case Mode::UpToDate:
                return { L"TrafficMonitor is up to date", L"TrafficMonitor 已經係最新版本" };
            case Mode::Failed:
                return { L"The update did not complete", L"更新未能完成" };
            default:
                return {};
            }
        }

        // Detail lines for the banner body. The unsigned-artifact warning is part
        // of the ready and waiting states because that is when a person decides.
        inline Text Detail(Mode mode, const Status& status, const std::wstring& systemMessage)
        {
            const Text warning = UnsignedWarning();
            switch (mode)
            {
            case Mode::Ready:
            {
                Text text{ L"Version " + status.version + L" is downloaded, verified and staged. Restart when you are ready; nothing closes until you choose. ",
                           L"版本 " + status.version + L" 已下載、驗證同準備好。你準備好先重新啟動，喺你揀之前唔會關閉任何嘢。" };
                text.english += warning.english; text.chinese += warning.chinese;
                if (status.error)
                {
                    const Text failure = FailureDetail(status, systemMessage);
                    text.english += L" Last restart attempt failed. " + failure.english;
                    text.chinese += L"上次重新啟動失敗。" + failure.chinese;
                }
                return text;
            }
            case Mode::WaitingToRestart:
            {
                Text text{ L"The restart starts once open windows such as Settings are closed. ", L"關閉設定等已開啟嘅視窗後就會重新啟動。" };
                text.english += warning.english; text.chinese += warning.chinese;
                if (status.error)
                {
                    const Text failure = FailureDetail(status, systemMessage);
                    text.english += L" Retrying after: " + failure.english;
                    text.chinese += L"重試原因：" + failure.chinese;
                }
                return text;
            }
            case Mode::Checking:
                return { L"This runs in the background and does not interrupt monitoring.", L"喺背景執行，唔會打斷監察。" };
            case Mode::UpToDate:
                return { L"No newer verified release is available.", L"冇更新嘅已驗證發行版本。" };
            case Mode::Failed:
                return FailureDetail(status, systemMessage);
            default:
                return {};
            }
        }
    }
}
