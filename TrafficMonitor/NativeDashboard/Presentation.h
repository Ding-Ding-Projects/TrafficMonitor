#pragma once
#include "../NativeMaterial/Theme.h"
#include "../NativeMonitoringSnapshot.h"
#include <string>

namespace NativeDashboard
{
    // Owns a copy of the displayed observation; painting never borrows worker memory.
    class DataView final : public CWnd
    {
    public:
        BOOL Create(CWnd*,UINT);
        void SetSnapshot(const NativeMonitoring::AdapterSnapshot*,bool);
        void SetAppearance(NativeMaterial::Appearance);
    protected:
        LRESULT WindowProc(UINT,WPARAM,LPARAM) override;
    private:
        HRESULT DrawDC(HDC);
        NativeMaterial::Appearance m_appearance{NativeMaterial::Appearance::Light};
        std::optional<NativeMonitoring::AdapterSnapshot> m_row;
        bool m_chinese{};
    };
    std::wstring Rate(const std::optional<double>& value, bool chinese);
    std::wstring RateStateText(NativeMonitoring::RateState state, bool chinese);
    std::wstring TimeText(std::uint64_t fileTime);
    std::wstring Details(const NativeMonitoring::AdapterSnapshot* row, bool selected, bool chinese);
}
