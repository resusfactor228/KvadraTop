#include "Serializer.hpp"

std::string serialize_snapshot(const metrics::Snapshot& snap) {
    nlohmann::json j;

    j["timestamp_ms"] = snap.timestamp_ms;

    // --- CPU ----------------------------------------------------------------
    auto& cpu = j["cpu"];
    cpu["total"]    = snap.cpu.total_usage_percent;
    cpu["load_avg"] = {snap.cpu.load_avg_1, snap.cpu.load_avg_5, snap.cpu.load_avg_15};
    auto& cores = cpu["cores"];
    cores = nlohmann::json::array();
    for (const auto& c : snap.cpu.cores) {
        cores.push_back({{"index", c.index}, {"usage", c.usage_percent}});
    }

    // --- Memory -------------------------------------------------------------
    auto& mem = j["memory"];
    mem["total_kb"]      = snap.memory.total_kb;
    mem["used_kb"]       = snap.memory.used_kb;
    mem["free_kb"]       = snap.memory.free_kb;
    mem["available_kb"]  = snap.memory.available_kb;
    mem["buffers_kb"]    = snap.memory.buffers_kb;
    mem["cached_kb"]     = snap.memory.cached_kb;
    mem["swap_total_kb"] = snap.memory.swap_total_kb;
    mem["swap_used_kb"]  = snap.memory.swap_used_kb;
    mem["usage_percent"] = snap.memory.usage_percent;

    // --- Network ------------------------------------------------------------
    auto& net = j["network"];
    net = nlohmann::json::array();
    for (const auto& iface : snap.network) {
        net.push_back({
            {"name",        iface.name},
            {"rx_bytes",    iface.rx_bytes_total},
            {"tx_bytes",    iface.tx_bytes_total},
            {"rx_rate_bps", iface.rx_rate_bps},
            {"tx_rate_bps", iface.tx_rate_bps}
        });
    }

    // --- Disk ---------------------------------------------------------------
    auto& disk = j["disk"];
    disk = nlohmann::json::array();
    for (const auto& dev : snap.disk) {
        disk.push_back({
            {"name",           dev.name},
            {"read_bytes",     dev.read_bytes_total},
            {"write_bytes",    dev.write_bytes_total},
            {"read_rate_bps",  dev.read_rate_bps},
            {"write_rate_bps", dev.write_rate_bps},
            {"util_percent",   dev.util_percent}
        });
    }

    // --- Processes ----------------------------------------------------------
    auto& procs = j["processes"];
    procs = nlohmann::json::array();
    for (const auto& p : snap.processes) {
        procs.push_back({
            {"pid",         p.pid},
            {"name",        p.name},
            {"user",        p.user},
            {"state",       std::string(1, p.state)},
            {"cpu_percent", p.cpu_percent},
            {"mem_percent", p.mem_percent},
            {"mem_rss_kb",  p.mem_rss_kb},
            {"threads",     p.threads},
            {"uptime_s",    p.uptime_seconds}
        });
    }

    // --- System -------------------------------------------------------------
    auto& sys = j["system"];
    sys["hostname"]       = snap.system.hostname;
    sys["kernel"]         = snap.system.kernel;
    sys["os"]             = snap.system.os_name;
    sys["uptime_seconds"] = snap.system.uptime_seconds;
    sys["cpu_count"]      = snap.system.cpu_count;

    return j.dump();
}
