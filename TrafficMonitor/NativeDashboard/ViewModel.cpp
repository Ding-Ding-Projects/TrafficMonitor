#include "../stdafx.h"
#include "ViewModel.h"
#include <cwctype>

namespace NativeDashboard
{
    namespace
    {
        std::wstring Fold(std::wstring value)
        {
            for (auto& ch : value) ch = static_cast<wchar_t>(std::towlower(ch));
            return value;
        }
    }
    std::wstring ViewModel::Identity(const NativeMonitoring::AdapterSnapshot& row) const
    {
        if (row.hasPersistentIdentity) return row.persistentId;
        return L"temporary:" + std::to_wstring(m_displayed->sequence) + L":" + std::to_wstring(row.interfaceLuid);
    }
    void ViewModel::Receive(std::shared_ptr<const NativeMonitoring::Snapshot> snapshot)
    {
        m_latest = std::move(snapshot);
        if (!m_paused) { m_displayed = m_latest; Rebuild(); }
    }
    void ViewModel::SetPaused(bool paused)
    {
        m_paused = paused;
        if (!paused) { m_displayed = m_latest; Rebuild(); }
    }
    void ViewModel::Reset()
    {
        m_latest.reset(); m_displayed.reset(); m_rows.clear(); m_paused = false;
    }
    void ViewModel::Filter(const std::wstring& query) { search.options.query=query; Rebuild(); }
    std::vector<std::wstring> ViewModel::SearchSubjects() const
    {
        std::vector<std::wstring> result;if(m_displayed)for(const auto& row:m_displayed->adapters)result.push_back(row.alias+L" "+row.description);return result;
    }
    bool ViewModel::PollFilter(){if(!search.Poll())return false;Rebuild();return true;}
    void ViewModel::Rebuild()
    {
        m_rows.clear();
        if (!m_displayed) return;
        search.Request(SearchSubjects());
        for (std::size_t i = 0; i < m_displayed->adapters.size(); ++i)
        {
            const auto& row = m_displayed->adapters[i];
            if (search.Visible(i))
                m_rows.push_back(i);
        }
        if (!m_selectedOnce && !m_rows.empty())
        {
            m_selected = Identity(m_displayed->adapters[m_rows.front()]);
            m_selectedOnce = true;
        }
    }
    const NativeMonitoring::AdapterSnapshot* ViewModel::Row(std::size_t index) const
    {
        return m_displayed && index < m_rows.size() ? &m_displayed->adapters[m_rows[index]] : nullptr;
    }
    const NativeMonitoring::AdapterSnapshot* ViewModel::Selected() const
    {
        if (m_displayed)
            for (const auto& row : m_displayed->adapters)
                if (Identity(row) == m_selected) return &row;
        return nullptr;
    }
    void ViewModel::Select(std::size_t index)
    {
        if (const auto* row = Row(index)) { m_selected = Identity(*row); m_selectedOnce = true; }
    }
    int ViewModel::SelectedRow() const
    {
        for (std::size_t i = 0; i < m_rows.size(); ++i)
            if (Identity(*Row(i)) == m_selected) return static_cast<int>(i);
        return -1;
    }
}
