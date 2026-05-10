#pragma once
#include "MetricsSnapshot.hpp"
#include <vector>

namespace metrics {

/// Reads /proc/stat periodically and computes CPU utilisation deltas.
class CpuMonitor {
public:
    CpuMonitor();

    /// Reads /proc/stat, computes delta since last call.
    CpuSnapshot sample();

private:
    /// Raw jiffie counts for one CPU row from /proc/stat.
    struct RawStat {
        uint64_t user, nice, system, idle, iowait, irq, softirq, steal;

        /// Sum of all jiffie fields.
        uint64_t total() const;

        /// Sum of all non-idle jiffie fields.
        uint64_t active() const;
    };

    std::vector<RawStat> prev_stats_;

    /// Parse all "cpu*" lines from /proc/stat. Index 0 = aggregate.
    static std::vector<RawStat> read_proc_stat();

    /// Compute usage percentage from two consecutive samples.
    static double compute_usage(const RawStat& prev, const RawStat& curr);
};

} // namespace metrics
