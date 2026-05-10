#include "MemoryMonitor.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <stdexcept>

namespace metrics {

MemorySnapshot MemoryMonitor::sample() {
    std::ifstream f("/proc/meminfo");
    if (!f) {
        throw std::runtime_error("cannot open /proc/meminfo");
    }

    // Parse every "Key: value kB" line into a map for easy lookup.
    std::map<std::string, uint64_t> fields;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string key;
        uint64_t    value = 0;
        std::string unit;   // typically "kB" — we ignore it (values are already in kB)
        ss >> key >> value >> unit;
        // key has a trailing colon, strip it
        if (!key.empty() && key.back() == ':') {
            key.pop_back();
        }
        fields[key] = value;
    }

    auto get = [&](const std::string& k) -> uint64_t {
        auto it = fields.find(k);
        return (it != fields.end()) ? it->second : 0ULL;
    };

    MemorySnapshot snap{};
    snap.total_kb     = get("MemTotal");
    snap.free_kb      = get("MemFree");
    snap.available_kb = get("MemAvailable");
    snap.buffers_kb   = get("Buffers");
    // "Cached" line in /proc/meminfo excludes SwapCached.
    snap.cached_kb    = get("Cached");

    snap.swap_total_kb = get("SwapTotal");
    const uint64_t swap_free = get("SwapFree");
    snap.swap_used_kb  = snap.swap_total_kb > swap_free
                             ? snap.swap_total_kb - swap_free
                             : 0ULL;

    // used = total - available  (this is the "application + buffers + cache" view)
    snap.used_kb = snap.total_kb > snap.available_kb
                       ? snap.total_kb - snap.available_kb
                       : 0ULL;

    snap.usage_percent = snap.total_kb > 0
        ? static_cast<double>(snap.used_kb) / static_cast<double>(snap.total_kb) * 100.0
        : 0.0;

    return snap;
}

} // namespace metrics
