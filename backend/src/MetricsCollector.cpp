#include "MetricsCollector.hpp"
#include <chrono>

MetricsCollector::MetricsCollector() = default;

metrics::Snapshot MetricsCollector::collect() {
    metrics::Snapshot snap;

    snap.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Memory must be sampled before processes so total_kb is available.
    snap.memory    = memory_.sample();
    snap.cpu       = cpu_.sample();
    snap.network   = network_.sample();
    snap.disk      = disk_.sample();
    snap.processes = processes_.sample(snap.memory.total_kb);
    snap.system    = sysinfo_.collect();

    return snap;
}
