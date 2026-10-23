#pragma once
#include "Preferences.h"
namespace NativePreferences
{
    class Subscription final
    {
    public:
        Subscription()=default;
        ~Subscription(){Detach();}
        Subscription(const Subscription&)=delete;
        Subscription& operator=(const Subscription&)=delete;
        void Attach(HWND window);
        void Detach();
        bool Accepts(WPARAM generation) const;
    private:
        HWND m_window{}; ULONG_PTR m_generation{};
    };
    void NotifySubscribers();
}
