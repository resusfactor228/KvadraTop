#pragma once
#include "MetricsSnapshot.hpp"

namespace metrics {

/// Collects static / slowly-changing system information.
class SystemInfoCollector {
public:
    /// Reads hostname, kernel version, OS name, uptime and CPU count.
    SystemInfo collect();
};

} // namespace metrics
