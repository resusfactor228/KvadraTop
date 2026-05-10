#pragma once
#include "MetricsSnapshot.hpp"

namespace metrics {

/// Reads /proc/meminfo and returns a memory snapshot.
class MemoryMonitor {
public:
    /// Reads /proc/meminfo and computes usage statistics.
    MemorySnapshot sample();
};

} // namespace metrics
