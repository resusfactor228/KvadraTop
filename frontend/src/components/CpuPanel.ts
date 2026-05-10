import {
  Chart,
  LineController,
  LineElement,
  PointElement,
  LinearScale,
  CategoryScale,
  Filler,
  Tooltip,
} from 'chart.js';
import type { CpuMetrics } from '../types/metrics';

Chart.register(LineController, LineElement, PointElement, LinearScale, CategoryScale, Filler, Tooltip);

const HISTORY_LENGTH = 60;

/**
 * Renders the CPU panel:
 *  - Total CPU usage (large number)
 *  - Load averages (1 / 5 / 15 minute)
 *  - 60-second history chart
 *  - Per-core usage bars
 */
export class CpuPanel {
  private readonly totalEl: HTMLElement;
  private readonly loadEl: HTMLElement;
  private readonly coresEl: HTMLElement;
  private readonly chart: Chart;
  private readonly history: number[] = new Array(HISTORY_LENGTH).fill(0);

  constructor(container: HTMLElement) {
    container.innerHTML = `
      <div class="panel__header">
        <h2 class="panel__title">CPU</h2>
        <span class="big-number usage--low" id="cpu-total">0.0%</span>
      </div>
      <p class="stat-row">
        <span class="stat-label">Load avg</span>
        <span id="cpu-load" class="stat-value">0.00 / 0.00 / 0.00</span>
      </p>
      <canvas id="cpu-chart" height="80"></canvas>
      <div class="core-grid" id="cpu-cores"></div>
    `;

    this.totalEl = container.querySelector('#cpu-total')!;
    this.loadEl  = container.querySelector('#cpu-load')!;
    this.coresEl = container.querySelector('#cpu-cores')!;

    const canvas = container.querySelector<HTMLCanvasElement>('#cpu-chart')!;
    this.chart = new Chart(canvas, {
      type: 'line',
      data: {
        labels: new Array(HISTORY_LENGTH).fill(''),
        datasets: [{
          data: [...this.history],
          borderColor: '#58a6ff',
          backgroundColor: 'rgba(88,166,255,0.08)',
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

  /** Apply a new CPU metrics snapshot to the panel. */
  update(cpu: CpuMetrics): void {
    // Total usage
    this.totalEl.textContent = `${cpu.total.toFixed(1)}%`;
    this.totalEl.className   = `big-number ${usageClass(cpu.total)}`;

    // Load averages
    this.loadEl.textContent = cpu.load_avg.map(v => v.toFixed(2)).join(' / ');

    // History chart
    this.history.push(cpu.total);
    if (this.history.length > HISTORY_LENGTH) this.history.shift();
    this.chart.data.datasets[0].data = [...this.history];
    this.chart.update('none');

    // Per-core bars (regenerated each tick — fast enough for ≤ 64 cores)
    this.coresEl.innerHTML = cpu.cores.map(c => `
      <div class="core-bar">
        <span class="core-bar__label">CPU${c.index}</span>
        <div class="core-bar__track">
          <div class="core-bar__fill ${usageClass(c.usage)}"
               style="width:${Math.min(c.usage, 100).toFixed(1)}%"></div>
        </div>
        <span class="core-bar__value">${c.usage.toFixed(0)}%</span>
      </div>
    `).join('');
  }
}

function usageClass(pct: number): string {
  if (pct >= 80) return 'usage--high';
  if (pct >= 50) return 'usage--medium';
  return 'usage--low';
}
