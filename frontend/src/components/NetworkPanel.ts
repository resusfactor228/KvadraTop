import {
  Chart,
  LineController,
  LineElement,
  PointElement,
  LinearScale,
  CategoryScale,
  Filler,
  Legend,
  Tooltip,
} from 'chart.js';
import type { NetworkInterface } from '../types/metrics';

Chart.register(
  LineController, LineElement, PointElement,
  LinearScale, CategoryScale, Filler, Legend, Tooltip,
);

const HISTORY_LENGTH = 60;

/**
 * Renders the Network panel:
 *  - Per-interface RX / TX rates (formatted as KB/s or MB/s)
 *  - A combined history chart with two datasets (RX=blue, TX=orange)
 *    using the highest-traffic interface as source
 */
export class NetworkPanel {
  private readonly ifaceListEl: HTMLElement;
  private readonly chart: Chart;
  private readonly rxHistory: number[] = new Array(HISTORY_LENGTH).fill(0);
  private readonly txHistory: number[] = new Array(HISTORY_LENGTH).fill(0);

  constructor(container: HTMLElement) {
    container.innerHTML = `
      <div class="panel__header">
        <h2 class="panel__title">Network</h2>
      </div>
      <div class="net-iface-list" id="net-ifaces"></div>
      <canvas id="net-chart" height="80"></canvas>
    `;

    this.ifaceListEl = container.querySelector('#net-ifaces')!;

    const canvas = container.querySelector<HTMLCanvasElement>('#net-chart')!;
    this.chart = new Chart(canvas, {
      type: 'line',
      data: {
        labels: new Array(HISTORY_LENGTH).fill(''),
        datasets: [
          {
            label: 'RX',
            data: [...this.rxHistory],
            borderColor: '#58a6ff',
            backgroundColor: 'rgba(88,166,255,0.08)',
            borderWidth: 1.5,
            pointRadius: 0,
            fill: true,
            tension: 0.3,
          },
          {
            label: 'TX',
            data: [...this.txHistory],
            borderColor: '#e3b341',
            backgroundColor: 'rgba(227,179,65,0.08)',
            borderWidth: 1.5,
            pointRadius: 0,
            fill: true,
            tension: 0.3,
          },
        ],
      },
      options: {
        animation: false,
        responsive: true,
        maintainAspectRatio: true,
        scales: {
          y: { min: 0, display: false },
          x: { display: false },
        },
        plugins: {
          legend: {
            display: true,
            position: 'top',
            labels: {
              color: '#8b949e',
              boxWidth: 12,
              padding: 8,
              font: { size: 11 },
            },
          },
          tooltip: { enabled: false },
        },
      },
    });
  }

  /** Apply a new list of network interface snapshots to the panel. */
  update(interfaces: NetworkInterface[]): void {
    // Render per-interface rows
    if (interfaces.length === 0) {
      this.ifaceListEl.innerHTML = '<p class="stat-label" style="padding:4px 0">No interfaces</p>';
    } else {
      this.ifaceListEl.innerHTML = interfaces.map(iface => `
        <div class="net-iface">
          <span class="net-iface__name" title="${iface.name}">${iface.name}</span>
          <div class="net-iface__rates">
            <span class="net-rate net-rate--rx">
              <span class="net-rate__arrow">&#x2193;</span>
              <span class="net-rate__value">${formatRate(iface.rx_rate_bps)}</span>
            </span>
            <span class="net-rate net-rate--tx">
              <span class="net-rate__arrow">&#x2191;</span>
              <span class="net-rate__value">${formatRate(iface.tx_rate_bps)}</span>
            </span>
          </div>
        </div>
      `).join('');
    }

    // For the chart, aggregate totals across all interfaces
    const totalRx = interfaces.reduce((s, i) => s + i.rx_rate_bps, 0);
    const totalTx = interfaces.reduce((s, i) => s + i.tx_rate_bps, 0);

    this.rxHistory.push(totalRx);
    this.txHistory.push(totalTx);
    if (this.rxHistory.length > HISTORY_LENGTH) this.rxHistory.shift();
    if (this.txHistory.length > HISTORY_LENGTH) this.txHistory.shift();

    this.chart.data.datasets[0].data = [...this.rxHistory];
    this.chart.data.datasets[1].data = [...this.txHistory];
    this.chart.update('none');
  }
}

/** Format bytes/second as a human-readable rate string. */
function formatRate(bps: number): string {
  if (bps >= 1024 * 1024) return `${(bps / 1024 / 1024).toFixed(1)} MB/s`;
  if (bps >= 1024)        return `${(bps / 1024).toFixed(1)} KB/s`;
  return `${bps.toFixed(0)} B/s`;
}
