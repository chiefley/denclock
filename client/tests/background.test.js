import test from 'node:test';
import assert from 'node:assert/strict';
import { selectScene, BackgroundDisplay, initializeBackground, sceneImages } from '../background.js';
import { initializeBackgroundPreview } from '../background-preview.js';
import { parseSolar, forecastUrl } from '../forecast.js';

const instant = value => Date.parse(`2026-11-06T${value}:00Z`);
const forecast = { timezone: 'America/New_York', solar: [
  { date: '2026-11-06', sunrise: instant('12:00'), sunset: instant('22:00') },
] };
const civil = hour => new Date(`2026-11-06T${hour}:00Z`);

test('solar windows include +/-30 minutes; use absolute UTC independent of browser timezone', () => {
  for (const [utc, expected] of [
    ['11:29', 'night'], ['11:30', 'twilight'], ['12:00', 'twilight'],
    ['12:30', 'twilight'], ['12:31', 'day'], ['21:29', 'day'],
    ['21:30', 'twilight'], ['22:30', 'twilight'], ['22:31', 'night'],
  ]) assert.equal(selectScene(civil('09:00'), instant(utc), forecast), expected);
  // Nov 7 UTC is still Nov 6 at the forecast location.
  assert.equal(selectScene(civil('19:00'), Date.parse('2026-11-07T00:00Z'), forecast), 'night');
});
test('startup, unavailable/malformed solar data and old-date fallback use synchronized civil time', () => {
  assert.equal(selectScene(null, null, null), 'twilight');
  for (const sample of [null, { ...forecast, solar: [] },
    { ...forecast, solar: [{ ...forecast.solar[0], sunset: null }] },
    { ...forecast, solar: [{ ...forecast.solar[0], date: '2026-11-05' }] }]) {
    for (const [hour, scene] of [['05:59', 'night'], ['06:00', 'twilight'], ['07:00', 'day'],
      ['17:59', 'day'], ['18:00', 'twilight'], ['19:00', 'night']])
      assert.equal(selectScene(civil(hour), instant('16:00'), sample), scene);
  }
});
test('solar parsing uses daily date offset and leaves missing solar data optional', () => {
  assert.match(forecastUrl({ latitude: 41, longitude: -81 }), /sunrise%2Csunset/);
  const data = { utc_offset_seconds: -18000, daily: {
    time: [instant('05:00') / 1000], sunrise: [instant('12:00') / 1000], sunset: [instant('22:00') / 1000],
  } };
  assert.deepEqual(parseSolar(data), forecast.solar);
  assert.deepEqual(parseSolar({}), []);
  data.daily.sunrise[0] = null; assert.deepEqual(parseSolar(data), []);
});
const element = () => ({ style: {}, getAnimations: () => [] });
const settle = () => new Promise(resolve => setImmediate(resolve));
test('preload before fade, keep opaque base, avoid repeats, and cache successful images', async () => {
  const base = element(), overlay = element();
  const loads = [], fades = [];
  let loaded, faded;
  const display = new BackgroundDisplay(base, overlay, {
    preload: url => { loads.push(url); return new Promise(resolve => loaded = resolve); },
    animate: target => { fades.push(target.style.backgroundImage); return new Promise(resolve => faded = resolve); },
  });
  const pending = display.show('day');
  assert.equal(overlay.style.backgroundImage, undefined);
  loaded(); await settle();
  assert.equal(base.style.backgroundImage, undefined, 'original CSS image stays underneath');
  assert.equal(fades.length, 1);
  faded(); await pending;
  assert.match(base.style.backgroundImage, /lodge-winter-day/);
  assert.equal(overlay.style.opacity, '0');
  await display.show('day'); assert.equal(loads.length, 1); assert.equal(fades.length, 1);
  const back = display.show('night'); loaded(); await settle(); faded(); await back;
  const again = display.show('day'); await settle(); faded(); await again;
  assert.equal(loads.length, 2);
});
test('rapid selections skip obsolete loads and image failure retains current scene and retries', async () => {
  const loads = new Map(), shown = [];
  const display = new BackgroundDisplay(element(), element(), {
    preload: url => new Promise((resolve, reject) => loads.set(url, { resolve, reject })),
    animate: async target => shown.push(target.style.backgroundImage),
  });
  const pending = display.show('day'); await display.show('night');
  loads.get(sceneImages.day).resolve(); await settle();
  loads.get(sceneImages.night).reject(Error('missing')); await pending;
  assert.equal(display.current, 'twilight'); assert.deepEqual(shown, []);
  const retry = display.show('night'); loads.get(sceneImages.night).resolve(); await retry;
  assert.equal(display.current, 'night'); assert.equal(shown.length, 1);
});
test('temporary GUI offers immediate session modes and Auto resumes periodic selection', async () => {
  const originalImage = globalThis.Image;
  globalThis.Image = class { decode() { return Promise.resolve(); } };
  let change, selected = 'auto';
  const select = { get value() { return selected; }, set value(v) { selected = v; },
    addEventListener: (_, fn) => change = fn };
  const base = element(), overlay = element();
  overlay.animate = (_, options) => {
    assert.equal(options.duration, 1000);
    return { finished: Promise.resolve() };
  };
  const root = { getElementById: id => id === 'background-base' ? base : id === 'background-next' ? overlay : select };
  let hour = '08:00';
  const time = { sample: null, read: () => ({ civilTime: civil(hour) }) };
  const controller = initializeBackground(root, time, { sample: null });
  try {
    initializeBackgroundPreview(root, controller); await settle();
    assert.match(base.style.backgroundImage, /winter-day/);
    for (const mode of ['night', 'twilight', 'day']) {
      select.value = mode; change(); await settle();
      assert.equal(base.style.backgroundImage, `url("${sceneImages[mode]}")`);
    }
    hour = '20:00'; select.value = 'auto'; change(); await settle();
    assert.match(base.style.backgroundImage, /winter-night/);
    hour = '06:15'; await new Promise(resolve => setTimeout(resolve, 1100));
    assert.match(base.style.backgroundImage, /cabin-master/);
  } finally { controller.dispose(); globalThis.Image = originalImage; }
});

test('startup waits safely for synchronized time and reevaluates changed solar data', async () => {
  const originalImage = globalThis.Image;
  globalThis.Image = class { decode() { return Promise.resolve(); } };
  const base = element(), overlay = element();
  overlay.animate = () => ({ finished: Promise.resolve() });
  const root = { getElementById: id => id === 'background-base' ? base : overlay };
  const time = { sample: null, now: () => 0, read: () => ({ civilTime: time.sample ? civil('08:00') : null }) };
  const weather = { sample: null };
  const controller = initializeBackground(root, time, weather);
  try {
    await settle(); assert.equal(base.style.backgroundImage, undefined, 'original twilight CSS remains');
    time.sample = { utcMilliseconds: instant('13:00'), receivedAt: 0 };
    await new Promise(resolve => setTimeout(resolve, 1100));
    assert.match(base.style.backgroundImage, /winter-day/);
    weather.sample = { ...forecast, solar: [{ ...forecast.solar[0], sunrise: instant('13:00') }] };
    await new Promise(resolve => setTimeout(resolve, 1100));
    assert.match(base.style.backgroundImage, /cabin-master/);
  } finally { controller.dispose(); globalThis.Image = originalImage; }
});

test('selection during an active transition is serialized and ends on the newest scene', async () => {
  const fades = [];
  const display = new BackgroundDisplay(element(), element(), {
    preload: async () => {},
    animate: () => new Promise(resolve => fades.push(resolve)),
  });
  const pending = display.show('day'); await settle();
  await display.show('night'); await display.show('twilight');
  assert.equal(fades.length, 1);
  fades[0](); await settle();
  assert.equal(display.current, 'day'); assert.equal(fades.length, 2);
  fades[1](); await pending;
  assert.equal(display.current, 'twilight');
});
