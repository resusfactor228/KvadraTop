import {
  Chart,
  LineController,
  LineElement,
  PointElement,
  LinearScale,
  CategoryScale,
  Filler,
} from 'chart.js';
import type { MemoryMetrics } from '../types/metrics';

Chart.register(LineController, LineElement, PointElement, LinearScale, CategoryScale, Filler);

const HISTORY_LENGTH = 60;

/**
 * Renders the Memory panel:
 *  - Total RAM usage percentage (large number)
 *  - Used / Total display
 *  - Buffers + cached breakdown
 *  - Swap usage with progress bar
 *  - 60-second usage history chart
 */
export class MemoryPanel {
  private readonly usageEl:      HTMLElement;
  private readonly memBarEl:     HTMLElement;
  private readonly swapBarEl:    HTMLElement;
  private readonly swapDetailEl: HTMLElement;
  private readonly usedValEl:    HTMLElement;
  private readonly cachedValEl:  HTMLElement;
  private readonly chart: Chart;
  private readonly history: number[] = new Array(HISTORY_LENGTH).fill(0);

  constructor(container: HTMLElement) {
    container.innerHTML = `
      <div class="panel__header">
        <h2 class="panel__title">Memory</h2>
        <span class="big-number usage--low" id="mem-usage">0.0%</span>
      </div>
      <div class="progress-bar" title="RAM usage">
        <div class="progress-bar__fill" id="mem-bar" style="width:0%"></div>
      </div>
      <p class="stat-row" id="mem-detail">
        <span class="stat-label">Used</span>
        <span class="stat-value" id="mem-used-val">—</span>
        <span class="stat-label stat-label--right">Cached</span>
        <span class="stat-value" id="mem-cached-val">—</span>
      </p>
      <div class="stat-row">
        <span class="stat-label">Swap</span>
        <span id="swap-detail" class="stat-value">N/A</span>
      </div>
      <div class="progress-bar progress-bar--swap">
        <div class="progress-bar__fill progress-bar__fill--swap" id="swap-bar" style="width:0%"></div>
      </div>
      <canvas id="mem-chart" height="80"></canvas>
    `;

    this.usageEl      = container.querySelector('#mem-usage')!;
    this.memBarEl     = container.querySelector('#mem-bar')!;
    this.swapBarEl    = container.querySelector('#swap-bar')!;
    this.swapDetailEl = container.querySelector('#swap-detail')!;
    this.usedValEl    = container.querySelector('#mem-used-val')!;
    this.cachedValEl  = container.querySelector('#mem-cached-val')!;

    const canvas = container.querySelector<HTMLCanvasElement>('#mem-chart')!;
    this.chart = new Chart(canvas, {
      type: 'line',
      data: {
        labels: new Array(HISTORY_LENGTH).fill(''),
        datasets: [{
          data: [...this.history],
          borderColor: '#39d353',
          backgroundColor: 'rgba(57,211,83,0.08)',
          borderWidth: 1.5,
          pointRadius: 0,
          fill: true,
          tension: 0.3,
        }],
      },
      options: {
        animation: false,
        responsive: true,
        maintainAspectRatio: true,
        scales: {
          y: { min: 0, max: 100, display: false },
          x: { display: false },
        },
        plugins: {
          legend:  { display: false },
          tooltip: { enabled: false },
        },
      },
    });
  }

  /** Apply a new memory metrics snapshot to the panel. */
  update(mem: MemoryMetrics): void {
    // Big number
    this.usageEl.textContent = `${mem.usage_percent.toFixed(1)}%`;
    this.usageEl.className   = `big-number ${usageClass(mem.usage_percent)}`;

    // RAM progress bar
    const pct = Math.min(mem.usage_percent, 100);
    this.memBarEl.style.width = `${pct.toFixed(1)}%`;

    // Used / Total + Cached row
    this.usedValEl.textContent   = `${formatBytes(mem.used_kb * 1024)} / ${formatBytes(mem.total_kb * 1024)}`;
    this.cachedValEl.textContent = formatBytes(mem.cached_kb * 1024);

    // Swap
    const swapPct = mem.swap_total_kb > 0
      ? Math.min((mem.swap_used_kb / mem.swap_total_kb) * 100, 100)
      : 0;
    this.swapBarEl.style.width = `${swapPct.toFixed(1)}%`;
    this.swapDetailEl.textContent = mem.swap_total_kb > 0
      ? `${formatBytes(mem.swap_used_kb * 1024)} / ${formatBytes(mem.swap_total_kb * 1024)}`
      : 'N/A';

    // History chart
    this.history.push(mem.usage_percent);
    if (this.history.length > HISTORY_LENGTH) this.history.shift();
    this.chart.data.datasets[0].data = [...this.history];
    this.chart.update('none');
  }
}

function formatBytes(bytes: number): string {
  if (bytes >= 1024 ** 3) return `${(bytes / 1024 ** 3).toFixed(1)} GB`;
  if (bytes >= 1024 ** 2) return `${(bytes / 1024 ** 2).toFixed(1)} MB`;
  if (bytes >= 1024)      return `${(bytes / 1024).toFixed(1)} KB`;
  return `${bytes} B`;
}

function usageClass(pct: number): string {
  if (pct >= 80) return 'usage--high';
  if (pct >= 50) return 'usage--medium';
  return 'usage--low';
}
