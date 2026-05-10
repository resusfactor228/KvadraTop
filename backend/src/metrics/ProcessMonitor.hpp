#pragma once
#include "MetricsSnapshot.hpp"
#include <map>
#include <string>

namespace metrics {

/// Iterates /proc/<pid>/stat to build a top-20 process list ranked by CPU.
class ProcessMonitor {
public:
    ProcessMonitor();

    /// Returns up to 20 processes sorted by CPU usage (descending).
    /// @param total_memory_kb  Total physical memory in kB (from MemoryMonitor).
    std::vector<ProcessEntry> sample(uint64_t total_memory_kb);

private:
    /// State retained between samples for delta-CPU computation.
    struct RawProc {
        uint64_t cpu_ticks;  ///< utime + stime at last sample
    };

    std::map<int, RawProc> prev_;
    long                   hertz_;             ///< CLK_TCK (jiffies per second)
    uint64_t               prev_total_ticks_;  ///< aggregate CPU jiffies at last sample

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /// Parsed fields from /proc/<pid>/stat.
    struct ProcStat {
        int         pid      = 0;
        std::string comm;       ///< executable name, already stripped of parens
        char        state    = '?';
        uint64_t    utime    = 0;   ///< jiffies
        uint64_t    stime    = 0;
        long        rss      = 0;   ///< pages
        int         threads  = 0;
        uint64_t    starttime = 0;  ///< jiffies since boot
    };

    /// Read and parse /proc/<pid>/stat.  Returns false on failure (e.g. pid gone).
    static bool read_proc_stat(int pid, ProcStat& out);

    /// Read the real UID from /proc/<pid>/status.  Returns -1 on failure.
    static int  read_uid(int pid);

    /// Translate numeric UID to username (thread-safe via getpwuid_r).
    static std::string uid_to_username(int uid);

    /// Read system uptime in seconds from /proc/uptime.
    static double read_uptime();

    /// Sum of all CPU jiffies from /proc/stat aggregate line.
    static uint64_t read_total_cpu_ticks();
};

} // namespace metrics
