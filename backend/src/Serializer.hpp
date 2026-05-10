#pragma once
#include "metrics/MetricsSnapshot.hpp"
#include <nlohmann/json.hpp>
#include <string>

/// Serializes a metrics::Snapshot to a compact JSON string.
std::string serialize_snapshot(const metrics::Snapshot& snap);
