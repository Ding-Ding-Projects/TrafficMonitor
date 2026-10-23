#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include "NativeMonitoringSnapshot.h"
#include <chrono>
#include <cstring>
#include <cwchar>
#include <utility>
#include <set>
#include <stdexcept>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace NativeMonitoring
{
    namespace
    {
        std::uint64_t UtcNow()
        {
            FILETIME time{};
            GetSystemTimeAsFileTime(&time);
            return (static_cast<std::uint64_t>(time.dwHighDateTime) << 32) | time.dwLowDateTime;
        }
        std::uint64_t MonotonicNow()
        {
            return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        }
        std::wstring GuidKey(const GUID& guid)
        {
            const GUID empty{};
            if (std::memcmp(&guid, &empty, sizeof(GUID)) == 0)
                return {};
            wchar_t text[39]{};
            swprintf_s(text, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
                guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
            return text;
        }
        struct TableDeleter { void operator()(MIB_IF_TABLE2* table) const { if (table) FreeMibTable(table); } };
        struct AddressResult
        {
            ULONG error{ ERROR_SUCCESS };
            std::map<std::uint64_t, std::vector<std::wstring>> byLuid;
        };
        AddressResult ReadAddresses()
        {
            AddressResult result;
            ULONG bytes = 16384;
            constexpr ULONG maximumBytes = 4 * 1024 * 1024;
            std::vector<unsigned char> buffer;
            for (int attempt = 0; attempt < 3; ++attempt)
            {
                if (bytes == 0 || bytes > maximumBytes)
                {
                    result.error = ERROR_BUFFER_OVERFLOW;
                    return result;
                }
                buffer.resize(bytes);
                auto* first = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
                result.error = GetAdaptersAddresses(AF_UNSPEC,
                    GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                    nullptr, first, &bytes);
                if (result.error == ERROR_BUFFER_OVERFLOW)
                    continue;
                if (result.error == ERROR_NO_DATA)
                {
                    result.error = ERROR_SUCCESS;
                    return result;
                }
                if (result.error != ERROR_SUCCESS)
                    return result;
                for (auto* adapter = first; adapter != nullptr; adapter = adapter->Next)
                {
                    auto& addresses = result.byLuid[adapter->Luid.Value];
                    for (auto* address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next)
                    {
                        if (!address->Address.lpSockaddr)
                            continue;
                        const auto* socket = address->Address.lpSockaddr;
                        const void* data{};
                        if (socket->sa_family == AF_INET && address->Address.iSockaddrLength >= sizeof(sockaddr_in))
                            data = &reinterpret_cast<const sockaddr_in*>(socket)->sin_addr;
                        else if (socket->sa_family == AF_INET6 && address->Address.iSockaddrLength >= sizeof(sockaddr_in6))
                            data = &reinterpret_cast<const sockaddr_in6*>(socket)->sin6_addr;
                        else
                            continue;
                        wchar_t text[INET6_ADDRSTRLEN]{};
                        if (!InetNtopW(socket->sa_family, data, text, INET6_ADDRSTRLEN))
                        {
                            result.error = static_cast<ULONG>(WSAGetLastError());
                            return result;
                        }
                        // Refuse partial address inventories instead of silently truncating.
                        if (addresses.size() >= 256)
                        {
                            result.error = ERROR_BUFFER_OVERFLOW;
                            return result;
                        }
                        std::wstring formatted = text;
                        if (socket->sa_family == AF_INET6)
                        {
                            const auto scope = reinterpret_cast<const sockaddr_in6*>(socket)->sin6_scope_id;
                            if (scope != 0) formatted += L"%" + std::to_wstring(scope);
                        }
                        addresses.push_back(std::move(formatted));
                    }
                }
                return result;
            }
            return result;
        }
    }

    NetworkSampler::NetworkSampler(SamplerLimits limits) : m_limits(limits)
    {
        if (limits.historyPointsPerAdapter == 0 || limits.historyPointsPerAdapter > 3600
            || limits.maximumAdapters == 0 || limits.maximumAdapters > 4096)
            throw std::invalid_argument("Invalid observational network sampler limits");
    }

    std::shared_ptr<const Snapshot> NetworkSampler::Latest() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_latest;
    }

    const AdapterSnapshot* NetworkSampler::Find(const Snapshot& snapshot, const GUID& guid)
    {
        const auto key = GuidKey(guid);
        if (key.empty()) return nullptr;
        for (const auto& row : snapshot.adapters)
            if (row.hasPersistentIdentity && row.persistentId == key) return &row;
        return nullptr;
    }

    std::shared_ptr<const Snapshot> NetworkSampler::Unavailable(std::uint32_t error, std::uint64_t attemptedAt)
    {
        auto snapshot = std::make_shared<Snapshot>();
        if (m_latest) *snapshot = *m_latest;
        snapshot->state = SnapshotState::Unavailable;
        snapshot->error = error;
        snapshot->sequence = ++m_sequence;
        snapshot->attemptedAtUtcFileTime = attemptedAt;
        for (auto& row : snapshot->adapters)
        {
            row.rateState = RateState::Stale;
            row.receivedBytesPerSecond.reset();
            row.sentBytesPerSecond.reset();
            row.intervalSeconds = 0;
        }
        // A gap can hide disconnect/reconnect. Never derive a rate across it.
        for (auto& entry : m_baselines) entry.second.valid = false;
        m_latest = snapshot;
        return snapshot;
    }

    std::shared_ptr<const Snapshot> NetworkSampler::Sample()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto attemptedAt = UtcNow();
        MIB_IF_TABLE2* raw{};
        const ULONG error = GetIfTable2(&raw);
        std::unique_ptr<MIB_IF_TABLE2, TableDeleter> table(raw);
        if (error != ERROR_SUCCESS) return Unavailable(error, attemptedAt);
        if (!table) return Unavailable(ERROR_INVALID_DATA, attemptedAt);
        if (table->NumEntries > m_limits.maximumAdapters)
            return Unavailable(ERROR_BUFFER_OVERFLOW, attemptedAt);
        const auto acquiredAt = UtcNow();
        const auto monotonic = MonotonicNow();
        const auto addresses = ReadAddresses();
        auto snapshot = std::make_shared<Snapshot>();
        snapshot->state = SnapshotState::Available;
        snapshot->sequence = ++m_sequence;
        snapshot->attemptedAtUtcFileTime = attemptedAt;
        snapshot->lastSuccessfulAcquisitionUtcFileTime = acquiredAt;
        std::map<std::wstring, std::uint64_t> discovered;
        std::set<std::uint64_t> luids;
        for (ULONG index = 0; index < table->NumEntries; ++index)
        {
            const auto& native = table->Table[index];
            const auto id = GuidKey(native.InterfaceGuid);
            const auto key = id.empty() ? L"runtime:" + std::to_wstring(native.InterfaceLuid.Value) : id;
            if (!discovered.emplace(key, native.InterfaceLuid.Value).second
                || native.InterfaceLuid.Value == 0 || !luids.insert(native.InterfaceLuid.Value).second)
                return Unavailable(ERROR_INVALID_DATA, attemptedAt);
        }
        if (discovered != m_discovered) ++m_discoveryEpoch;
        snapshot->discoveryEpoch = m_discoveryEpoch;
        for (ULONG index = 0; index < table->NumEntries; ++index)
        {
            const auto& native = table->Table[index];
            AdapterSnapshot row;
            row.interfaceGuid = native.InterfaceGuid;
            row.persistentId = GuidKey(native.InterfaceGuid);
            row.hasPersistentIdentity = !row.persistentId.empty();
            row.interfaceLuid = native.InterfaceLuid.Value;
            row.interfaceIndex = native.InterfaceIndex;
            row.alias = native.Alias;
            row.description = native.Description;
            row.interfaceType = native.Type;
            row.operationalStatus = native.OperStatus;
            row.receivedOctets = native.InOctets;
            row.sentOctets = native.OutOctets;
            row.receiveLinkBitsPerSecond = native.ReceiveLinkSpeed;
            row.transmitLinkBitsPerSecond = native.TransmitLinkSpeed;
            row.acquiredAtUtcFileTime = acquiredAt;
            row.acquiredAtMonotonicMilliseconds = monotonic / 1000000;
            row.addressError = addresses.error;
            if (addresses.error == ERROR_SUCCESS)
            {
                const auto found = addresses.byLuid.find(row.interfaceLuid);
                if (found != addresses.byLuid.end()) row.addresses = found->second;
            }
            const auto key = row.hasPersistentIdentity ? row.persistentId : L"runtime:" + std::to_wstring(row.interfaceLuid);
            auto& previous = m_baselines[key];
            const bool connected = native.OperStatus == IfOperStatusUp;
            const bool reset = !previous.valid || previous.luid != row.interfaceLuid
                || previous.connected != connected || monotonic <= previous.monotonicNanoseconds
                || row.receivedOctets < previous.received || row.sentOctets < previous.sent;
            if (reset) previous.sampleEpoch = ++m_sampleEpoch;
            row.sampleEpoch = previous.sampleEpoch;
            if (!row.hasPersistentIdentity) row.rateState = RateState::IdentityUnavailable;
            else if (!connected) row.rateState = RateState::Disconnected;
            else if (reset) row.rateState = RateState::CollectingFirstSample;
            else
            {
                row.rateState = RateState::Available;
                row.intervalSeconds = static_cast<double>(monotonic - previous.monotonicNanoseconds) / 1000000000.0;
                row.receivedBytesPerSecond = static_cast<double>(row.receivedOctets - previous.received) / row.intervalSeconds;
                row.sentBytesPerSecond = static_cast<double>(row.sentOctets - previous.sent) / row.intervalSeconds;
            }
            previous.luid = row.interfaceLuid;
            previous.received = row.receivedOctets;
            previous.sent = row.sentOctets;
            previous.monotonicNanoseconds = monotonic;
            previous.connected = connected;
            previous.valid = true;
            previous.history.push_back({ row.acquiredAtMonotonicMilliseconds, row.sampleEpoch, row.rateState,
                row.receivedBytesPerSecond, row.sentBytesPerSecond });
            while (previous.history.size() > m_limits.historyPointsPerAdapter) previous.history.pop_front();
            row.history.assign(previous.history.begin(), previous.history.end());
            snapshot->adapters.push_back(std::move(row));
        }
        // Removed identities disappear from live rows and lose their baselines.
        // A later reappearance necessarily starts a new epoch and first sample.
        for (auto it = m_baselines.begin(); it != m_baselines.end();)
            if (discovered.find(it->first) == discovered.end()) it = m_baselines.erase(it);
            else ++it;
        m_discovered = std::move(discovered);
        m_latest = snapshot;
        return snapshot;
    }
}
