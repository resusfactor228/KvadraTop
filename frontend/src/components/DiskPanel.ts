import {
  Chart,
  LineController,
  LineElement,
  PointElement,
  LinearScale,
  CategoryScale,
  Filler,
  Legend,
} from 'chart.js';
import type { DiskDevice } from '../types/metrics';

Chart.register(
  LineController, LineElement, PointElement,
  LinearScale, CategoryScale, Filler, Legend,
);

const HISTORY_LENGTH = 60;

/**
 * Renders the Disk panel:
 *  - Per-device name, read/write rates, utilisation bar
 *  - A combined read/write history chart
 */
export class DiskPanel {
  private readonly deviceListEl: HTMLElement;
  private readonly chart: Chart;
  private readonly readHistory:  number[] = new Array(HISTORY_LENGTH).fill(0);
  private readonly writeHistory: number[] = new Array(HISTORY_LENGTH).fill(0);

  constructor(container: HTMLElement) {
    container.innerHTML = `
      <div class="panel__header">
        <h2 class="panel__title">Disk</h2>
      </div>
      <div class="disk-device-list" id="disk-devices"></div>
      <canvas id="disk-chart" height="80"></canvas>
    `;

    this.deviceListEl = container.querySelector('#disk-devices')!;

    const canvas = container.querySelector<HTMLCanvasElement>('#disk-chart')!;
    this.chart = new Chart(canvas, {
      type: 'line',
      data: {
        labels: new Array(HISTORY_LENGTH).fill(''),
        datasets: [
          {
            label: 'Read',
            data: [...this.readHistory],
            borderColor: '#39d353',
            backgroundColor: 'rgba(57,211,83,0.08)',
            borderWidth: 1.5,
            pointRadius: 0,
            fill: true,
            tension: 0.3,
          },
          {
            label: 'Write',
            data: [...this.writeHistory],
            borderColor: '#bc8cff',
            backgroundColor: 'rgba(188,140,255,0.08)',
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

  /** Apply a new list of disk device snapshots to the panel. */
  update(devices: DiskDevice[]): void {
    if (devices.length === 0) {
      this.deviceListEl.innerHTML = '<p class="stat-label" style="padding:4px 0">No disks detected</p>';
    } else {
      this.deviceListEl.innerHTML = devices.map(dev => {
        const utilClass = utilColourClass(dev.util_percent);
        return `
          <div class="disk-device">
            <div class="disk-device__header">
              <span class="disk-device__name">${dev.name}</span>
              <span class="disk-device__util">${dev.util_percent.toFixed(1)}% util</span>
            </div>
            <div class="disk-util-bar">
              <div class="disk-util-bar__fill ${utilClass}"
                   style="width:${Math.min(dev.util_percent, 100).toFixed(1)}%"></div>
            </div>
            <div class="disk-device__rates">
              <span class="disk-rate">
                <span class="disk-rate__label">R</span>
                <span class="disk-rate__value">${formatRate(dev.read_rate_bps)}</span>
              </span>
              <span class="disk-rate">
                <span class="disk-rate__label">W</span>
                <span class="disk-rate__value">${formatRate(dev.write_rate_bps)}</span>
              </span>
            </div>
          </div>
        `;
      }).join('');
    }

    // History chart: aggregate across all devices
    const totalRead  = devices.reduce((s, d) => s + d.read_rate_bps,  0);
    const totalWrite = devices.reduce((s, d) => s + d.write_rate_bps, 0);

    this.readHistory.push(totalRead);
    this.writeHistory.push(totalWrite);
    if (this.readHistory.length  > HISTORY_LENGTH) this.readHistory.shift();
    if (this.writeHistory.length > HISTORY_LENGTH) this.writeHistory.shift();

    this.chart.data.datasets[0].data = [...this.readHistory];
    this.chart.data.datasets[1].data = [...this.writeHistory];
    this.chart.update('none');
  }
}

/** CSS class for util bar colour based on utilisation percentage. */
function utilColourClass(pct: number): string {
  if (pct >= 80) return 'usage--high';
  if (pct >= 50) return 'usage--medium';
  return '';
}

/** Format bytes/second as a human-readable rate string. */
function formatRate(bps: number): string {
  if (bps >= 1024 * 1024) return `${(bps / 1024 / 1024).toFixed(1)} MB/s`;
  if (bps >= 1024)        return `${(bps / 1024).toFixed(1)} KB/s`;
  return `${bps.toFixed(0)} B/s`;
}
