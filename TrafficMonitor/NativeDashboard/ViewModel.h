#pragma once
#include "../NativeMonitoringSnapshot.h"

namespace NativeDashboard
{
    class ViewModel final
    {
    public:
        void Receive(std::shared_ptr<const NativeMonitoring::Snapshot> snapshot);
        void SetPaused(bool paused);
        void Reset();
        void Filter(const std::wstring& query);
        void Select(std::size_t visibleRow);
        const NativeMonitoring::AdapterSnapshot* Selected() const;
        const NativeMonitoring::AdapterSnapshot* Row(std::size_t visibleRow) const;
        const std::shared_ptr<const NativeMonitoring::Snapshot>& Displayed() const { return m_displayed; }
        const std::vector<std::size_t>& Rows() const { return m_rows; }
        const std::wstring& SelectedIdentity() const { return m_selected; }
        bool Paused() const { return m_paused; }
        int SelectedRow() const;
    private:
        std::wstring Identity(const NativeMonitoring::AdapterSnapshot& adapter) const;
        void Rebuild();
        std::shared_ptr<const NativeMonitoring::Snapshot> m_latest, m_displayed;
        std::vector<std::size_t> m_rows;
        std::wstring m_query, m_selected;
        bool m_paused{}, m_selectedOnce{};
    };
}
