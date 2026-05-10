import './styles/main.css';
import { MetricsService } from './services/MetricsService';
import { CpuPanel }       from './components/CpuPanel';
import { MemoryPanel }    from './components/MemoryPanel';
import { NetworkPanel }   from './components/NetworkPanel';
import { DiskPanel }      from './components/DiskPanel';
import { ProcessTable }   from './components/ProcessTable';
import type { Snapshot }  from './types/metrics';

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

function formatUptime(seconds: number): string {
  const d = Math.floor(seconds / 86400);
  const h = Math.floor((seconds % 86400) / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m`;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

function main(): void {
  // Connect to the backend WebSocket.  In dev mode Vite proxies /ws;
  // in production the backend serves it directly on the same origin.
  const wsUrl = `ws://${window.location.host}/ws`;

  const service = new MetricsService(wsUrl);

  // Header elements
  const statusEl   = document.getElementById('connection-status')!;
  const hostnameEl = document.getElementById('sys-hostname')!;
  const uptimeEl   = document.getElementById('sys-uptime')!;

  // Panel components — each component owns its DOM subtree.
  const cpuPanel     = new CpuPanel(document.querySelector<HTMLElement>('.panel--cpu')!);
  const memPanel     = new MemoryPanel(document.querySelector<HTMLElement>('.panel--memory')!);
  const netPanel     = new NetworkPanel(document.querySelector<HTMLElement>('.panel--network')!);
  const diskPanel    = new DiskPanel(document.querySelector<HTMLElement>('.panel--disk')!);
  const processTable = new ProcessTable(document.querySelector<HTMLElement>('.panel--processes')!);

  // Connection state changes
  service.onConnection((connected: boolean) => {
    statusEl.textContent = connected ? 'Connected' : 'Disconnected';
    statusEl.className   = connected ? 'status-online' : 'status-offline';
  });

  // Snapshot updates
  service.onSnapshot((snap: Snapshot) => {
    hostnameEl.textContent = snap.system.hostname;
    uptimeEl.textContent   = `up ${formatUptime(snap.system.uptime_seconds)}`;

    cpuPanel.update(snap.cpu);
    memPanel.update(snap.memory);
    netPanel.update(snap.network);
    diskPanel.update(snap.disk);
    processTable.update(snap.processes);
  });

  service.connect();
}

main();
