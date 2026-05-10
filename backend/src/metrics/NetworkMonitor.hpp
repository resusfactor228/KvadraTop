#pragma once
#include "MetricsSnapshot.hpp"
#include <map>
#include <chrono>
#include <string>

namespace metrics {

/// Reads /proc/net/dev periodically and computes per-interface byte rates.
class NetworkMonitor {
public:
    NetworkMonitor();

    /// Reads /proc/net/dev and computes rates since last call.
    std::vector<NetworkInterface> sample();

private:
    /// Raw byte counters for one interface.
    struct RawNet {
        uint64_t rx_bytes;
        uint64_t tx_bytes;
    };

    std::map<std::string, RawNet>         prev_;
    std::chrono::steady_clock::time_point prev_time_;

    /// Parse /proc/net/dev and return a per-interface map of byte totals.
    static std::map<std::string, RawNet> read_proc_net_dev();
};

} // namespace metrics
