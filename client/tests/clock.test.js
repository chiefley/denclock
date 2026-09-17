import test from 'node:test';
import assert from 'node:assert/strict';
import { TimeClient, ClockDisplay, formatTime } from '../clock.js';
import { config } from '../config.js';

const timestamp = Date.UTC(2026, 8, 17, 12, 58, 32);
const payload = { unixTimeMilliseconds: timestamp, utcOffsetSeconds: -14400 };
const response = (data = payload) => ({ ok: true, json: async () => data });

test('startup has no time; synchronization compensates transit and advances monotonically', async () => {
  let now = 100;
  let calls = 0;
  const client = new TimeClient({ ...config, serverAddress: 'http://DenClock:5000/' }, {
    now: () => now,
    fetchTime: async (url, options) => {
      calls++;
      assert.equal(url, 'http://DenClock:5000/api/time');
      assert.equal(options.cache, 'no-store');
      now += 80;
      return response();
    },
  });
  assert.equal(client.read().civilTime, null);
  assert.equal(await client.synchronize(), true);
  assert.equal(client.read().civilTime.getTime(), timestamp - 14400000 + 40);
  now += 12500;
  assert.equal(client.read().civilTime.getTime(), timestamp - 14400000 + 12540);
  assert.equal(client.read().syncAgeMs, 12500);
  assert.equal(calls, 1);
  assert.deepEqual(formatTime(client.read().civilTime), {
    hoursMinutes: '8:58', seconds: '44', date: 'Thursday, September 17',
  });
});

test('failed sync preserves time, becomes stale, and recovers with a new offset', async () => {
  let now = 0;
  let failure = false;
  const client = new TimeClient(config, {
    now: () => now,
    fetchTime: async () => {
      if (failure) throw new Error('Offline');
      return response({ ...payload, unixTimeMilliseconds: timestamp + now, utcOffsetSeconds: now ? -18000 : -14400 });
    },
  });
  await client.synchronize();
  failure = true;
  now = 60000;
  assert.equal(await client.synchronize(), false);
  assert.equal(client.read().stale, false);
  now = 180000;
  assert.equal(client.read().stale, true);
  assert.equal(formatTime(client.read().civilTime).hoursMinutes, '9:01');
  failure = false;
  await client.synchronize();
  assert.equal(client.read().stale, false);
  assert.equal(client.read().syncAgeMs, 0);
  assert.equal(client.lastError, null);
  assert.equal(formatTime(client.read().civilTime).hoursMinutes, '8:01');
});

test('invalid responses never establish time or overwrite a good sample', async () => {
  const failures = [
    { ok: false, status: 503 },
    { ok: true, json: async () => { throw new SyntaxError('Bad JSON'); } },
    ...[null, {}, { ...payload, unixTimeMilliseconds: '123' },
      { ...payload, unixTimeMilliseconds: 1.5 },
      { ...payload, unixTimeMilliseconds: 8640000000000001 },
      { ...payload, utcOffsetSeconds: null },
      { ...payload, utcOffsetSeconds: 100000 },
    ].map(response),
  ];
  for (const invalid of failures) {
    let next = invalid;
    const client = new TimeClient(config, { now: () => 0, fetchTime: async () => next });
    assert.equal(await client.synchronize(), false);
    assert.equal(client.read().civilTime, null);
    next = response();
    await client.synchronize();
    const previous = client.read().civilTime.getTime();
    next = invalid;
    assert.equal(await client.synchronize(), false);
    assert.equal(client.read().civilTime.getTime(), previous);
  }
});

test('hung requests time out, do not overlap, and permit retry', async () => {
  let calls = 0;
  const client = new TimeClient({ ...config, requestTimeoutMs: 10 }, {
    fetchTime: (url, { signal }) => {
      calls++;
      return new Promise((resolve, reject) => {
        signal.addEventListener('abort', () => reject(new Error('Aborted')), { once: true });
      });
    },
  });
  const pending = client.synchronize();
  assert.equal(await client.synchronize(), false);
  assert.equal(calls, 1);
  assert.equal(await pending, false);
  assert.equal(client.pending, false);
  client.fetchTime = async () => response();
  assert.equal(await client.synchronize(), true);
});

test('start synchronizes immediately and periodically without duplicating the timer', async () => {
  let calls = 0;
  const client = new TimeClient({ ...config, syncIntervalMs: 10 }, {
    fetchTime: async () => { calls++; return response(); },
  });
  client.start();
  const timer = client.timer;
  try {
    client.start();
    assert.equal(client.timer, timer);
    assert.equal(calls, 1);
    await new Promise(resolve => setTimeout(resolve, 35));
    assert.ok(calls >= 2);
  } finally {
    clearInterval(timer);
  }
});

test('formatting handles noon, midnight, date rollover, and ignores host timezone/wall clock', async () => {
  const originalTimezone = process.env.TZ;
  const originalNow = Date.now;
  try {
    Date.now = () => { throw new Error('Host wall clock must not be read'); };
    for (const zone of ['America/Los_Angeles', 'Asia/Tokyo', 'UTC']) {
      process.env.TZ = zone;
      for (const [utc, offset, time, date] of [
        [Date.UTC(2026, 8, 17, 16), -14400, '12:00', 'Thursday, September 17'],
        [Date.UTC(2026, 8, 18, 4), -14400, '12:00', 'Friday, September 18'],
        [Date.UTC(2026, 8, 17, 20), 19800, '1:30', 'Friday, September 18'],
      ]) {
        const client = new TimeClient(config, {
          now: () => 0,
          fetchTime: async () => response({ unixTimeMilliseconds: utc, utcOffsetSeconds: offset }),
        });
        await client.synchronize();
        assert.deepEqual(formatTime(client.read().civilTime), { hoursMinutes: time, seconds: '00', date });
      }
    }
  } finally {
    Date.now = originalNow;
    if (originalTimezone === undefined) delete process.env.TZ;
    else process.env.TZ = originalTimezone;
  }
});

test('display shows placeholders, date rollover, stale indication, and recovery', async () => {
  let now = 0;
  const client = new TimeClient(config, {
    now: () => now,
    fetchTime: async () => response({ unixTimeMilliseconds: Date.UTC(2026, 8, 18, 3, 59, 59) + now, utcOffsetSeconds: -14400 }),
  });
  const elements = Object.fromEntries(['hours-minutes', 'seconds', 'date', 'sync-status'].map(id => [id, { textContent: '' }]));
  const display = new ClockDisplay(client, { getElementById: id => elements[id] });
  display.render();
  assert.equal(elements['hours-minutes'].textContent, '--:--');
  assert.equal(elements['sync-status'].textContent, 'Waiting for time');
  await client.synchronize();
  now = 1000;
  display.render();
  assert.equal(elements['hours-minutes'].textContent, '12:00');
  assert.equal(elements.seconds.textContent, '00');
  assert.equal(elements.date.textContent, 'Friday, September 18');
  assert.equal(elements['sync-status'].textContent, '');
  now = 180000;
  display.render();
  assert.equal(elements['sync-status'].textContent, 'Sync delayed');
  await client.synchronize();
  display.render();
  assert.equal(elements['sync-status'].textContent, '');
});
