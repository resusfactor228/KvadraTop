#include "DiskMonitor.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <algorithm>

namespace metrics {

static constexpr uint64_t SECTOR_BYTES = 512;

DiskMonitor::DiskMonitor()
    : prev_time_(std::chrono::steady_clock::now())
{
    prev_ = read_proc_diskstats();
}

std::vector<DiskDevice> DiskMonitor::sample() {
    auto now  = std::chrono::steady_clock::now();
    auto curr = read_proc_diskstats();

    const double elapsed_s  = std::chrono::duration<double>(now - prev_time_).count();
    const double elapsed_ms = elapsed_s * 1000.0;

    std::vector<DiskDevice> result;
    result.reserve(curr.size());

    for (auto& [name, raw] : curr) {
        DiskDevice dev;
        dev.name              = name;
        dev.read_bytes_total  = raw.read_sectors  * SECTOR_BYTES;
        dev.write_bytes_total = raw.write_sectors * SECTOR_BYTES;

        if (elapsed_s > 0.0 && prev_.count(name)) {
            const auto& p = prev_.at(name);

            const uint64_t dr = raw.read_sectors  >= p.read_sectors
                                ? raw.read_sectors  - p.read_sectors  : 0;
            const uint64_t dw = raw.write_sectors >= p.write_sectors
                                ? raw.write_sectors - p.write_sectors : 0;
            const uint64_t dt = raw.io_ticks      >= p.io_ticks
                                ? raw.io_ticks      - p.io_ticks      : 0;

            dev.read_rate_bps  = static_cast<double>(dr) * SECTOR_BYTES / elapsed_s;
            dev.write_rate_bps = static_cast<double>(dw) * SECTOR_BYTES / elapsed_s;
            // Utilisation: fraction of time the device was busy (clamped to 100 %).
            dev.util_percent   = elapsed_ms > 0.0
                                 ? std::min(static_cast<double>(dt) / elapsed_ms * 100.0, 100.0)
                                 : 0.0;
        } else {
            dev.read_rate_bps  = 0.0;
            dev.write_rate_bps = 0.0;
            dev.util_percent   = 0.0;
        }

        result.push_back(std::move(dev));
    }

    prev_      = std::move(curr);
    prev_time_ = now;
    return result;
}

std::map<std::string, DiskMonitor::RawDisk> DiskMonitor::read_proc_diskstats() {
    std::ifstream f("/proc/diskstats");
    if (!f) {
        throw std::runtime_error("cannot open /proc/diskstats");
    }

    std::map<std::string, RawDisk> result;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);

        unsigned int major_num = 0, minor_num = 0;
        std::string  name;
        uint64_t reads_completed = 0, reads_merged = 0;
        uint64_t sectors_read    = 0;
        uint64_t read_ms         = 0;
        uint64_t writes_completed = 0, writes_merged = 0;
        uint64_t sectors_written  = 0;
        uint64_t write_ms         = 0;
        uint64_t io_in_progress   = 0;
        uint64_t io_ticks         = 0;

        ss >> major_num >> minor_num >> name
           >> reads_completed >> reads_merged >> sectors_read    >> read_ms
           >> writes_completed >> writes_merged >> sectors_written >> write_ms
           >> io_in_progress   >> io_ticks;

        if (!is_whole_disk(name)) continue;

        RawDisk raw{};
        raw.read_sectors  = sectors_read;
        raw.write_sectors = sectors_written;
        raw.io_ticks      = io_ticks;
        result[name]      = raw;
    }

    return result;
}

bool DiskMonitor::is_whole_disk(const std::string& name) {
    // Accept: sda, sdb, hda, nvme0n1, vda, xvda, mmcblk0
    // Reject: sda1, sda2, nvme0n1p1, mmcblk0p1, loop*, dm-*, sr*, fd*
    static const std::regex whole_disk(
        R"(^(sd[a-z]+|hd[a-z]+|vd[a-z]+|xvd[a-z]+|nvme\d+n\d+|mmcblk\d+)$)"
    );
    return std::regex_match(name, whole_disk);
}

} // namespace metrics
