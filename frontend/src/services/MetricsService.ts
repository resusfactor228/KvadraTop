import type { Snapshot } from '../types/metrics';

type SnapshotHandler   = (snapshot: Snapshot) => void;
type ConnectionHandler = (connected: boolean) => void;

/**
 * Manages the WebSocket connection to the KvadraTop backend.
 *
 * Automatically reconnects after a configurable delay if the connection
 * drops.  Registered handlers are called on every snapshot or connection
 * state change.
 */
export class MetricsService {
  private socket: WebSocket | null = null;
  private readonly url: string;
  private snapshotHandlers:   SnapshotHandler[]   = [];
  private connectionHandlers: ConnectionHandler[] = [];
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private readonly reconnectDelay = 3000;  // ms
  private destroyed = false;

  constructor(url: string) {
    this.url = url;
  }

  /** Open the WebSocket connection.  Safe to call multiple times. */
  connect(): void {
    if (this.destroyed) return;

    // Cancel any pending reconnect timer.
    if (this.reconnectTimer !== null) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }

    this.socket = new WebSocket(this.url);
    this.socket.binaryType = 'blob';

    this.socket.addEventListener('open', () => {
      this.connectionHandlers.forEach(h => h(true));
    });

    this.socket.addEventListener('message', (event: MessageEvent<string>) => {
      try {
        const snapshot = JSON.parse(event.data) as Snapshot;
        this.snapshotHandlers.forEach(h => h(snapshot));
      } catch {
        console.error('[MetricsService] Failed to parse snapshot:', event.data.slice(0, 100));
      }
    });

    this.socket.addEventListener('close', () => {
      this.connectionHandlers.forEach(h => h(false));
      if (!this.destroyed) {
        this.scheduleReconnect();
      }
    });

    this.socket.addEventListener('error', () => {
      // The 'close' event will follow — let that trigger the reconnect.
      this.socket?.close();
    });
  }

  /** Register a callback for every parsed snapshot. */
  onSnapshot(handler: SnapshotHandler): void {
    this.snapshotHandlers.push(handler);
  }

  /** Register a callback for connection state changes. */
  onConnection(handler: ConnectionHandler): void {
    this.connectionHandlers.push(handler);
  }

  /** Permanently close the connection and cancel pending reconnects. */
  disconnect(): void {
    this.destroyed = true;
    if (this.reconnectTimer !== null) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    this.socket?.close();
    this.socket = null;
  }

  // ---------------------------------------------------------------------------
  // Private
  // ---------------------------------------------------------------------------

  private scheduleReconnect(): void {
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.connect();
    }, this.reconnectDelay);
  }
}
