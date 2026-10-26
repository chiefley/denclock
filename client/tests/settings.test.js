import test from 'node:test';
import assert from 'node:assert/strict';
import { buildConfiguration, initializeSettings } from '../settings.js';

const original = {
  temperatureUnit: 'F', future: { preserved: true }, sensors: [
    { internalName: 'den', displayName: 'Den', position: 'upperLeft', maxAgeSeconds: 60 },
    { internalName: 'porch', displayName: 'Outside', position: 'upperRight', maxAgeSeconds: 120 },
    { internalName: 'attic', displayName: 'Attic', position: 'secondary', maxAgeSeconds: 300, extra: 42 },
  ],
};
const slots = () => [
  { position: 'upperLeft', internalName: 'den', displayName: 'Living room', maxAgeSeconds: '30' },
  { position: 'upperRight', internalName: 'porch', displayName: 'Outside', maxAgeSeconds: '120' },
];
test('primary edits preserve secondary sensors and unknown fields without mutating loaded settings', () => {
  const copy = structuredClone(original);
  const changed = slots(); changed[0].internalName = 'attic';
  const result = buildConfiguration(original, 'C', changed);
  assert.deepEqual(original, copy);
  assert.equal(result.temperatureUnit, 'C');
  assert.deepEqual(result.future, { preserved: true });
  assert.deepEqual(result.sensors[0], { ...original.sensors[0], position: 'secondary' });
  assert.equal(result.sensors[2].position, 'upperLeft');
  assert.equal(result.sensors[2].extra, 42);
  changed[0].internalName = '';
  const unassigned = buildConfiguration(original, 'F', changed);
  assert.equal(unassigned.sensors[0].position, 'secondary');
  assert.deepEqual(unassigned.sensors[2], original.sensors[2]);
});
test('duplicate assignments, invalid ages, empty labels, and unknown identities are rejected', () => {
  for (const mutate of [
    s => s[0].internalName = 'porch', s => s[0].internalName = 'unknown',
    s => s[0].displayName = ' ', s => s[0].maxAgeSeconds = '',
    s => s[0].maxAgeSeconds = 0, s => s[0].maxAgeSeconds = -1,
    s => s[0].maxAgeSeconds = 'Infinity', s => s[0].maxAgeSeconds = 'bad',
  ]) { const draft = slots(); mutate(draft); assert.throws(() => buildConfiguration(original, 'F', draft)); }
});

// Exercise asynchronous UI lifecycle and request races independently of native
// dialog layout/focus containment, which still needs real-browser verification.
class Element {
  value = ''; textContent = ''; disabled = false; hidden = false; open = false;
  listeners = new Map(); options = [];
  addEventListener(type, callback) {
    if (!this.listeners.has(type)) this.listeners.set(type, []);
    this.listeners.get(type).push(callback);
  }
  async emit(type) {
    const event = { defaultPrevented: false, preventDefault() { this.defaultPrevented = true; } };
    await Promise.all((this.listeners.get(type) || []).map(callback => callback(event)));
    return event;
  }
  focus() { this.focused = true; }
  setAttribute() {}
  reset() {}
  replaceChildren(...options) { this.options = options; this.value = options[0]?.value ?? ''; }
  add(option) { this.options.push(option); }
  showModal() { this.open = true; }
  close() { this.open = false; void this.emit('close'); }
}
globalThis.Option = class { constructor(text, value) { this.text = text; this.value = value; } };
const config = { serverAddress: 'http://server/', requestTimeoutMs: 1000 };
const response = (status = 200, body = { revision: 'r1', configuration: structuredClone(original) }) => ({
  ok: status >= 200 && status < 300, status, json: async () => body,
});
const settle = () => new Promise(resolve => setImmediate(resolve));
function setup(fetch) {
  const ids = ['button', 'view', 'form', 'unit', 'save', 'cancel', 'status', 'reload',
    'left-sensor', 'left-label', 'left-age', 'right-sensor', 'right-label', 'right-age'];
  const elements = new Map(ids.map(id => ['settings-' + id, new Element()]));
  elements.get('settings-form').querySelectorAll = () => [new Element(), new Element()];
  let refreshed = 0;
  initializeSettings({ getElementById: id => elements.get(id) }, config, () => refreshed++, fetch);
  return { get: id => elements.get('settings-' + id), refreshed: () => refreshed };
}
test('fresh load each open, cancel discards drafts, and late loads cannot replace a new session', async () => {
  const loads = [];
  const ui = setup(() => new Promise(resolve => loads.push(resolve)));
  await ui.get('button').emit('click');
  assert.equal(ui.get('save').disabled, true);
  await ui.get('cancel').emit('click');
  assert.equal(ui.get('view').open, false);
  assert.equal(ui.get('button').focused, true);
  await ui.get('button').emit('click');
  loads[1](response()); await settle();
  assert.equal(ui.get('left-label').value, 'Den');
  ui.get('left-label').value = 'Draft'; await ui.get('left-label').emit('input');
  loads[0](response(200, { revision: 'old', configuration: { ...original, temperatureUnit: 'C' } }));
  await settle();
  assert.equal(ui.get('left-label').value, 'Draft');
  assert.equal(ui.get('unit').value, 'F');
  await ui.get('cancel').emit('click');
  await ui.get('button').emit('click');
  loads[2](response()); await settle();
  assert.equal(ui.get('left-label').value, 'Den');
  assert.equal(ui.get('left-sensor').options.length, 4, 'configured identities include secondary');
  ui.get('left-sensor').value = ''; await ui.get('left-sensor').emit('change');
  assert.equal(ui.get('left-label').disabled, true);
  assert.equal(ui.get('left-age').disabled, true);
});
test('load failure leaves Save disabled and provides explicit reload', async () => {
  let fail = true;
  const ui = setup(async () => fail ? response(500) : response());
  await ui.get('button').emit('click'); await settle();
  assert.equal(ui.get('save').disabled, true);
  assert.equal(ui.get('reload').hidden, false);
  assert.match(ui.get('status').textContent, /Could not load/);
  fail = false; await ui.get('reload').emit('click'); await settle();
  assert.equal(ui.get('save').disabled, false);
});
test('save blocks duplicate submits and closing, then restores focus and refreshes presentation', async () => {
  let complete, saves = 0;
  const ui = setup(async (url, options) => {
    assert.equal(url, 'http://server/api/settings/presentation');
    assert.equal(options.cache, 'no-store');
    if (!options.method) return response();
    saves++;
    assert.equal(options.method, 'PUT');
    assert.equal(options.headers['Content-Type'], 'application/json');
    const body = JSON.parse(options.body);
    assert.equal(body.revision, 'r1');
    assert.equal(body.configuration.sensors[0].displayName, 'Edited');
    assert.deepEqual(body.configuration.sensors[2], original.sensors[2]);
    return new Promise(resolve => complete = resolve);
  });
  await ui.get('button').emit('click'); await settle();
  ui.get('left-label').value = 'Edited';
  const pending = ui.get('form').emit('submit');
  assert.equal(ui.get('cancel').disabled, true);
  assert.equal((await ui.get('view').emit('cancel')).defaultPrevented, true);
  await ui.get('cancel').emit('click');
  await ui.get('form').emit('submit');
  assert.equal(saves, 1);
  assert.equal(ui.get('view').open, true);
  complete(response()); await pending;
  assert.equal(ui.get('view').open, false);
  assert.equal(ui.get('button').focused, true);
  assert.equal(ui.refreshed(), 1);
});
test('conflict and persistence errors preserve the draft until explicit reload', async () => {
  let code = 409, calls = 0;
  const ui = setup(async (_, options) => {
    calls++;
    return options.method ? response(code, { error: 'Disk write failed.' }) : response();
  });
  await ui.get('button').emit('click'); await settle();
  ui.get('left-label').value = 'Keep draft';
  await ui.get('form').emit('submit');
  assert.equal(ui.get('view').open, true);
  assert.equal(ui.get('left-label').value, 'Keep draft');
  assert.match(ui.get('status').textContent, /Another client/);
  assert.equal(ui.get('reload').hidden, false);
  assert.equal(calls, 2, 'no automatic reload');
  code = 500; await ui.get('form').emit('submit');
  assert.equal(ui.get('left-label').value, 'Keep draft');
  assert.equal(ui.get('status').textContent, 'Disk write failed.');
  assert.equal(ui.refreshed(), 0);
  await ui.get('reload').emit('click'); await settle();
  assert.equal(ui.get('left-label').value, 'Den');
});
