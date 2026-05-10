#include "ProcessMonitor.hpp"
#include <dirent.h>
#include <pwd.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace metrics {

namespace {

/// Returns true if s is a non-empty string of ASCII digits only.
bool all_digits(const char* s) {
    if (!s || *s == '\0') return false;  // reject empty string
    for (; *s; ++s) {
        if (*s < '0' || *s > '9') return false;
    }
    return true;
}

} // anonymous namespace

ProcessMonitor::ProcessMonitor()
    : hertz_(sysconf(_SC_CLK_TCK))
    , prev_total_ticks_(read_total_cpu_ticks())
{
    if (hertz_ <= 0) hertz_ = 100;
}

// ---------------------------------------------------------------------------
// sample()
// ---------------------------------------------------------------------------

std::vector<ProcessEntry> ProcessMonitor::sample(uint64_t total_memory_kb) {
    const uint64_t curr_total_ticks = read_total_cpu_ticks();
    const uint64_t total_delta      = curr_total_ticks > prev_total_ticks_
                                      ? curr_total_ticks - prev_total_ticks_
                                      : 1;

    const long   page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
    const double uptime       = read_uptime();
    const double boot_jiffies = uptime * static_cast<double>(hertz_);

    std::vector<ProcessEntry> entries;

    DIR* dir = opendir("/proc");
    if (!dir) return entries;

    dirent* ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        // Only numeric directories are processes.
        if (ent->d_type != DT_DIR && ent->d_type != DT_UNKNOWN) continue;
        if (!all_digits(ent->d_name)) continue;

        const int pid = std::stoi(ent->d_name);

        ProcStat ps{};
        if (!read_proc_stat(pid, ps)) continue;

        const uint64_t cpu_ticks = ps.utime + ps.stime;

        // CPU delta since last sample.
        double cpu_pct = 0.0;
        if (prev_.count(pid)) {
            const uint64_t proc_delta = cpu_ticks >= prev_[pid].cpu_ticks
                                        ? cpu_ticks - prev_[pid].cpu_ticks
                                        : 0;
            cpu_pct = total_delta > 0
                      ? static_cast<double>(proc_delta) / static_cast<double>(total_delta) * 100.0
                      : 0.0;
        }

        // Memory percentage.
        const uint64_t rss_kb = static_cast<uint64_t>(ps.rss) * static_cast<uint64_t>(page_size_kb);
        const double mem_pct  = total_memory_kb > 0
                                ? static_cast<double>(rss_kb) / static_cast<double>(total_memory_kb) * 100.0
                                : 0.0;

        // Process uptime.
        uint64_t uptime_sec = 0;
        if (boot_jiffies > static_cast<double>(ps.starttime)) {
            uptime_sec = static_cast<uint64_t>(
                (boot_jiffies - static_cast<double>(ps.starttime)) / static_cast<double>(hertz_));
        }

        // Owner username.
        const int uid = read_uid(pid);
        std::string user = uid >= 0 ? uid_to_username(uid) : "?";

        ProcessEntry e{};
        e.pid            = pid;
        e.name           = ps.comm;
        e.user           = std::move(user);
        e.state          = ps.state;
        e.cpu_percent    = cpu_pct;
        e.mem_percent    = mem_pct;
        e.mem_rss_kb     = rss_kb;
        e.threads        = ps.threads;
        e.uptime_seconds = uptime_sec;
        entries.push_back(std::move(e));

        // Update per-process cache.
        prev_[pid] = RawProc{cpu_ticks};
    }
    closedir(dir);

    // Remove PIDs that no longer exist to prevent unbounded map growth.
    for (auto it = prev_.begin(); it != prev_.end(); ) {
        bool found = false;
        for (const auto& e : entries) {
            if (e.pid == it->first) { found = true; break; }
        }
        it = found ? std::next(it) : prev_.erase(it);
    }

    prev_total_ticks_ = curr_total_ticks;

    // Sort descending by cpu_percent, keep top 20.
    std::sort(entries.begin(), entries.end(), [](const ProcessEntry& a, const ProcessEntry& b) {
        return a.cpu_percent > b.cpu_percent;
    });

    if (entries.size() > 20) {
        entries.resize(20);
    }

    return entries;
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

bool ProcessMonitor::read_proc_stat(int pid, ProcStat& out) {
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream f(path);
    if (!f) return false;

    std::string line;
    if (!std::getline(f, line)) return false;

    // The comm field is enclosed in parentheses and may contain spaces and
    // even nested parentheses.  Find the last ')' to locate where it ends.
    const auto open_paren  = line.find('(');
    const auto close_paren = line.rfind(')');
    if (open_paren == std::string::npos || close_paren == std::string::npos) return false;

    out.pid  = std::stoi(line.substr(0, open_paren));
    out.comm = line.substr(open_paren + 1, close_paren - open_paren - 1);

    // Remaining fields start after ')'.
    std::istringstream ss(line.substr(close_paren + 1));

    // Field indices in /proc/pid/stat (after pid and comm, 1-based):
    //  1=state  2=ppid  3=pgrp  4=session  5=tty_nr  6=tpgid  7=flags
    //  8=minflt  9=cminflt  10=majflt  11=cmajflt
    //  12=utime  13=stime  14=cutime  15=cstime
    //  16=priority  17=nice  18=num_threads  19=itrealvalue  20=starttime
    //  21=vsize  22=rss
    char   state;
    long   ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags;
    unsigned long minflt, cminflt, majflt, cmajflt;
    unsigned long utime, stime;
    long cutime, cstime;
    long priority, nice, num_threads;
    long itrealvalue;
    unsigned long long starttime;
    unsigned long vsize;
    long rss;

    ss >> state
       >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags
       >> minflt >> cminflt >> majflt >> cmajflt
       >> utime >> stime >> cutime >> cstime
       >> priority >> nice >> num_threads >> itrealvalue
       >> starttime
       >> vsize >> rss;

    if (ss.fail()) return false;

    out.state     = state;
    out.utime     = static_cast<uint64_t>(utime);
    out.stime     = static_cast<uint64_t>(stime);
    out.rss       = rss;
    out.threads   = static_cast<int>(num_threads);
    out.starttime = static_cast<uint64_t>(starttime);
    return true;
}

int ProcessMonitor::read_uid(int pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream f(path);
    if (!f) return -1;

    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("Uid:", 0) == 0) {
            std::istringstream ss(line.substr(4));
            int uid = -1;
            ss >> uid;
            return uid;
        }
    }
    return -1;
}

std::string ProcessMonitor::uid_to_username(int uid) {
    if (uid < 0) return "?";

    char        buf[2048];
    struct passwd pwbuf{};
    struct passwd* result = nullptr;
    if (getpwuid_r(static_cast<uid_t>(uid), &pwbuf, buf, sizeof(buf), &result) == 0 && result) {
        return result->pw_name;
    }
    return std::to_string(uid);
}

double ProcessMonitor::read_uptime() {
    std::ifstream f("/proc/uptime");
    if (!f) return 0.0;
    double up = 0.0;
    f >> up;
    return up;
}

uint64_t ProcessMonitor::read_total_cpu_ticks() {
    std::ifstream f("/proc/stat");
    if (!f) return 0;

    std::string label;
    uint64_t user = 0, nice = 0, system = 0, idle = 0,
             iowait = 0, irq = 0, softirq = 0, steal = 0;
    f >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

} // namespace metrics
