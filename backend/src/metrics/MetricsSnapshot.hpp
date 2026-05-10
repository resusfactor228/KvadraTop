#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace metrics {

/// Per-core CPU usage.
struct CpuCore {
    int    index;
    double usage_percent;  ///< 0–100
};

/// Aggregate CPU snapshot including per-core breakdowns and load averages.
struct CpuSnapshot {
    double               total_usage_percent;
    std::vector<CpuCore> cores;
    double               load_avg_1;
    double               load_avg_5;
    double               load_avg_15;
};

/// Physical and virtual memory usage.
struct MemorySnapshot {
    uint64_t total_kb;
    uint64_t used_kb;
    uint64_t free_kb;
    uint64_t available_kb;
    uint64_t buffers_kb;
    uint64_t cached_kb;
    uint64_t swap_total_kb;
    uint64_t swap_used_kb;
    double   usage_percent;  ///< used / total * 100
};

/// Per-interface network statistics and rates.
struct NetworkInterface {
    std::string name;
    uint64_t    rx_bytes_total;
    uint64_t    tx_bytes_total;
    double      rx_rate_bps;   ///< bytes/sec since last sample
    double      tx_rate_bps;
};

/// Per-device disk statistics and rates.
struct DiskDevice {
    std::string name;
    uint64_t    read_bytes_total;
    uint64_t    write_bytes_total;
    double      read_rate_bps;
    double      write_rate_bps;
    double      util_percent;
};

/// A single process entry as reported in the process list.
struct ProcessEntry {
    int         pid;
    std::string name;
    std::string user;
    char        state;
    double      cpu_percent;
    double      mem_percent;
    uint64_t    mem_rss_kb;
    int         threads;
    uint64_t    uptime_seconds;
};

/// Static system information collected once per snapshot.
struct SystemInfo {
    std::string hostname;
    std::string kernel;
    std::string os_name;
    uint64_t    uptime_seconds;
    int         cpu_count;
};

/// Complete system snapshot at a single point in time.
struct Snapshot {
    int64_t                       timestamp_ms;
    CpuSnapshot                   cpu;
    MemorySnapshot                memory;
    std::vector<NetworkInterface> network;
    std::vector<DiskDevice>       disk;
    std::vector<ProcessEntry>     processes;
    SystemInfo                    system;
};

} // namespace metrics
