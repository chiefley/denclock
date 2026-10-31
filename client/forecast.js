const HOUR = 3_600_000;
const finite = value => typeof value === 'number' && Number.isFinite(value);

export function forecastUrl(location) {
  if (!location || !finite(location.latitude) || Math.abs(location.latitude) > 90 ||
      !finite(location.longitude) || Math.abs(location.longitude) > 180) return null;
  // Unix timestamps keep the hourly sequence unambiguous across DST changes.
  const params = new URLSearchParams({ latitude: location.latitude, longitude: location.longitude,
    hourly: 'temperature_2m,precipitation_probability', forecast_days: 3,
    timeformat: 'unixtime', timezone: 'auto', temperature_unit: 'celsius' });
  return 'https://api.open-meteo.com/v1/forecast?' + params;
}

export function parseForecast(data) {
  const h = data?.hourly;
  if (!h || !Array.isArray(h.time) || !Array.isArray(h.temperature_2m) ||
      !Array.isArray(h.precipitation_probability) || h.time.length < 24 ||
      h.time.length !== h.temperature_2m.length || h.time.length !== h.precipitation_probability.length ||
      data.hourly_units?.temperature_2m !== '°C' || data.hourly_units?.precipitation_probability !== '%')
    throw new Error('Invalid forecast');
  new Intl.DateTimeFormat('en-US', { timeZone: data.timezone }).format(0);
  if (typeof data.timezone !== 'string') throw new Error('Missing timezone');
  const hours = h.time.map((t, i) => {
    const temperature = h.temperature_2m[i], precipitation = h.precipitation_probability[i];
    if (!finite(t) || !Number.isFinite(new Date(t * 1000).getTime()) ||
        (i && t - h.time[i - 1] !== 3600) ||
        (temperature !== null && (!finite(temperature) || Math.abs(temperature) > 100)) ||
        (precipitation !== null && (!finite(precipitation) || precipitation < 0 || precipitation > 100)))
      throw new Error('Invalid hourly data');
    return { time: t * 1000, temperature, precipitation };
  });
  return { hours, timezone: data.timezone };
}

export function selectHours(sample, utc) {
  const start = Math.floor(utc / HOUR) * HOUR;
  return sample?.hours.filter(h => h.time >= start && h.time < start + 24 * HOUR) ?? [];
}

export class ForecastClient {
  constructor(location, { fetchData = (...args) => fetch(...args), now = () => performance.now() } = {}) {
    this.url = forecastUrl(location);
    this.fetchData = fetchData; this.now = now;
    this.sample = null; this.lastError = null; this.pending = false; this.timer = null;
  }
  start() {
    if (this.timer !== null || !this.url) return;
    void this.refresh();
    this.timer = setInterval(() => void this.refresh(), 15 * 60_000);
  }
  stop() { clearInterval(this.timer); this.timer = null; }
  async refresh() {
    if (!this.url || this.pending) return false;
    this.pending = true;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 8_000);
    try {
      const response = await this.fetchData(this.url, { signal: controller.signal, cache: 'no-store' });
      if (!response.ok) throw new Error('Forecast request failed');
      const parsed = parseForecast(await response.json());
      if (parsed.hours.filter(h => h.temperature !== null).length < 24) throw new Error('Incomplete forecast');
      this.sample = { ...parsed, receivedAt: this.now() }; this.lastError = null;
      return true;
    } catch (error) { this.lastError = error; return false; }
    finally { clearTimeout(timeout); this.pending = false; }
  }
}

export function chartMarkup(hours, unit, timezone) {
  const temp = h => h.temperature === null ? null : unit === 'F' ? h.temperature * 1.8 + 32 : h.temperature;
  const values = hours.map(temp).filter(finite);
  if (!values.length) return '';
  const low = Math.floor((Math.min(...values) - 2) / 5) * 5;
  const high = Math.max(low + 10, Math.ceil((Math.max(...values) + 2) / 5) * 5);
  const x = i => 58 + i * 864 / 23, y = t => 206 - (t - low) / (high - low) * 174;
  const label = new Intl.DateTimeFormat('en-US', { hour: 'numeric', timeZone: timezone });
  let svg = '', pen = false, path = '';
  for (let i = 0; i <= 2; i++) {
    const t = low + (high - low) * i / 2;
    svg += `<line class="forecast-grid" x1="58" x2="922" y1="${y(t)}" y2="${y(t)}"/><text x="44" y="${y(t) + 6}" text-anchor="end">${Math.round(t)}°</text>`;
  }
  hours.forEach((h, i) => {
    const t = temp(h);
    if (t === null) pen = false;
    else { path += `${pen ? 'L' : 'M'}${x(i)},${y(t)} `; pen = true; }
    if (h.precipitation > 0) svg += `<rect class="forecast-rain" x="${x(i) - 7}" y="${238 - h.precipitation * .24}" width="14" height="${h.precipitation * .24}" rx="3"/>`;
    if (i % 3 === 0) {
      svg += `<text x="${x(i)}" y="270" text-anchor="middle">${label.format(h.time)}</text>`;
      if (t !== null) svg += `<text class="forecast-value" x="${x(i)}" y="${y(t) - 12}" text-anchor="middle">${Math.round(t)}°</text>`;
    }
  });
  return svg + `<path class="forecast-line" d="${path}"/>`;
}

export function initializeForecast(root, client, timeClient, presentation) {
  const status = root.getElementById('forecast-status'), graph = root.getElementById('forecast-graph');
  const title = root.getElementById('forecast-title');
  let previous = '';
  const render = () => {
    const clock = timeClient.sample;
    const utc = clock ? clock.utcMilliseconds + timeClient.now() - clock.receivedAt : null;
    const hours = utc === null ? [] : selectHours(client.sample, utc);
    const usable = hours.length === 24 && hours.some(h => h.temperature !== null);
    const unit = presentation.read().temperatureUnit ?? 'F';
    status.textContent = !client.url ? 'Set the forecast location in config.js' : utc === null ? 'Waiting for synchronized time' :
      !client.sample ? (client.lastError ? 'Forecast unavailable · retrying automatically' : 'Loading forecast…') :
      !usable ? 'Forecast expired · waiting for an update' : client.lastError || client.now() - client.sample.receivedAt >= HOUR ?
      'Forecast update delayed · showing last available data' : '';
    title.textContent = `Next 24 hours · °${unit}`;
    const markup = usable ? chartMarkup(hours, unit, client.sample.timezone) : '';
    if (markup !== previous) { graph.innerHTML = markup; previous = markup; }
    graph.toggleAttribute('hidden', !usable);
    if (usable) graph.setAttribute('aria-label', `Hourly forecast in degrees ${unit}. ` + hours.map(h =>
      `${new Intl.DateTimeFormat('en-US', { hour: 'numeric', timeZone: client.sample.timezone }).format(h.time)}: ` +
      (h.temperature === null ? 'temperature unavailable' : `${Math.round(unit === 'F' ? h.temperature * 1.8 + 32 : h.temperature)} degrees`) +
      (h.precipitation === null ? ', precipitation chance unavailable' : `, ${h.precipitation}% precipitation chance`)).join('; '));
  };
  client.start(); render();
  const timer = setInterval(render, 1000);
  return () => { clearInterval(timer); client.stop(); };
}
