#include "SystemInfo.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <limits.h>
#include <unistd.h>

namespace metrics {

SystemInfo SystemInfoCollector::collect() {
    SystemInfo info{};

    // --- Hostname -----------------------------------------------------------
    {
        char buf[HOST_NAME_MAX + 1] = {};
        if (gethostname(buf, sizeof(buf)) == 0) {
            info.hostname = buf;
        } else {
            std::ifstream f("/proc/sys/kernel/hostname");
            if (f) std::getline(f, info.hostname);
        }
    }

    // --- Kernel version -----------------------------------------------------
    {
        std::ifstream f("/proc/version");
        if (f) {
            std::string line;
            std::getline(f, line);
            // Trim to the first two words: "Linux version X.Y.Z-..." → keep until '-' or space
            // We just store the full first line — it's descriptive enough.
            info.kernel = line;
            // Extract just "Linux version X.Y.Z" prefix for brevity.
            std::istringstream ss(line);
            std::string tok;
            std::string kver;
            int count = 0;
            while (ss >> tok && count < 3) {
                if (!kver.empty()) kver += ' ';
                kver += tok;
                ++count;
            }
            if (!kver.empty()) info.kernel = kver;
        }
    }

    // --- OS name ------------------------------------------------------------
    {
        std::ifstream f("/etc/os-release");
        if (f) {
            std::string line;
            while (std::getline(f, line)) {
                if (line.rfind("PRETTY_NAME=", 0) == 0) {
                    std::string val = line.substr(12);
                    // Strip surrounding quotes if present.
                    if (!val.empty() && val.front() == '"') val = val.substr(1);
                    if (!val.empty() && val.back()  == '"') val.pop_back();
                    info.os_name = val;
                    break;
                }
            }
        }
        if (info.os_name.empty()) info.os_name = "Linux";
    }

    // --- Uptime -------------------------------------------------------------
    {
        std::ifstream f("/proc/uptime");
        if (f) {
            double up = 0.0;
            f >> up;
            info.uptime_seconds = static_cast<uint64_t>(up);
        }
    }

    // --- CPU count ----------------------------------------------------------
    {
        std::ifstream f("/proc/cpuinfo");
        if (f) {
            int count = 0;
            std::string line;
            while (std::getline(f, line)) {
                if (line.rfind("processor", 0) == 0) ++count;
            }
            info.cpu_count = count;
        }
        if (info.cpu_count == 0) {
            const long n = sysconf(_SC_NPROCESSORS_ONLN);
            info.cpu_count = (n > 0) ? static_cast<int>(n) : 1;
        }
    }

    return info;
}

} // namespace metrics
