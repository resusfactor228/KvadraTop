/** Per-core CPU usage snapshot. */
export interface CpuCore {
  index: number;
  usage: number;
}

/** Aggregate CPU snapshot including per-core usage and load averages. */
export interface CpuMetrics {
  total: number;
  load_avg: [number, number, number];
  cores: CpuCore[];
}

/** Physical and virtual memory statistics. */
export interface MemoryMetrics {
  total_kb: number;
  used_kb: number;
  free_kb: number;
  available_kb: number;
  buffers_kb: number;
  cached_kb: number;
  swap_total_kb: number;
  swap_used_kb: number;
  usage_percent: number;
}

/** Per-interface network statistics and byte rates. */
export interface NetworkInterface {
  name: string;
  rx_bytes: number;
  tx_bytes: number;
  rx_rate_bps: number;
  tx_rate_bps: number;
}

/** Per-device disk I/O statistics, rates, and utilisation. */
export interface DiskDevice {
  name: string;
  read_bytes: number;
  write_bytes: number;
  read_rate_bps: number;
  write_rate_bps: number;
  util_percent: number;
}

/** A single process entry in the process list. */
export interface ProcessEntry {
  pid: number;
  name: string;
  user: string;
  state: string;
  cpu_percent: number;
  mem_percent: number;
  mem_rss_kb: number;
  threads: number;
  uptime_s: number;
}

/** System-level information. */
export interface SystemInfo {
  hostname: string;
  kernel: string;
  os: string;
  uptime_seconds: number;
  cpu_count: number;
}

/** Complete metrics snapshot broadcast every second. */
export interface Snapshot {
  timestamp_ms: number;
  cpu: CpuMetrics;
  memory: MemoryMetrics;
  network: NetworkInterface[];
  disk: DiskDevice[];
  processes: ProcessEntry[];
  system: SystemInfo;
}
