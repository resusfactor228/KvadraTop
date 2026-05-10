#pragma once
#include "MetricsSnapshot.hpp"
#include <map>
#include <chrono>
#include <string>

namespace metrics {

/// Reads /proc/diskstats periodically and computes per-device I/O rates.
class DiskMonitor {
public:
    DiskMonitor();

    /// Reads /proc/diskstats and computes rates since last call.
    std::vector<DiskDevice> sample();

private:
    /// Raw sector counts and I/O ticks for one device.
    struct RawDisk {
        uint64_t read_sectors;
        uint64_t write_sectors;
        uint64_t io_ticks;   ///< milliseconds spent doing I/O
    };

    std::map<std::string, RawDisk>        prev_;
    std::chrono::steady_clock::time_point prev_time_;

    /// Parse /proc/diskstats; returns only "real" block devices (no partitions).
    static std::map<std::string, RawDisk> read_proc_diskstats();

    /// Return true if the device name looks like a whole disk (not a partition).
    static bool is_whole_disk(const std::string& name);
};

} // namespace metrics
