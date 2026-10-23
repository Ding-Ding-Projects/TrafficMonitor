#include "../stdafx.h"
#include "Subscription.h"
#include <map>
namespace NativePreferences
{
    namespace { std::map<ULONG_PTR,HWND> windows; ULONG_PTR sequence{}; }
    void Subscription::Attach(HWND window){Detach();if(IsWindow(window)){m_window=window;m_generation=++sequence;windows.emplace(m_generation,window);}}
    void Subscription::Detach(){if(m_generation)windows.erase(m_generation);m_generation=0;m_window=nullptr;}
    bool Subscription::Accepts(WPARAM generation)const{return m_generation && generation==m_generation && IsWindow(m_window);}
    void NotifySubscribers(){for(const auto& entry:windows)PostMessageW(entry.second,ChangedMessage,entry.first,0);}
}
