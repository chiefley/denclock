import test from 'node:test';
import assert from 'node:assert/strict';
import { PresentationClient, SensorDisplay, formatTemperature, validateSnapshot } from '../presentation.js';
const base = Date.parse('2026-09-22T12:00:00Z');
const config = { serverAddress: 'http://test/', presentationIntervalMs: 10, requestTimeoutMs: 20 };
function snapshot({ age = 0, temperatureC = 22.15, relativeHumidity = 52, unit = 'F' } = {}) {
  return {
    serverUtc: new Date(base).toISOString(), temperatureUnit: unit,
    slots: [
      { position: 'upperLeft', internalName: null, displayName: '', maxAgeSeconds: null, observation: null },
      { position: 'upperRight', internalName: 'porch', displayName: 'Outside', maxAgeSeconds: 60,
        observation: { receivedUtc: new Date(base - age * 1000).toISOString(), temperatureC, relativeHumidity } },
    ],
  };
}
const response = data => ({ ok: true, json: async () => data });
test('unit conversion, signed fractions, carry, rounding, and unknown values', () => {
  for (const [value, unit, expected] of [[0,'F','32.0'],[22.15,'F','71.9'],[-40,'F','−40.0'],
    [-0.15,'C','−0.2'],[-0.01,'C','0.0'],[99.96,'C','100.0'],[-9.96,'C','−10.0'],
    [22.15,'C','22.2'],[null,'F','--.-']]) {
    const actual = formatTemperature(value, unit);
    assert.equal(actual.whole + actual.fraction, expected);
  }
});
test('snapshot validation rejects malformed and ambiguous API data', () => {
  assert.equal(validateSnapshot(snapshot()).slots[1].observation.temperatureC, 22.15);
  const mutations = [
    d => d.temperatureUnit = 'f',
    d => d.serverUtc = 'invalid',
    d => d.slots.pop(),
    d => d.slots[1].position = 'upperLeft',
    d => d.slots[1].maxAgeSeconds = 0,
    d => d.slots[1].observation.temperatureC = '22',
    d => d.slots[1].observation.relativeHumidity = 101,
    d => d.slots[1].observation.receivedUtc = '2026-09-23T12:00:00Z',
    d => d.slots[0].observation = d.slots[1].observation,
  ];
  for (const mutate of mutations) { const data = snapshot(); mutate(data); assert.throws(() => validateSnapshot(data)); }
});
test('monotonic freshness survives API outages and retained/duplicate replies', async () => {
  let now = 0, data = snapshot({ age: 59 }), fail = false;
  const client = new PresentationClient(config, { now: () => now, fetchSnapshot: async (url, options) => {
    assert.equal(url, 'http://test/api/presentation'); assert.equal(options.cache, 'no-store');
    if (fail) throw Error('offline');
    return response(data);
  }});
  assert.equal(client.read().slots[1].observation, null);
  await client.synchronize();
  assert.equal(client.read().slots[1].stale, false);
  now = 1000;
  assert.equal(client.read().slots[1].stale, true);
  fail = true;
  await client.synchronize();
  now = 60000;
  assert.equal(client.read().slots[1].stale, true);
  fail = false;
  data = snapshot({ age: 120 });
  await client.synchronize();
  assert.equal(client.read().slots[1].stale, true);
  data = snapshot({ age: 0 });
  await client.synchronize();
  assert.equal(client.read().slots[1].stale, false);
});
test('wall clock changes do not affect age; malformed snapshots preserve state', async () => {
  let now = 0, data = snapshot();
  const client = new PresentationClient(config, { now: () => now, fetchSnapshot: async () => response(data) });
  await client.synchronize();
  const original = Date.now;
  try {
    Date.now = () => 0;
    now = 60000;
    assert.equal(client.read().slots[1].stale, true);
    data = {};
    assert.equal(await client.synchronize(), false);
    assert.equal(client.read().slots[1].observation.temperatureC, 22.15);
  } finally { Date.now = original; }
});
test('hung requests time out, never overlap, and recover', async () => {
  let hung = true, calls = 0;
  const client = new PresentationClient(config, { fetchSnapshot: async (_, { signal }) => {
    calls++;
    if (!hung) return response(snapshot());
    return new Promise((_, reject) => signal.addEventListener('abort', () => reject(Error('timeout'))));
  }});
  const pending = client.synchronize();
  assert.equal(await client.synchronize(), false);
  assert.equal(await pending, false);
  assert.equal(calls, 1);
  hung = false;
  assert.equal(await client.synchronize(), true);
});
test('one immediate and periodic polling loop', async () => {
  let calls = 0;
  const client = new PresentationClient(config, { fetchSnapshot: async () => { calls++; return response(snapshot()); } });
  client.start();
  const timer = client.timer; client.start();
  assert.equal(timer, client.timer);
  await new Promise(resolve => setTimeout(resolve, 36));
  client.stop();
  assert.ok(calls >= 2);
});
test('DOM rendering uses unknown, fractional digits, labels, and stale recovery', async () => {
  let now = 0;
  const client = new PresentationClient(config, { now: () => now, fetchSnapshot: async () => response(snapshot()) });
  const panels = new Map(['upperLeft','upperRight'].map(id => [id, {
    elements: new Map(), stale: false, attributes: {},
    querySelector(key) { if (!this.elements.has(key)) this.elements.set(key, { textContent: '' }); return this.elements.get(key); },
    classList: { toggle(key, value) { panels.get(id).stale = value; } },
    setAttribute(key, value) { this.attributes[key] = value; },
  }]));
  const display = new SensorDisplay(client, { getElementById: id => panels.get(id) });
  display.render();
  assert.equal(panels.get('upperLeft').querySelector('.temperature-whole').textContent, '--');
  await client.synchronize(); display.render();
  const panel = panels.get('upperRight');
  assert.equal(panel.querySelector('.temperature-whole').textContent, '71');
  assert.equal(panel.querySelector('.temperature-fraction').textContent, '.9');
  assert.equal(panel.querySelector('.sensor-name').textContent, 'Outside');
  assert.equal(panel.querySelector('.humidity-value').textContent, '52%');
  now = 60000; display.render(); assert.equal(panel.stale, true);
  await client.synchronize(); display.render(); assert.equal(panel.stale, false);
});

test('post-save refresh waits for an older poll and then fetches updated presentation', async () => {
  let release, calls = 0;
  const client = new PresentationClient({ ...config, requestTimeoutMs: 1000 }, { fetchSnapshot: async () => {
    calls++;
    if (calls === 1) return new Promise(resolve => release = resolve);
    return response(snapshot({ unit: 'C' }));
  }});
  const pending = client.synchronize();
  const refreshed = client.refresh();
  assert.equal(calls, 1);
  release(response(snapshot({ unit: 'F' })));
  await pending; await refreshed;
  assert.equal(calls, 2);
  assert.equal(client.read().temperatureUnit, 'C');
});
