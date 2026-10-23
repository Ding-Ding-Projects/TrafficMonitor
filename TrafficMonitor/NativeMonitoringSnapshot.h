#pragma once

#include <guiddef.h>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Observational network sampling only. This module never reads or writes the
// application's selected aggregate rates, traffic counters or daily accounting.
namespace NativeMonitoring
{
    enum class SnapshotState { Available, Unavailable };
    enum class RateState { CollectingFirstSample, Available, Disconnected, IdentityUnavailable, Stale };

    struct RatePoint
    {
        std::uint64_t monotonicMilliseconds{};
        std::uint64_t sampleEpoch{};
        RateState state{ RateState::CollectingFirstSample };
        std::optional<double> receivedBytesPerSecond;
        std::optional<double> sentBytesPerSecond;
    };

    struct AdapterSnapshot
    {
        GUID interfaceGuid{};
        bool hasPersistentIdentity{};
        std::wstring persistentId; // GUID string; empty means it must not be persisted.
        std::uint64_t interfaceLuid{};
        std::uint32_t interfaceIndex{}; // Current-table join only, never a saved selection.
        std::wstring alias;
        std::wstring description;
        std::uint32_t interfaceType{};
        std::uint32_t operationalStatus{};
        std::uint64_t receivedOctets{};
        std::uint64_t sentOctets{};
        std::uint64_t receiveLinkBitsPerSecond{};
        std::uint64_t transmitLinkBitsPerSecond{};
        std::uint64_t acquiredAtUtcFileTime{};
        std::uint64_t acquiredAtMonotonicMilliseconds{};
        std::uint64_t sampleEpoch{};
        RateState rateState{ RateState::CollectingFirstSample };
        std::optional<double> receivedBytesPerSecond;
        std::optional<double> sentBytesPerSecond;
        double intervalSeconds{};
        std::vector<std::wstring> addresses;
        std::uint32_t addressError{}; // Win32 result; a failure is not an empty successful list.
        std::vector<RatePoint> history; // Bounded display samples, not accounting history.
    };

    struct Snapshot
    {
        SnapshotState state{ SnapshotState::Unavailable };
        std::uint32_t error{}; // GetIfTable2, allocation-limit or identity-validation result.
        std::uint64_t sequence{};
        std::uint64_t discoveryEpoch{}; // Changes only when the GUID/LUID identity set changes.
        std::uint64_t attemptedAtUtcFileTime{};
        std::uint64_t lastSuccessfulAcquisitionUtcFileTime{};
        std::vector<AdapterSnapshot> adapters;
    };

    struct SamplerLimits
    {
        std::size_t historyPointsPerAdapter{ 120 }; // Valid range: 1..3600.
        std::size_t maximumAdapters{ 1024 }; // Valid range: 1..4096; overflow fails explicitly.
    };

    // Call Sample on an owned worker at the desired cadence. Calls are serialized;
    // no thread, timer, HWND or UI dispatch is created by this class. Keep only the
    // latest shared_ptr when coalescing UI notifications. A display pause freezes
    // that pointer, not collection. Snapshots remain valid after later samples.
    // Invalid limits throw std::invalid_argument. Sampling API failures return an
    // Unavailable snapshot, preserving last-acquired rows as Stale with no rates.
    class NetworkSampler final
    {
    public:
        explicit NetworkSampler(SamplerLimits limits = {});
        std::shared_ptr<const Snapshot> Sample();
        // Returns null before the first Sample call.
        std::shared_ptr<const Snapshot> Latest() const;
        // GUID lookup is independent of table order. A missing result means an
        // open details view must show "Adapter no longer present", not another row.
        static const AdapterSnapshot* Find(const Snapshot& snapshot, const GUID& guid);

    private:
        struct Baseline
        {
            std::uint64_t luid{};
            std::uint64_t received{};
            std::uint64_t sent{};
            std::uint64_t monotonicNanoseconds{};
            std::uint64_t sampleEpoch{};
            bool connected{};
            bool valid{};
            std::deque<RatePoint> history;
        };
        std::shared_ptr<const Snapshot> Unavailable(std::uint32_t error, std::uint64_t attemptedAt);
        SamplerLimits m_limits;
        mutable std::mutex m_mutex;
        std::map<std::wstring, Baseline> m_baselines;
        std::map<std::wstring, std::uint64_t> m_discovered;
        std::shared_ptr<const Snapshot> m_latest;
        std::uint64_t m_sequence{};
        std::uint64_t m_discoveryEpoch{};
        std::uint64_t m_sampleEpoch{};
    };
}
