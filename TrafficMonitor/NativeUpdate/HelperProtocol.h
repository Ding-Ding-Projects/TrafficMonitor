#pragma once
#include <windows.h>

namespace NativeUpdate::Detail
{
    constexpr DWORD HelperPreparationTimeout = 10000;
    enum class PreparationState { Preparing, Validated, Failed };

    class OwnedHandle
    {
        HANDLE value{};
    public:
        OwnedHandle() = default;
        explicit OwnedHandle(HANDLE handle) : value(handle) {}
        ~OwnedHandle() { reset(); }
        OwnedHandle(const OwnedHandle&) = delete;
        OwnedHandle& operator=(const OwnedHandle&) = delete;
        HANDLE get() const { return value; }
        void reset(HANDLE handle = nullptr)
        {
            if(value && value != INVALID_HANDLE_VALUE)CloseHandle(value);
            value=handle;
        }
    };

    // One unnamed, handle-bound handshake. The parent retains the process and
    // signals until either validated exit permission or cooperative cancellation.
    struct HelperPreparation
    {
        OwnedHandle process, identity, validated, proceed, cancel;
        ULONGLONG deadline{GetTickCount64()+HelperPreparationTimeout};
        DWORD error{};
        bool committed{};

        ~HelperPreparation() { Cancel(ERROR_CANCELLED); }
        void Cancel(DWORD reason)
        {
            if(committed)return;
            if(!error)error=reason?reason:ERROR_GEN_FAILURE;
            if(cancel.get())SetEvent(cancel.get());
        }
        bool CreateSignals()
        {
            SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,FALSE};
            validated.reset(CreateEventW(&attributes,TRUE,FALSE,nullptr));
            if(!validated.get()){error=GetLastError();return false;}
            proceed.reset(CreateEventW(&attributes,TRUE,FALSE,nullptr));
            if(!proceed.get()){error=GetLastError();return false;}
            cancel.reset(CreateEventW(&attributes,TRUE,FALSE,nullptr));
            if(!cancel.get()){error=GetLastError();return false;}
            return true;
        }
        bool Exited() const
        { return !process.get()||WaitForSingleObject(process.get(),0)==WAIT_OBJECT_0; }
        PreparationState Observe()
        {
            if(!process.get()){Cancel(error?error:ERROR_INVALID_HANDLE);return PreparationState::Failed;}
            auto state=WaitForSingleObject(process.get(),0);
            if(state==WAIT_OBJECT_0)
            {
                DWORD result{};
                if(!GetExitCodeProcess(process.get(),&result))result=GetLastError();
                Cancel(result?result:ERROR_PROCESS_ABORTED);
                return PreparationState::Failed;
            }
            if(state!=WAIT_TIMEOUT){Cancel(GetLastError());return PreparationState::Failed;}
            if(error)return PreparationState::Failed;
            if(GetTickCount64()>=deadline){Cancel(ERROR_TIMEOUT);return PreparationState::Failed;}
            state=WaitForSingleObject(validated.get(),0);
            if(state==WAIT_TIMEOUT)return PreparationState::Preparing;
            if(state!=WAIT_OBJECT_0){Cancel(GetLastError());return PreparationState::Failed;}
            // A signaled event from a helper which already exited is never ready.
            state=WaitForSingleObject(process.get(),0);
            if(state!=WAIT_TIMEOUT){Cancel(ERROR_PROCESS_ABORTED);return PreparationState::Failed;}
            return PreparationState::Validated;
        }
        bool PermitParentExit()
        {
            if(Observe()!=PreparationState::Validated)return false;
            if(!SetEvent(proceed.get())){Cancel(GetLastError());return false;}
            committed=true;
            return true;
        }
    };
}
