import type { ProcessEntry } from '../types/metrics';

type SortKey = keyof Pick<
  ProcessEntry,
  'pid' | 'name' | 'user' | 'cpu_percent' | 'mem_percent' | 'mem_rss_kb' | 'threads'
>;

/**
 * Renders the sortable process table.
 *
 * Clicking a column header sorts by that column; clicking again toggles
 * the sort direction.  The default sort is CPU% descending.
 */
export class ProcessTable {
  private readonly tbody: HTMLTableSectionElement;
  private readonly countEl: HTMLElement;
  private sortKey: SortKey = 'cpu_percent';
  private sortAsc = false;

  constructor(container: HTMLElement) {
    container.innerHTML = `
      <div class="panel__header">
        <h2 class="panel__title">Processes</h2>
        <span id="proc-count" class="stat-badge">0 processes</span>
      </div>
      <div class="table-scroll">
        <table class="process-table">
          <thead>
            <tr>
              <th data-key="pid"         style="width:60px">PID</th>
              <th data-key="name"        style="width:160px">Name</th>
              <th data-key="user"        style="width:90px">User</th>
              <th                        style="width:60px">State</th>
              <th data-key="cpu_percent" style="width:70px" class="sort-desc">CPU%</th>
              <th data-key="mem_percent" style="width:70px">MEM%</th>
              <th data-key="mem_rss_kb"  style="width:80px">RSS</th>
              <th data-key="threads"     style="width:70px">Threads</th>
            </tr>
          </thead>
          <tbody id="proc-tbody"></tbody>
        </table>
      </div>
    `;

    this.tbody   = container.querySelector<HTMLTableSectionElement>('#proc-tbody')!;
    this.countEl = container.querySelector('#proc-count')!;

    // Attach sort handlers to column headers.
    container.querySelectorAll<HTMLElement>('th[data-key]').forEach(th => {
      th.addEventListener('click', () => {
        const key = th.dataset['key'] as SortKey;
        if (this.sortKey === key) {
          this.sortAsc = !this.sortAsc;
        } else {
          this.sortKey = key;
          this.sortAsc = false;
        }
        this.updateSortIndicators(container);
      });
    });
  }

  /** Apply a new process list to the table. */
  update(processes: ProcessEntry[]): void {
    this.countEl.textContent = `${processes.length} processes`;

    const sorted = [...processes].sort((a, b) => {
      const av = a[this.sortKey];
      const bv = b[this.sortKey];
      let cmp: number;
      if (typeof av === 'string' && typeof bv === 'string') {
        cmp = av.localeCompare(bv);
      } else {
        cmp = (av as number) - (bv as number);
      }
      return this.sortAsc ? cmp : -cmp;
    });

    this.tbody.innerHTML = sorted.map(p => `
      <tr>
        <td class="col-pid">${p.pid}</td>
        <td class="col-name" title="${escapeHtml(p.name)}">${escapeHtml(p.name)}</td>
        <td class="col-user">${escapeHtml(p.user)}</td>
        <td><span class="state-badge state-${p.state}">${p.state}</span></td>
        <td class="col-num ${cpuClass(p.cpu_percent)}">${p.cpu_percent.toFixed(1)}</td>
        <td class="col-num">${p.mem_percent.toFixed(1)}</td>
        <td class="col-num">${formatKb(p.mem_rss_kb)}</td>
        <td class="col-num">${p.threads}</td>
      </tr>
    `).join('');
  }

  // ---------------------------------------------------------------------------
  // Private
  // ---------------------------------------------------------------------------

  private updateSortIndicators(container: HTMLElement): void {
    container.querySelectorAll<HTMLElement>('th[data-key]').forEach(th => {
      th.classList.remove('sort-asc', 'sort-desc');
      if (th.dataset['key'] === this.sortKey) {
        th.classList.add(this.sortAsc ? 'sort-asc' : 'sort-desc');
      }
    });
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function formatKb(kb: number): string {
  if (kb >= 1024 * 1024) return `${(kb / 1024 / 1024).toFixed(1)}G`;
  if (kb >= 1024)        return `${(kb / 1024).toFixed(1)}M`;
  return `${kb}K`;
}

function cpuClass(pct: number): string {
  if (pct >= 50) return 'usage--high';
  if (pct >= 10) return 'usage--medium';
  return '';
}

function escapeHtml(s: string): string {
  return s
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}
