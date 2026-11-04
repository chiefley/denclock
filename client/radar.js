const MINUTE = 60_000;
export const RADAR_METADATA_URL = 'https://api.rainviewer.com/public/weather-maps.json';
export const RADAR_ZOOM = 7;

export function parseRadarMetadata(data) {
  const host = new URL(data?.host);
  if (host.protocol !== 'https:' || host.username || host.password || host.search || host.hash ||
      host.pathname !== '/' || !Array.isArray(data?.radar?.past)) throw Error('Invalid radar metadata');
  const frames = new Map();
  for (const frame of data.radar.past) {
    if (Number.isInteger(frame?.time) && frame.time > 0 && frame.time < 8_640_000_000_000 &&
        typeof frame.path === 'string' && /^\/v2\/radar\/[\w/-]+$/.test(frame.path) && !frame.path.includes('..'))
      frames.set(frame.time, { time: frame.time, path: frame.path });
  }
  const past = [...frames.values()].sort((a, b) => a.time - b.time).slice(-13);
  if (!past.length) throw Error('No historical radar frames');
  return { host: host.origin, frames: past };
}

// Fixed regional Web Mercator map, with no gestures or controls to intercept taps.
// Keep the longest edge at 512 map pixels; CSS size/DPR do not multiply requests.
export function radarLayout(location, width, height) {
  if (!Number.isFinite(location?.latitude) || Math.abs(location.latitude) > 90 ||
      !Number.isFinite(location?.longitude) || Math.abs(location.longitude) > 180 ||
      !(width > 0 && height > 0)) return null;
  const count = 2 ** RADAR_ZOOM, world = count * 256;
  const latitude = Math.max(-85.05112878, Math.min(85.05112878, location.latitude)) * Math.PI / 180;
  const cx = (location.longitude + 180) / 360 * world;
  const cy = (1 - Math.asinh(Math.tan(latitude)) / Math.PI) / 2 * world;
  const scale = Math.max(width, height) / 512;
  const left = cx - width / scale / 2, top = cy - height / scale / 2;
  const tiles = [];
  for (let y = Math.floor(top / 256); y < Math.ceil((top + height / scale) / 256); y++) {
    if (y < 0 || y >= count) continue;
    for (let x = Math.floor(left / 256); x < Math.ceil((left + width / scale) / 256); x++) {
      tiles.push({ x: ((x % count) + count) % count, y,
        left: (x * 256 - left) * scale, top: (y * 256 - top) * scale, size: 256 * scale });
    }
  }
  return { width, height, tiles, key: `${location.latitude},${location.longitude}/${width}/${height}` };
}

export function radarTileUrl(metadata, frame, tile) {
  return `${metadata.host}${frame.path}/256/${RADAR_ZOOM}/${tile.x}/${tile.y}/2/1_1.png`;
}

function loadImage(url, signal) {
  return new Promise((resolve, reject) => {
    const image = new Image();
    let timeout;
    const finish = error => {
      clearTimeout(timeout);
      image.onload = image.onerror = null;
      signal.removeEventListener('abort', abort);
      if (error) { image.src = ''; reject(error); } else resolve(image);
    };
    const abort = () => finish(Error('Radar load cancelled'));
    if (signal.aborted) { abort(); return; }
    signal.addEventListener('abort', abort, { once: true });
    image.onload = () => finish(); image.onerror = () => finish(Error('Map tile unavailable'));
    timeout = setTimeout(() => finish(Error('Map tile timeout')), 8_000);
    // Normal browser HTTP caching and Referer are preserved, including OSM tiles.
    image.src = url;
  });
}

// One queue for map and radar tiles: <=75 starts/minute plus one metadata request
// per five minutes, below RainViewer's documented 100/IP/minute transition limit.
export class RadarTiles {
  constructor({ load = loadImage, now = () => performance.now(), schedule = (fn, delay) => setTimeout(fn, delay), cancel = id => clearTimeout(id) } = {}) {
    this.load = load; this.now = now; this.schedule = schedule; this.cancel = cancel;
    this.cache = new Map(); this.jobs = []; this.lastStart = -Infinity;
    this.active = null; this.timer = null; this.disposed = false;
  }
  get(url, valid = () => true) {
    if (this.disposed || !valid()) return Promise.reject(Error('Radar load cancelled'));
    let entry = this.cache.get(url);
    if (entry?.failedAt !== undefined && this.now() - entry.failedAt >= 5 * MINUTE) {
      this.cache.delete(url); entry = null;
    }
    if (entry?.valid && !entry.valid()) { this.cache.delete(url); entry = null; }
    if (entry) { this.cache.delete(url); this.cache.set(url, entry); return entry.promise; }
    entry = { valid };
    entry.promise = new Promise((resolve, reject) => this.jobs.push({ url, entry, valid, resolve, reject }));
    this.cache.set(url, entry);
    // Bound successful/failed image retention; pending requests stay in the queue.
    while (this.cache.size > 192) this.cache.delete(this.cache.keys().next().value);
    this.pump();
    return entry.promise;
  }
  pump() {
    if (this.disposed || this.active || this.timer !== null || !this.jobs.length) return;
    while (this.jobs.length && !this.jobs[0].valid()) {
      const job = this.jobs.shift();
      if (this.cache.get(job.url) === job.entry) this.cache.delete(job.url);
      job.reject(Error('Radar load cancelled'));
    }
    if (!this.jobs.length) return;
    const delay = Math.max(0, 800 - (this.now() - this.lastStart));
    if (delay) { this.timer = this.schedule(() => { this.timer = null; this.pump(); }, delay); return; }
    const job = this.jobs.shift();
    this.lastStart = this.now();
    const controller = new AbortController(); this.active = controller;
    Promise.resolve().then(() => this.load(job.url, controller.signal)).then(image => {
      delete job.entry.valid; job.resolve(image);
    }, error => {
      delete job.entry.valid; job.entry.failedAt = this.now(); job.reject(error);
    }).finally(() => { this.active = null; this.pump(); });
  }
  dispose() {
    this.disposed = true; this.cancel(this.timer); this.active?.abort();
    for (const job of this.jobs) job.reject(Error('Radar load cancelled'));
    this.jobs = []; this.cache.clear();
  }
}

export class RadarAnimation {
  constructor(draw, { schedule = (fn, delay) => setTimeout(fn, delay), cancel = id => clearTimeout(id) } = {}) {
    this.draw = draw; this.schedule = schedule; this.cancel = cancel; this.timer = null;
  }
  stop() { this.cancel(this.timer); this.timer = null; }
  play(frames) {
    this.stop();
    if (!frames.length) return;
    let step = 0;
    const tick = () => {
      this.timer = null;
      this.draw(frames[step % frames.length]);
      if (++step < frames.length * 2 && frames.length > 1) this.timer = this.schedule(tick, 500);
    };
    tick();
  }
}

export class RadarView {
  constructor(root, { fetchData = (...args) => fetch(...args), now = () => performance.now(),
    tiles = new RadarTiles(), observe = callback => new ResizeObserver(callback) } = {}) {
    this.root = root; this.area = root.getElementById('radar-view');
    this.base = root.getElementById('radar-base'); this.image = root.getElementById('radar-image');
    this.marker = root.getElementById('radar-home'); this.status = root.getElementById('radar-status');
    this.timestamp = root.getElementById('radar-time');
    this.replayButton = root.getElementById('radar-replay');
    this.onReplay = event => { event.stopPropagation(); this.replayAnimation(); };
    this.replayButton.addEventListener('click', this.onReplay);
    this.fetchData = fetchData; this.now = now; this.tiles = tiles;
    this.metadata = null; this.receivedAt = null; this.error = false; this.revision = 0;
    this.frames = []; this.visible = false; this.token = 0; this.disposed = false;
    this.animation = new RadarAnimation(frame => this.draw(frame));
    this.observer = observe(() => { if (this.visible) void this.prepare(); });
    this.observer.observe(this.area);
    void this.refresh();
    this.timer = setInterval(() => void this.refresh(), 5 * MINUTE);
  }
  async refresh() {
    if (this.pending || this.disposed) return;
    const controller = new AbortController(); this.pending = controller;
    const timeout = setTimeout(() => controller.abort(), 8_000);
    try {
      const response = await this.fetchData(RADAR_METADATA_URL, { signal: controller.signal });
      if (!response.ok) throw Error('Radar metadata unavailable');
      const metadata = parseRadarMetadata(await response.json());
      if (this.disposed) return;
      this.metadata = metadata; this.receivedAt = this.now(); this.error = false; this.revision++;
      if (this.visible) void this.prepare();
    } catch { if (!this.disposed) this.error = true; }
    finally { clearTimeout(timeout); this.pending = null; this.updateStatus(); }
  }
  update({ visible, location, utc, timezone }) {
    this.utc = utc; this.timezone = timezone;
    const key = location && `${location.latitude},${location.longitude}`;
    if (key !== this.locationKey) {
      this.locationKey = key; this.location = location; this.token++;
      this.animation.stop(); this.frames = []; this.readyKey = null; this.baseKey = null;
      this.image.getContext('2d').clearRect(0, 0, this.image.width, this.image.height);
      this.base.getContext('2d').clearRect(0, 0, this.base.width, this.base.height);
      this.timestamp.textContent = ''; this.displayed = null;
    }
    // The owning forecast render also resumes/replays after background-tab pauses.
    visible = visible && !this.root.hidden;
    if (visible !== this.visible) {
      this.visible = visible; this.token++; this.animation.stop();
      this.replay = visible;
      this.lastAttempt = null;
    }
    if (visible && utc !== null) void this.prepare();
    this.updateStatus();
  }
  expired() {
    const newest = this.frames.at(-1) ?? this.metadata?.frames.at(-1);
    return newest && this.utc !== null && this.utc - newest.time * 1000 >= 30 * MINUTE;
  }
  updateStatus() {
    if (this.disposed) return;
    const expired = this.expired();
    const show = this.frames.length > 0 && !expired && this.utc !== null;
    const oldImagery = this.frames.length > 0 && this.utc !== null &&
      this.utc - this.frames.at(-1).time * 1000 >= 20 * MINUTE;
    this.image.hidden = !show; this.marker.hidden = !show;
    this.timestamp.hidden = !this.frames.length || !Number.isFinite(this.utc);
    this.replayButton.disabled = !show || !this.visible;
    this.updateTimestamp();
    if (expired) this.animation.stop();
    this.status.textContent = !this.location ? 'Waiting for forecast settings' : this.utc === null ? 'Waiting for synchronized time' :
      expired ? 'Radar expired · waiting for an update' : !this.metadata ?
      (this.error ? 'Radar unavailable · retrying automatically' : 'Loading radar…') :
      (this.error || oldImagery || this.receivedAt !== null && this.now() - this.receivedAt >= 10 * MINUTE) && show ?
      'Radar update delayed · showing last available imagery' : this.loading ? 'Loading radar history…' : this.error ?
      'Radar unavailable · retrying automatically' :
      this.incomplete ? 'Some radar frames unavailable' : this.baseError ? 'Geographic map unavailable' : '';
  }
  canvas(layout) {
    const canvas = this.root.createElement('canvas');
    const ratio = Math.min(globalThis.devicePixelRatio || 1, 2);
    canvas.width = Math.round(layout.width * ratio); canvas.height = Math.round(layout.height * ratio);
    canvas.getContext('2d').scale(ratio, ratio);
    return canvas;
  }
  async compose(layout, url, valid) {
    const images = await Promise.all(layout.tiles.map(tile => this.tiles.get(url(tile), valid)));
    if (!valid()) throw Error('Radar load cancelled');
    const canvas = this.canvas(layout), ctx = canvas.getContext('2d');
    layout.tiles.forEach((tile, i) => ctx.drawImage(images[i], tile.left, tile.top, tile.size, tile.size));
    return canvas;
  }
  copy(source, target) {
    target.width = source.width; target.height = source.height;
    target.getContext('2d').drawImage(source, 0, 0);
  }
  draw(frame) {
    if (this.disposed || !this.visible) return;
    this.copy(frame.canvas, this.image); this.displayed = frame;
    this.updateTimestamp();
  }
  updateTimestamp() {
    const newest = this.frames.at(-1);
    if (!newest || !Number.isFinite(this.utc)) {
      this.timestamp.textContent = '';
      this.timestamp.classList.toggle('is-stale', false);
      return;
    }
    const age = Math.max(0, Math.floor((this.utc - newest.time * 1000) / MINUTE));
    const time = new Intl.DateTimeFormat('en-US', {
      hour: 'numeric', minute: '2-digit', timeZone: this.timezone ?? 'UTC', timeZoneName: 'short',
    }).format(newest.time * 1000);
    this.timestamp.textContent = `Radar ${time} · ${age} min old${age >= 20 ? ' · stale' : ''}`;
    this.timestamp.classList.toggle('is-stale', age >= 20);
  }
  replayAnimation() {
    if (this.disposed || !this.visible || !this.frames.length || this.expired() || !Number.isFinite(this.utc)) return;
    this.replay = false;
    this.animation.play(this.frames);
  }
  async prepare() {
    if (this.disposed || !this.visible || !this.metadata || this.utc === null || this.utc - this.metadata.frames.at(-1).time * 1000 >= 30 * MINUTE) return;
    const layout = radarLayout(this.location, Math.round(this.area.clientWidth), Math.round(this.area.clientHeight));
    if (!layout) { this.status.textContent = 'Waiting for forecast settings'; return; }
    const metadata = this.metadata;
    const sequence = `${layout.key}/${metadata.host}/${metadata.frames.map(f => `${f.time}:${f.path}`).join(',')}`;
    const attempt = `${sequence}/${this.revision}`;
    if (sequence === this.readyKey && (!(this.incomplete || this.baseError) || this.lastAttempt === attempt)) {
      if (this.replay) { this.replay = false; this.animation.play(this.frames); }
      return;
    }
    if (this.lastAttempt === attempt) return;
    this.lastAttempt = attempt;
    const token = ++this.token, valid = () => !this.disposed && this.visible && this.token === token;
    this.loading = true; this.animation.stop(); this.updateStatus();
    try {
      if (this.baseKey !== layout.key) {
        try {
          const base = await this.compose(layout, tile => `https://tile.openstreetmap.org/${RADAR_ZOOM}/${tile.x}/${tile.y}.png`, valid);
          if (!valid()) return;
          this.copy(base, this.base); this.baseKey = layout.key; this.baseError = false;
        } catch { if (!valid()) return; this.baseError = true; }
      }
      const frames = [];
      // Show a complete newest frame first, then preload the historical loop.
      for (const frame of [...metadata.frames].reverse()) {
        try {
          const canvas = await this.compose(layout, tile => radarTileUrl(metadata, frame, tile), valid);
          if (!valid()) return;
          frames.push({ ...frame, canvas });
          if (frames.length === 1) {
            this.frames = [frames[0]]; this.draw(frames[0]); this.updateStatus();
          }
        } catch { if (!valid()) return; }
      }
      if (!frames.length) throw Error('Radar frames unavailable');
      this.frames = frames.sort((a, b) => a.time - b.time); this.readyKey = sequence;
      this.incomplete = frames.length !== metadata.frames.length;
      this.replay = false; this.animation.play(this.frames);
    } catch { if (valid()) this.error = true; }
    finally { if (valid()) { this.loading = false; this.updateStatus(); } }
  }
  dispose() {
    this.disposed = true; this.token++; clearInterval(this.timer); this.pending?.abort();
    this.replayButton.removeEventListener('click', this.onReplay);
    this.animation.stop(); this.observer.disconnect(); this.tiles.dispose(); this.frames = []; this.displayed = null; this.metadata = null;
    this.base.width = this.image.width = 0;
  }
}

export function initializeRadar(root, options) {
  return root.getElementById('radar-view') ? new RadarView(root, options) : null;
}
