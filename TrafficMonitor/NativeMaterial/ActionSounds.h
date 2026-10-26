#pragma once
#include "../NativePreferences/Preferences.h"
#include <mmsystem.h>
#include <cstdint>
namespace NativeSounds
{
    // Meaningful native actions that may produce a cue. Each has exactly one fixed outcome class.
    enum class Action { SettingsApplied, ActionRefused, SearchNoResults, SetupCompleted };
    enum class Cue { None, Success, Failure, Notice };
    // Windows-provided system sound aliases only. No audio file is bundled and nothing is fetched.
    inline DWORD_PTR AliasFor(Cue cue)
    {
        switch(cue)
        {
        case Cue::Success: return static_cast<DWORD_PTR>(SND_ALIAS_SYSTEMASTERISK);
        case Cue::Failure: return static_cast<DWORD_PTR>(SND_ALIAS_SYSTEMHAND);
        case Cue::Notice: return static_cast<DWORD_PTR>(SND_ALIAS_SYSTEMEXCLAMATION);
        default: return 0;
        }
    }
    inline Cue CueFor(Action action)
    {
        switch(action)
        {
        case Action::SettingsApplied: case Action::SetupCompleted: return Cue::Success;
        case Action::ActionRefused: return Cue::Failure;
        case Action::SearchNoResults: return Cue::Notice;
        }
        return Cue::None;
    }
    // Pure policy: Off plays nothing; Important plays only failure cues; All plays every cue.
    inline Cue Decide(NativePreferences::ActionSounds level,Action action)
    {
        const Cue cue=CueFor(action);
        if(level==NativePreferences::ActionSounds::Off||cue==Cue::None)return Cue::None;
        if(level==NativePreferences::ActionSounds::Important&&cue!=Cue::Failure)return Cue::None;
        return cue;
    }
    // Cues closer together than this are dropped so repeated actions never stack or overlap.
    constexpr std::uint64_t MinimumGapMilliseconds=600;
    using Player=bool(*)(Cue);
    struct Throttle { std::uint64_t last{}; bool played{}; };
    // Injectable dispatch used by the application and the focused tests. A failing player is ignored by the caller.
    inline bool Dispatch(NativePreferences::ActionSounds level,Action action,Player player,std::uint64_t nowMilliseconds,Throttle& throttle)
    {
        const Cue cue=Decide(level,action);
        if(cue==Cue::None||!player)return false;
        if(throttle.played&&nowMilliseconds-throttle.last<MinimumGapMilliseconds)return false;
        throttle.last=nowMilliseconds;throttle.played=true;
        return player(cue);
    }
    // Reads the accepted preference, plays asynchronously through PlaySound and never reports a playback failure to the caller.
    void Notify(Action action);
}