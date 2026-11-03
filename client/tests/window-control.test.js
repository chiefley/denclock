import test from 'node:test';
import assert from 'node:assert/strict';
import { initializeWindowControl } from '../window-control.js';

function fixture(hostname = 'localhost') {
    const button = { hidden: true, disabled: false, attributes: {}, classes: new Set(),
        setAttribute(k, v) { this.attributes[k] = v; },
        classList: { toggle(k, v) { v ? button.classes.add(k) : button.classes.delete(k); } },
        addEventListener(_, fn) { this.click = fn; } };
    const status = { textContent: '' };
    const pending = [], calls = [], timers = [];
    initializeWindowControl({ getElementById: id => id === 'window-button' ? button : status }, {
        hostname, schedule: fn => timers.push(fn),
        fetcher: (url, options) => { calls.push({ url, options }); return new Promise((resolve, reject) => pending.push({ resolve, reject })); },
    });
    return { button, status, calls, pending, timers };
}
const settle = () => new Promise(resolve => setImmediate(resolve));
const reply = state => ({ ok: true, json: async () => ({ fullscreen: state }) });

test('remote pages never contact or expose local desktop control', () => {
    const f = fixture('bensclock');
    assert.equal(f.button.hidden, true);
    assert.equal(f.calls.length, 0);
});

test('actual state drives icons; clicks serialize; desktop changes are observed', async () => {
    const f = fixture();
    assert.equal(f.button.disabled, true);
    f.pending.shift().resolve(reply(true));
    await settle();
    assert.equal(f.button.attributes['aria-label'], 'Restore window');
    assert.equal(f.button.classes.has('is-fullscreen'), true);
    f.button.click();
    f.button.click();
    assert.equal(f.calls.length, 2);
    assert.deepEqual(JSON.parse(f.calls[1].options.body), { fullscreen: false });
    assert.equal(f.button.disabled, true);
    f.pending.shift().resolve(reply(false));
    await settle();
    assert.equal(f.button.attributes['aria-label'], 'Full screen');
    assert.equal(f.button.classes.has('is-fullscreen'), false);
    const polling = f.timers.shift()();
    f.pending.shift().resolve(reply(true));
    await polling;
    assert.equal(f.button.attributes['aria-label'], 'Restore window');
});

test('unavailable helper disables the button and polling recovers', async () => {
    const f = fixture();
    f.pending.shift().reject(new Error('offline'));
    await settle();
    assert.equal(f.button.disabled, true);
    assert.match(f.status.textContent, /unavailable/);
    const polling = f.timers.shift()();
    f.pending.shift().resolve(reply(false));
    await polling;
    assert.equal(f.status.textContent, '');
    assert.equal(f.button.disabled, false);
    f.button.click();
    f.pending.shift().resolve({ ok: true, json: async () => ({ fullscreen: 'bad' }) });
    await settle();
    assert.equal(f.button.disabled, true);
    assert.match(f.status.textContent, /unavailable/);
});
