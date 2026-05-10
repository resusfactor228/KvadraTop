#pragma once
#include "metrics/MetricsSnapshot.hpp"
#include "metrics/CpuMonitor.hpp"
#include "metrics/MemoryMonitor.hpp"
#include "metrics/NetworkMonitor.hpp"
#include "metrics/DiskMonitor.hpp"
#include "metrics/ProcessMonitor.hpp"
#include "metrics/SystemInfo.hpp"

/// Aggregates all monitor sub-systems and produces a complete metrics::Snapshot.
class MetricsCollector {
public:
    MetricsCollector();

    /// Collects a full snapshot of all system metrics.
    metrics::Snapshot collect();

private:
    metrics::CpuMonitor          cpu_;
    metrics::MemoryMonitor       memory_;
    metrics::NetworkMonitor      network_;
    metrics::DiskMonitor         disk_;
    metrics::ProcessMonitor      processes_;
    metrics::SystemInfoCollector sysinfo_;
};
