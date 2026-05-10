#include "CpuMonitor.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace metrics {

// ---------------------------------------------------------------------------
// RawStat helpers
// ---------------------------------------------------------------------------

uint64_t CpuMonitor::RawStat::total() const {
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

uint64_t CpuMonitor::RawStat::active() const {
    return user + nice + system + irq + softirq + steal;
}

// ---------------------------------------------------------------------------
// Construction / sampling
// ---------------------------------------------------------------------------

CpuMonitor::CpuMonitor() {
    prev_stats_ = read_proc_stat();
}

CpuSnapshot CpuMonitor::sample() {
    auto curr = read_proc_stat();

    CpuSnapshot snap;
    snap.load_avg_1 = 0.0;
    snap.load_avg_5 = 0.0;
    snap.load_avg_15 = 0.0;

    if (!prev_stats_.empty() && !curr.empty()) {
        // Index 0 is the aggregate "cpu" line.
        snap.total_usage_percent = compute_usage(prev_stats_[0], curr[0]);

        snap.cores.reserve(curr.size() > 0 ? curr.size() - 1 : 0);
        for (size_t i = 1; i < curr.size(); ++i) {
            CpuCore core;
            core.index = static_cast<int>(i - 1);
            if (i < prev_stats_.size()) {
                core.usage_percent = compute_usage(prev_stats_[i], curr[i]);
            } else {
                core.usage_percent = 0.0;
            }
            snap.cores.push_back(core);
        }
    } else {
        snap.total_usage_percent = 0.0;
    }

    prev_stats_ = std::move(curr);

    // Load averages from /proc/loadavg
    std::ifstream la("/proc/loadavg");
    if (la) {
        la >> snap.load_avg_1 >> snap.load_avg_5 >> snap.load_avg_15;
    }

    return snap;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::vector<CpuMonitor::RawStat> CpuMonitor::read_proc_stat() {
    std::ifstream f("/proc/stat");
    if (!f) {
        throw std::runtime_error("cannot open /proc/stat");
    }

    std::vector<RawStat> stats;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("cpu", 0) != 0) {
            break;  // cpu lines are always at the top; stop at first non-cpu line
        }
        std::istringstream ss(line);
        std::string label;
        RawStat s{};
        ss >> label
           >> s.user >> s.nice >> s.system >> s.idle
           >> s.iowait >> s.irq >> s.softirq >> s.steal;
        stats.push_back(s);
    }
    return stats;
}

double CpuMonitor::compute_usage(const RawStat& prev, const RawStat& curr) {
    const uint64_t total_delta  = curr.total()  - prev.total();
    const uint64_t active_delta = curr.active() - prev.active();
    if (total_delta == 0) {
        return 0.0;
    }
    return std::clamp(
        static_cast<double>(active_delta) / static_cast<double>(total_delta) * 100.0,
        0.0,
        100.0
    );
}

} // namespace metrics
