// Standalone check for the update banner decision and text rules (NativeUpdate/BannerLogic.h).
// It touches no network, window or file, so the person-visible rules can be verified headlessly.
#include "BannerLogic.h"
#include <iostream>

using namespace NativeUpdate;
using namespace NativeUpdate::Banner;

static int failures{};
static void Expect(bool value, const char* name)
{
    std::cout << (value ? "PASS " : "FAIL ") << name << std::endl;
    if (!value) ++failures;
}

static Status Make(Phase phase, unsigned long long completed = 0, const wchar_t* version = L"1.0.72")
{
    Status status;
    status.phase = phase;
    status.version = version;
    status.completed = completed;
    return status;
}

static bool Contains(const std::wstring& text, const wchar_t* part) { return text.find(part) != std::wstring::npos; }

int main()
{
    // Release notes link: exact release page for a clean tag, nothing for anything suspicious.
    Expect(ReleaseNotesUrl("v1.0.72-run5-1") == "https://github.com/Ding-Ding-Projects/TrafficMonitor/releases/tag/v1.0.72-run5-1", "release notes link points at the exact GitHub release page");
    Expect(ReleaseNotesUrl("").empty(), "empty tag gives no link");
    Expect(ReleaseNotesUrl("v1/../x").empty(), "path characters in a tag give no link");
    Expect(ReleaseNotesUrl("v1 2").empty(), "space in a tag gives no link");

    // Ready banner is persistent until the person chooses; Later defers it for the session.
    UiState ui;
    Expect(ChooseMode(Make(Phase::Ready), ui) == Mode::Ready, "staged update shows the ready banner");
    ui.deferred = true;
    Expect(ChooseMode(Make(Phase::Ready), ui) == Mode::Hidden, "Later hides the banner for this session");
    ui = UiState{};
    ui.restartRequested = true;
    Expect(ChooseMode(Make(Phase::Ready), ui) == Mode::WaitingToRestart, "chosen restart waits visibly for open windows");

    // Background activity stays silent; a manual check reports its progress and its exact result.
    ui = UiState{};
    Expect(ChooseMode(Make(Phase::Checking), ui) == Mode::Hidden, "automatic checking shows nothing");
    Expect(ChooseMode(Make(Phase::Failed, 3), ui) == Mode::Hidden, "automatic failure is left to the Settings state");
    ui.manualActive = true;
    ui.manualBaseline = 3;
    Expect(ChooseMode(Make(Phase::Checking, 3), ui) == Mode::Checking, "manual check shows progress");
    Expect(ChooseMode(Make(Phase::Idle, 3), ui) == Mode::Checking, "manual check does not claim up to date before it finishes");
    Expect(ChooseMode(Make(Phase::Idle, 4), ui) == Mode::UpToDate, "finished manual check reports up to date");
    Expect(ChooseMode(Make(Phase::Failed, 4), ui) == Mode::Failed, "finished manual failure is shown, not hidden");
    ui.dismissed = true;
    Expect(ChooseMode(Make(Phase::Failed, 4), ui) == Mode::Hidden, "dismissed manual result stays hidden");

    // A manual check the updater never acknowledges becomes an error instead of a spinner.
    ui = UiState{};
    ui.manualActive = true;
    ui.manualBaseline = 0;
    ui.waitedMilliseconds = UnavailableAfterMilliseconds - 1;
    Expect(ChooseMode(Make(Phase::Disabled, 0), ui) == Mode::Checking, "unacknowledged manual check may still be starting");
    ui.waitedMilliseconds = UnavailableAfterMilliseconds;
    Expect(ChooseMode(Make(Phase::Disabled, 0), ui) == Mode::Failed, "unacknowledged manual check is reported as failed");

    // A restart the person chose that could not be prepared keeps its error on screen.
    ui = UiState{};
    ui.restartFailed = true;
    Expect(ChooseMode(Make(Phase::Failed), ui) == Mode::Failed, "failed chosen restart stays visible");

    // Restart only on the person's choice, and only when the interface is quiet.
    ui = UiState{};
    const Status ready = Make(Phase::Ready);
    Expect(!ShouldPrepareRestart(ready, ui, false, true), "no restart without a choice when automatic restart is off");
    ui.restartRequested = true;
    Expect(ShouldPrepareRestart(ready, ui, false, true), "chosen restart proceeds on a quiet interface");
    Expect(!ShouldPrepareRestart(ready, ui, false, false), "chosen restart waits while windows or drafts are open");
    Expect(!ShouldPrepareRestart(Make(Phase::Downloading), ui, false, true), "nothing restarts before the update is staged");
    ui = UiState{};
    Expect(ShouldPrepareRestart(ready, ui, true, true), "explicit automatic restart works on a quiet interface");
    Expect(!ShouldPrepareRestart(ready, ui, true, false), "automatic restart still waits for a quiet interface");
    ui.deferred = true;
    Expect(!ShouldPrepareRestart(ready, ui, true, true), "automatic restart never overrides Later");

    // Banner text: exact version, unsigned warning, exact error detail, both languages.
    const Text headline = Headline(Mode::Ready, ready);
    Expect(Contains(headline.english, L"1.0.72") && Contains(headline.chinese, L"1.0.72"), "headline carries the exact version in both languages");
    const Text detail = Detail(Mode::Ready, ready, L"");
    Expect(Contains(detail.english, L"not code-signed") && Contains(detail.english, L"SHA-256"), "ready banner warns that the artifact is unsigned and states how it was verified");
    Expect(!detail.chinese.empty() && Contains(detail.chinese, L"SHA-256"), "ready banner warning exists in Cantonese");
    Expect(Contains(detail.english, L"nothing closes until you choose"), "ready banner promises no interruption");
    Status failed = Make(Phase::Failed);
    failed.stage = L"package-sha256";
    failed.error = 13;
    failed.httpStatus = 200;
    const Text failure = Detail(Mode::Failed, failed, L"The data is invalid.");
    Expect(Contains(failure.english, L"package-sha256") && Contains(failure.english, L"13") && Contains(failure.english, L"The data is invalid.") && Contains(failure.english, L"200"),
        "failure text names the exact stage, code, system message and HTTP status");
    Expect(Contains(failure.chinese, L"package-sha256") && Contains(failure.chinese, L"13"), "failure text exists in Cantonese with the same facts");
    Status retry = ready;
    retry.error = 1460;
    retry.stage = L"restart-readiness";
    Expect(Contains(Detail(Mode::Ready, retry, L"timeout").english, L"restart-readiness"), "a failed restart attempt is shown on the ready banner");

    std::cout << "Banner checks: " << failures << " failed" << std::endl;
    return failures ? 1 : 0;
}
