#include "../stdafx.h"
#include "ActionSounds.h"
#pragma comment(lib,"winmm.lib")
namespace NativeSounds
{
    namespace
    {
        bool PlaySystemCue(Cue cue)
        {
            const DWORD_PTR alias=AliasFor(cue);
            // Asynchronous, no default-beep fallback, and no change to host volume or sound settings.
            return alias&&PlaySoundW(reinterpret_cast<LPCWSTR>(alias),nullptr,SND_ALIAS_ID|SND_ASYNC|SND_NODEFAULT|SND_NOSTOP)!=FALSE;
        }
        Throttle throttle;
    }
    void Notify(Action action)
    {
        // Playback failure (no device, muted by the host) never alters the action result.
        try{Dispatch(NativePreferences::Current().values.actionSounds,action,PlaySystemCue,GetTickCount64(),throttle);}catch(...){}
    }
}