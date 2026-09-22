const positions = ['upperLeft', 'upperRight'];
const isNumber = value => typeof value === 'number' && Number.isFinite(value);
const utcPattern = /^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d+)?(?:Z|\+00:00)$/;
function utcMilliseconds(value) {
  if (typeof value !== 'string' || !utcPattern.test(value)) throw new Error('Invalid UTC timestamp');
  const time = Date.parse(value);
  if (!Number.isFinite(time)) throw new Error('Invalid UTC timestamp');
  return time;
}
export function validateSnapshot(data) {
  if (!data || !['C', 'F'].includes(data.temperatureUnit) || !Array.isArray(data.slots) || data.slots.length !== 2)
    throw new Error('Invalid presentation snapshot');
  const utc = utcMilliseconds(data.serverUtc);
  const names = new Set();
  const slots = positions.map(position => {
    const matches = data.slots.filter(slot => slot?.position === position);
    if (matches.length !== 1) throw new Error('Invalid presentation positions');
    const slot = matches[0];
    if (typeof slot.displayName !== 'string') throw new Error('Invalid sensor label');
    if (slot.internalName === null) {
      if (slot.displayName !== '' || slot.maxAgeSeconds !== null || slot.observation !== null)
        throw new Error('Invalid empty slot');
    } else {
      if (typeof slot.internalName !== 'string' || !slot.internalName.trim() || !slot.displayName.trim()
          || names.has(slot.internalName) || !isNumber(slot.maxAgeSeconds) || slot.maxAgeSeconds <= 0)
        throw new Error('Invalid sensor configuration');
      names.add(slot.internalName);
    }
    let observation = null;
    if (slot.observation !== null) {
      const value = slot.observation;
      if (!value || typeof value !== 'object') throw new Error('Invalid observation');
      const receivedAt = utcMilliseconds(value.receivedUtc);
      if (receivedAt > utc) throw new Error('Future observation');
      if (value.temperatureC !== null && (!isNumber(value.temperatureC) || !isNumber(value.temperatureC * 1.8 + 32)))
        throw new Error('Invalid temperature');
      if (value.relativeHumidity !== null && (!isNumber(value.relativeHumidity) || value.relativeHumidity < 0 || value.relativeHumidity > 100))
        throw new Error('Invalid humidity');
      observation = { ...value, receivedAt };
    }
    return { ...slot, observation };
  });
  return { utc, temperatureUnit: data.temperatureUnit, slots };
}
export class PresentationClient {
  constructor(config, { now = () => performance.now(), fetchSnapshot = (...args) => fetch(...args) } = {}) {
    this.config = config;
    this.now = now;
    this.fetchSnapshot = fetchSnapshot;
    this.sample = null;
    this.pending = false;
    this.timer = null;
    this.lastError = null;
  }
  start() {
    if (this.timer !== null) return;
    void this.synchronize();
    this.timer = setInterval(() => void this.synchronize(), this.config.presentationIntervalMs);
  }
  stop() { clearInterval(this.timer); this.timer = null; }
  async synchronize() {
    if (this.pending) return false;
    this.pending = true;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.config.requestTimeoutMs);
    try {
      const started = this.now();
      const response = await this.fetchSnapshot(this.config.serverAddress.replace(/\/$/, '') + '/api/presentation', {
        cache: 'no-store', signal: controller.signal,
      });
      if (!response.ok) throw new Error('Presentation request failed: HTTP ' + response.status);
      const data = validateSnapshot(await response.json());
      const receivedAt = this.now();
      this.sample = { ...data, utc: data.utc + (receivedAt - started) / 2, receivedAt };
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
    if (!this.sample) return {
      temperatureUnit: null,
      slots: positions.map(position => ({ position, displayName: '', observation: null, stale: false })),
    };
    const utc = this.sample.utc + this.now() - this.sample.receivedAt;
    return {
      temperatureUnit: this.sample.temperatureUnit,
      slots: this.sample.slots.map(slot => ({
        ...slot,
        stale: slot.observation !== null && (utc - slot.observation.receivedAt) / 1000 >= slot.maxAgeSeconds,
      })),
    };
  }
}
export function formatTemperature(celsius, unit) {
  if (!isNumber(celsius) || !['C', 'F'].includes(unit)) return { whole: '--', fraction: '.-' };
  const value = unit === 'F' ? celsius * 1.8 + 32 : celsius;
  if (!Number.isFinite(value)) return { whole: '--', fraction: '.-' };
  // Round once before splitting, including carry and signed fractions.
  const rounded = Math.round((Math.abs(value) + Number.EPSILON) * 10) / 10;
  if (!Number.isFinite(rounded)) return { whole: '--', fraction: '.-' };
  const text = rounded.toLocaleString('en-US', { useGrouping: false, minimumFractionDigits: 1, maximumFractionDigits: 1 });
  const [whole, fraction] = text.split('.');
  return { whole: (value < 0 && rounded !== 0 ? '−' : '') + whole, fraction: '.' + fraction };
}
export class SensorDisplay {
  constructor(client, root) {
    this.client = client;
    this.panels = new Map(positions.map(position => [position, root.getElementById(position)]));
  }
  render() {
    const { slots, temperatureUnit } = this.client.read();
    for (const slot of slots) {
      const panel = this.panels.get(slot.position);
      const reading = formatTemperature(slot.observation?.temperatureC, temperatureUnit);
      const set = (selector, value) => {
        const element = panel.querySelector(selector);
        if (element.textContent !== value) element.textContent = value;
      };
      set('.temperature-whole', reading.whole);
      set('.temperature-fraction', reading.fraction);
      set('.temperature-unit', temperatureUnit ? '°' + temperatureUnit : '');
      set('.sensor-name', slot.displayName);
      set('.humidity-value', isNumber(slot.observation?.relativeHumidity) ? Math.round(slot.observation.relativeHumidity) + '%' : '--');
      panel.classList.toggle('stale', slot.stale);
      panel.setAttribute('aria-label', (slot.displayName || 'Unassigned sensor') + ': ' + reading.whole + reading.fraction + (temperatureUnit ? ' degrees ' + temperatureUnit : '') + (slot.stale ? ', stale' : ''));
    }
  }
  start() {
    const frame = () => { this.render(); requestAnimationFrame(frame); };
    frame();
  }
}
