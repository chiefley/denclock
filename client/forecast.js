import { initializeRadar } from './radar.js';

const HOUR = 3_600_000;
const finite = value => typeof value === 'number' && Number.isFinite(value);

export function forecastUrl(location) {
  if (!location || !finite(location.latitude) || Math.abs(location.latitude) > 90 ||
      !finite(location.longitude) || Math.abs(location.longitude) > 180) return null;
  // Unix timestamps keep the hourly sequence unambiguous across DST changes.
  const params = new URLSearchParams({ latitude: location.latitude, longitude: location.longitude,
    hourly: 'temperature_2m,precipitation_probability',
    daily: 'temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset', forecast_days: 10,
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
  return { hours, days: parseDays(data), solar: parseSolar(data), timezone: data.timezone };
}

// Solar instants are Unix seconds; daily date keys use the provider UTC offset.
// Missing solar data must never invalidate the existing weather response.
export function parseSolar(data) {
  const d = data.daily;
  if (!Array.isArray(d?.time) || !finite(data.utc_offset_seconds)
      || Math.abs(data.utc_offset_seconds) > 86400) return [];
  return d.time.flatMap((time, i) => {
    const sunrise = d.sunrise?.[i], sunset = d.sunset?.[i];
    const date = new Date((time + data.utc_offset_seconds) * 1000);
    if (!finite(time) || !Number.isFinite(date.getTime()) || !finite(sunrise) || !finite(sunset)
        || sunset <= sunrise || !Number.isFinite(new Date(sunrise * 1000).getTime())
        || !Number.isFinite(new Date(sunset * 1000).getTime())) return [];
    return [{ date: date.toISOString().slice(0, 10), sunrise: sunrise * 1000, sunset: sunset * 1000 }];
  });
}

// Daily Unix timestamps use the response's UTC offset to identify calendar dates,
// as specified by Open-Meteo. Keep dates separate from elapsed hourly timestamps.
function parseDays(data) {
  const d = data.daily, units = data.daily_units;
  if (!Array.isArray(d?.time) || !finite(data.utc_offset_seconds) ||
      Math.abs(data.utc_offset_seconds) > 86400 ||
      units?.temperature_2m_max !== '°C' || units?.temperature_2m_min !== '°C') return [];
  const temperature = value => finite(value) && Math.abs(value) <= 100 ? value : null;
  const days = new Map();
  d.time.forEach((time, i) => {
    if (!finite(time)) return;
    const date = new Date((time + data.utc_offset_seconds) * 1000);
    if (!Number.isFinite(date.getTime())) return;
    const key = date.toISOString().slice(0, 10);
    let high = temperature(d.temperature_2m_max?.[i]), low = temperature(d.temperature_2m_min?.[i]);
    if (high !== null && low !== null && high < low) high = low = null;
    const rain = d.precipitation_probability_max?.[i];
    const precipitation = units.precipitation_probability_max === '%' && finite(rain) && rain >= 0 && rain <= 100 ? rain : null;
    days.set(key, { date: key, high, low, precipitation });
  });
  return [...days.values()];
}

function localDate(utc, timezone) {
  const parts = new Intl.DateTimeFormat('en-US', {
    timeZone: timezone, year: 'numeric', month: '2-digit', day: '2-digit',
  }).formatToParts(utc);
  const part = type => parts.find(p => p.type === type).value;
  return `${part('year')}-${part('month')}-${part('day')}`;
}

export function selectDays(sample, utc) {
  if (!sample) return [];
  const today = Date.parse(localDate(utc, sample.timezone) + 'T00:00:00Z');
  const days = new Map((sample.days ?? []).map(day => [day.date, day]));
  // Empty calendar slots preserve gaps; no missing forecast values are invented.
  return Array.from({ length: 10 }, (_, i) => {
    const date = new Date(today + i * 24 * HOUR).toISOString().slice(0, 10);
    return days.get(date) ?? { date, high: null, low: null, precipitation: null };
  });
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
  setLocation(location) {
    const url = forecastUrl(location);
    if (!url || url === this.url) { if (url) this.location = location; return; }
    this.location = location;
    this.url = url;
    this.sample = null; this.lastError = null;
    // Finish any old request before starting the newly configured location.
    if (!this.pending) void this.refresh();
  }
  start() {
    if (this.timer !== null) return;
    void this.refresh();
    this.timer = setInterval(() => void this.refresh(), 15 * 60_000);
  }
  stop() { clearInterval(this.timer); this.timer = null; }
  async refresh() {
    if (!this.url || this.pending) return false;
    this.pending = true;
    const requestedUrl = this.url;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 8_000);
    try {
      const response = await this.fetchData(requestedUrl, { signal: controller.signal, cache: 'no-store' });
      if (!response.ok) throw new Error('Forecast request failed');
      const parsed = parseForecast(await response.json());
      if (parsed.hours.filter(h => h.temperature !== null).length < 24) throw new Error('Incomplete forecast');
      if (requestedUrl !== this.url) return false;
      this.sample = { ...parsed, receivedAt: this.now() }; this.lastError = null;
      return true;
    } catch (error) { if (requestedUrl === this.url) this.lastError = error; return false; }
    finally { clearTimeout(timeout); this.pending = false; if (requestedUrl !== this.url) void this.refresh(); }
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
    svg += `<line class="forecast-grid" x1="58" x2="922" y1="${y(t)}" y2="${y(t)}"/>`;
  }
  hours.forEach((h, i) => {
    const t = temp(h);
    if (t === null) pen = false;
    else { path += `${pen ? 'L' : 'M'}${x(i)},${y(t)} `; pen = true; }
    if (h.precipitation > 0) svg += `<rect class="forecast-rain" x="${x(i) - 7}" y="${238 - h.precipitation * .24}" width="14" height="${h.precipitation * .24}" rx="3"/>`;
    if (i % 3 === 0) {
      const timeLabel = label.formatToParts(h.time).map(part => part.type === 'dayPeriod'
        ? `<tspan class="forecast-day-period">${part.value}</tspan>` : part.value).join('');
      svg += `<text x="${x(i)}" y="270" text-anchor="middle">${timeLabel}</text>`;
      if (t !== null) svg += `<text class="forecast-value" x="${x(i)}" y="${y(t) - 12}" text-anchor="middle">${Math.round(t)}°</text>`;
    }
  });
  return svg + `<path class="forecast-line" d="${path}"/>`;
}

export function dailyChartMarkup(days, unit) {
  const temp = value => value === null ? null : unit === 'F' ? value * 1.8 + 32 : value;
  const values = days.flatMap(day => [temp(day.high), temp(day.low)]).filter(finite);
  if (!values.length) return '';
  const min = Math.min(...values), max = Math.max(...values);
  const padding = Math.max(3, (max - min) * .15);
  const low = min - padding, high = max + padding;
  const x = i => 58 + i * 864 / 9;
  const y = t => 194 - (t - low) / (high - low) * 150;
  const weekday = new Intl.DateTimeFormat('en-US', { weekday: 'short', timeZone: 'UTC' });
  let svg = '';
  for (const series of ['high', 'low']) {
    let path = '', pen = false;
    days.forEach((day, i) => {
      const t = temp(day[series]);
      if (t === null) { pen = false; return; }
      path += `${pen ? 'L' : 'M'}${x(i)},${y(t)} `; pen = true;
      svg += `<circle class="forecast-point forecast-${series}" cx="${x(i)}" cy="${y(t)}" r="2.5"/>`;
      svg += `<text class="forecast-value forecast-${series}" x="${x(i)}" y="${y(t) + (series === 'high' ? -12 : 24)}" text-anchor="middle">${Math.round(t)}°</text>`;
    });
    svg += `<path class="forecast-line forecast-${series}" d="${path}"/>`;
  }
  days.forEach((day, i) => {
    if (day.precipitation !== null) svg += `<text class="forecast-probability" x="${x(i)}" y="242" text-anchor="middle">${Math.round(day.precipitation)}%</text>`;
    svg += `<text x="${x(i)}" y="270" text-anchor="middle">${weekday.format(new Date(day.date + 'T00:00:00Z'))}</text>`;
  });
  return svg;
}

export function initializeForecast(root, client, timeClient, presentation, radar = initializeRadar(root)) {
  const status = root.getElementById('forecast-status'), graph = root.getElementById('forecast-graph');
  const title = root.getElementById('forecast-title'), area = root.getElementById('forecast-area');
  const key = root.getElementById('forecast-key');
  const radarArea = root.getElementById('radar-view');
  const weatherCredit = root.getElementById('forecast-weather-credit'), radarCredit = root.getElementById('forecast-radar-credit');
  let previous = '', view = 0;
  const render = () => {
    const daily = view === 1, radarVisible = view === 2;
    client.setLocation(presentation.read().forecastLocation);
    const clock = timeClient.sample;
    const utc = clock ? clock.utcMilliseconds + timeClient.now() - clock.receivedAt : null;
    const hours = utc === null ? [] : selectHours(client.sample, utc);
    const days = utc === null ? [] : selectDays(client.sample, utc);
    const usable = daily ? days.some(day => day.high !== null || day.low !== null) :
      hours.length === 24 && hours.some(h => h.temperature !== null);
    const incomplete = daily && usable && days.some(day => day.high === null || day.low === null);
    const unit = presentation.read().temperatureUnit ?? 'F';
    status.toggleAttribute('hidden', radarVisible);
    radarArea?.toggleAttribute('hidden', !radarVisible);
    weatherCredit?.toggleAttribute('hidden', radarVisible);
    radarCredit?.toggleAttribute('hidden', !radarVisible);
    radar?.update({ visible: radarVisible, location: presentation.read().forecastLocation, utc, timezone: client.sample?.timezone });
    status.textContent = !client.url ? 'Waiting for forecast settings' : utc === null ? 'Waiting for synchronized time' :
      !client.sample ? (client.lastError ? 'Forecast unavailable · retrying automatically' : 'Loading forecast…') :
      !usable ? (daily ? 'Daily forecast unavailable · waiting for an update' : 'Forecast expired · waiting for an update') :
      client.lastError || client.now() - client.sample.receivedAt >= HOUR ?
      'Forecast update delayed · showing last available data' : incomplete ? 'Some daily forecasts unavailable' : '';
    const locationPrefix = client.location?.displayName ? `${client.location.displayName} · ` : '';
    title.textContent = radarVisible ? `${locationPrefix}Weather radar` : `${locationPrefix}${daily ? 'Next 10 days' : 'Next 24 hours'} · °${unit}`;
    key.textContent = radarVisible ? 'Past two hours' : daily ? 'High / low · rain chance' : 'Blue bars · precipitation chance';
    area.setAttribute('aria-label', `${['Hourly forecast', 'Ten-day forecast', 'Weather radar'][view]}. Activate to show ${['ten-day forecast', 'weather radar', 'hourly forecast'][view]}.`);
    const markup = usable ? daily ? dailyChartMarkup(days, unit) : chartMarkup(hours, unit, client.sample.timezone) : '';
    if (markup !== previous) { graph.innerHTML = markup; previous = markup; }
    graph.toggleAttribute('hidden', radarVisible || !usable);
    const degrees = value => value === null ? 'unavailable' : `${Math.round(unit === 'F' ? value * 1.8 + 32 : value)} degrees`;
    if (usable) graph.setAttribute('aria-label', daily ? `Daily forecast in degrees ${unit}. ` + days.map(day =>
      `${day.date}: high ${degrees(day.high)}, low ${degrees(day.low)}` +
      (day.precipitation === null ? ', precipitation chance unavailable' : `, ${day.precipitation}% precipitation chance`)).join('; ') :
      `Hourly forecast in degrees ${unit}. ` + hours.map(h =>
      `${new Intl.DateTimeFormat('en-US', { hour: 'numeric', timeZone: client.sample.timezone }).format(h.time)}: ` +
      (h.temperature === null ? 'temperature unavailable' : degrees(h.temperature)) +
      (h.precipitation === null ? ', precipitation chance unavailable' : `, ${h.precipitation}% precipitation chance`)).join('; '));
  };
  const toggle = () => { view = (view + 1) % 3; render(); };
  const click = event => { if (!event.target.closest('a, button')) toggle(); };
  const keydown = event => {
    if (event.target === area && (event.key === 'Enter' || event.key === ' ')) {
      event.preventDefault(); toggle();
    }
  };
  area.addEventListener('click', click);
  area.addEventListener('keydown', keydown);
  client.start(); render();
  const timer = setInterval(render, 1000);
  return () => {
    clearInterval(timer); client.stop(); radar?.dispose();
    area.removeEventListener('click', click); area.removeEventListener('keydown', keydown);
  };
}

// Validate before PUT so failed lookups or forecasts leave persisted settings intact.
export async function resolveForecastLocation(query, fetchData = (...args) => fetch(...args)) {
  query = query.trim();
  if (!/^\d{5}$/.test(query) && !/^[^,]+,\s*[^,]+$/.test(query))
    throw new Error('Enter a city, state or a five-digit US ZIP code.');
  if (query.length > 200) throw new Error('Forecast location is too long.');
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 8_000);
  try {
    const response = await fetchData('https://geocoding-api.open-meteo.com/v1/search?' +
      new URLSearchParams({ name: query, count: 10, language: 'en', countryCode: 'US' }),
      { signal: controller.signal, cache: 'no-store' });
    if (!response.ok) throw new Error('Location service unavailable. Previous settings remain saved.');
    const results = (await response.json()).results;
    if (!results?.length) throw new Error('Location not found. Check the city/state or ZIP code.');
    const candidates = /^\d{5}$/.test(query) ? results : results.filter(place =>
      place.name?.localeCompare(query.split(',')[0].trim(), 'en', { sensitivity: 'base' }) === 0);
    if (!candidates.length) throw new Error('Location not found. Enter the full city name and state.');
    const place = candidates[0];
    const location = { query, displayName: `${place.name}, ${place.admin1}`, latitude: place.latitude, longitude: place.longitude };
    if (!forecastUrl(location) || !place.name || !place.admin1) throw new Error('Invalid location service response.');
    const forecast = new ForecastClient(location, { fetchData });
    if (!await forecast.refresh()) throw new Error('Forecast unavailable for this location. Previous settings remain saved; try again later.');
    return location;
  } finally { clearTimeout(timeout); }
}
