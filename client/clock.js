export class TimeClient {
  constructor(config, { now = () => performance.now(), fetchTime = (...args) => fetch(...args) } = {}) {
    this.config = config;
    this.now = now;
    this.fetchTime = fetchTime;
    this.sample = null;
    this.lastError = null;
    this.pending = false;
    this.timer = null;
  }

  start() {
    if (this.timer !== null) return;
    void this.synchronize();
    this.timer = setInterval(() => void this.synchronize(), this.config.syncIntervalMs);
  }

  async synchronize() {
    if (this.pending) return false;
    this.pending = true;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.config.requestTimeoutMs);
    try {
      const t0 = this.now();
      const response = await this.fetchTime(`${this.config.serverAddress.replace(/\/$/, '')}/api/time`, {
        cache: 'no-store',
        signal: controller.signal,
      });
      if (!response.ok) throw new Error(`Time request failed: HTTP ${response.status}`);
      const data = await response.json();
      const t1 = this.now();
      if (!Number.isSafeInteger(data?.unixTimeMilliseconds)
          || Math.abs(data.unixTimeMilliseconds) > 8_640_000_000_000_000
          || !Number.isInteger(data?.utcOffsetSeconds)
          || Math.abs(data.utcOffsetSeconds) > 86_400) {
        throw new Error('Invalid time response');
      }
      const utcMilliseconds = data.unixTimeMilliseconds + (t1 - t0) / 2;
      if (!Number.isFinite(new Date(utcMilliseconds + data.utcOffsetSeconds * 1000).getTime())) {
        throw new Error('Time response is outside the supported date range');
      }
      this.sample = { utcMilliseconds, offsetSeconds: data.utcOffsetSeconds, receivedAt: t1 };
      this.lastError = null;
      return true;
    } catch (error) {
      this.lastError = error;
      return false;
    } finally {
      clearTimeout(timeout);
      this.pending = false;
    }
  }

  read() {
    if (!this.sample) return { civilTime: null, stale: false, syncAgeMs: null };
    const syncAgeMs = this.now() - this.sample.receivedAt;
    return {
      civilTime: new Date(this.sample.utcMilliseconds + this.sample.offsetSeconds * 1000 + syncAgeMs),
      stale: syncAgeMs >= this.config.staleAfterMs,
      syncAgeMs,
    };
  }
}

// The Date holds server civil time shifted into UTC. UTC accessors/formatting
// intentionally avoid applying the browser's own timezone a second time.
const dateFormatter = new Intl.DateTimeFormat('en-US', {
  weekday: 'long', month: 'long', day: 'numeric', timeZone: 'UTC',
});

export function formatTime(civilTime) {
  if (!civilTime) return { hoursMinutes: '--:--', seconds: '--', date: '' };
  return {
    hoursMinutes: `${civilTime.getUTCHours() % 12 || 12}:${String(civilTime.getUTCMinutes()).padStart(2, '0')}`,
    seconds: String(civilTime.getUTCSeconds()).padStart(2, '0'),
    date: dateFormatter.format(civilTime),
  };
}

export class ClockDisplay {
  constructor(timeClient, root) {
    this.timeClient = timeClient;
    this.hoursMinutes = root.getElementById('hours-minutes');
    this.seconds = root.getElementById('seconds');
    this.date = root.getElementById('date');
    this.status = root.getElementById('sync-status');
  }

  render() {
    const state = this.timeClient.read();
    const formatted = formatTime(state.civilTime);
    this.setText(this.hoursMinutes, formatted.hoursMinutes);
    this.setText(this.seconds, formatted.seconds);
    this.setText(this.date, formatted.date);
    this.setText(this.status, !state.civilTime ? 'Waiting for time' : state.stale ? 'Sync delayed' : '');
  }

  setText(element, text) {
    if (element.textContent !== text) element.textContent = text;
  }

  start() {
    const frame = () => {
      this.render();
      requestAnimationFrame(frame);
    };
    frame();
  }
}
