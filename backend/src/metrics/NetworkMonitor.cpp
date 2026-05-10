#include "NetworkMonitor.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>

namespace metrics {

NetworkMonitor::NetworkMonitor()
    : prev_time_(std::chrono::steady_clock::now())
{
    prev_ = read_proc_net_dev();
}

std::vector<NetworkInterface> NetworkMonitor::sample() {
    auto now  = std::chrono::steady_clock::now();
    auto curr = read_proc_net_dev();

    const double elapsed = std::chrono::duration<double>(now - prev_time_).count();

    std::vector<NetworkInterface> result;
    result.reserve(curr.size());

    for (auto& [name, raw] : curr) {
        NetworkInterface iface;
        iface.name           = name;
        iface.rx_bytes_total = raw.rx_bytes;
        iface.tx_bytes_total = raw.tx_bytes;

        if (elapsed > 0.0 && prev_.count(name)) {
            const auto& p       = prev_.at(name);
            const uint64_t drx  = raw.rx_bytes >= p.rx_bytes ? raw.rx_bytes - p.rx_bytes : 0;
            const uint64_t dtx  = raw.tx_bytes >= p.tx_bytes ? raw.tx_bytes - p.tx_bytes : 0;
            iface.rx_rate_bps   = static_cast<double>(drx) / elapsed;
            iface.tx_rate_bps   = static_cast<double>(dtx) / elapsed;
        } else {
            iface.rx_rate_bps = 0.0;
            iface.tx_rate_bps = 0.0;
        }

        result.push_back(std::move(iface));
    }

    prev_      = std::move(curr);
    prev_time_ = now;
    return result;
}

std::map<std::string, NetworkMonitor::RawNet> NetworkMonitor::read_proc_net_dev() {
    std::ifstream f("/proc/net/dev");
    if (!f) {
        throw std::runtime_error("cannot open /proc/net/dev");
    }

    std::map<std::string, RawNet> result;
    std::string line;

    // Skip the two header lines.
    std::getline(f, line);
    std::getline(f, line);

    while (std::getline(f, line)) {
        if (line.empty()) continue;

        // Find the colon that separates interface name from counters.
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string name = line.substr(0, colon);
        // Trim leading whitespace from name.
        const auto first = name.find_first_not_of(" \t");
        if (first != std::string::npos) name = name.substr(first);

        // Skip loopback.
        if (name == "lo") continue;

        std::istringstream ss(line.substr(colon + 1));
        RawNet raw{};
        uint64_t dummy = 0;
        // Fields: rx_bytes rx_packets rx_errs rx_drop rx_fifo rx_frame rx_compressed rx_multicast
        //         tx_bytes tx_packets ...
        ss >> raw.rx_bytes
           >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy
           >> raw.tx_bytes;

        result[name] = raw;
    }

    return result;
}

} // namespace metrics
