// Run from the repository root: node server/tests/hosting.mjs
// Requires .NET 10, Node 18+, and an unused port 5000. No extra packages.
import assert from 'node:assert/strict';
import { spawn, spawnSync } from 'node:child_process';
import { mkdtemp, mkdir, writeFile, readFile, readdir, rename, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { setTimeout as pause } from 'node:timers/promises';

const root = fileURLToPath(new URL('../../', import.meta.url));
const output = await mkdtemp(join(tmpdir(), 'denclock-hosting-'));
const data = await mkdtemp(join(tmpdir(), 'denclock-settings-hosting-'));
const fixture = await mkdtemp(join(root, 'client', 'hosting-check-'));
const asset = `${fixture.split(/[\\/]/).at(-1)}/nested/probe.js`;
let processHandle;
let logs = '';
const base = 'http://localhost:5000';
async function start(args, cwd = root, environment = {}) {
  logs = '';
  processHandle = spawn('dotnet', args, { cwd, env: { ...process.env, Presentation__ConfigurationDirectory: data, Cors__DevelopmentOrigins__1: 'http://dev.denclock.test:8080', ...environment }, stdio: ['ignore', 'pipe', 'pipe'] });
  processHandle.stdout.on('data', data => { logs += data; });
  processHandle.stderr.on('data', data => { logs += data; });
  for (let attempt = 0; attempt < 100; attempt++) {
    if (processHandle.exitCode !== null || processHandle.signalCode !== null) throw new Error(logs);
    if (logs.includes('Application started.')) return;
    await pause(100);
  }
  throw new Error(`Server did not start: ${logs}`);
}
async function stop() {
  if (!processHandle || (processHandle.exitCode !== null || processHandle.signalCode !== null)) return;
  const exited = new Promise(resolve => processHandle.once('exit', resolve));
  processHandle.kill('SIGTERM');
  await exited;
}
async function get(path, headers = {}) {
  const response = await fetch(base + path, { headers });
  assert.equal(response.status, 200, path);
  return response;
}
try {
  await mkdir(join(fixture, 'nested'));
  await writeFile(join(root, 'client', asset), 'export const probe = 1;');
  await start([join(root, 'server/bin/Debug/net10.0/DenClock.Server.dll')], join(root, 'server'), { ASPNETCORE_ENVIRONMENT: 'Development', DOTNET_ENVIRONMENT: 'Development', Client__TimeServerAddress: 'http://192.168.50.90:5000' });
  assert.match(await (await get('/client-settings.js')).text(), /192\.168\.50\.90:5000/);
  assert.match(await (await get('/')).text(), /client-settings.js/);
  assert.equal(await (await get('/' + asset)).text(), 'export const probe = 1;');
  await writeFile(join(root, 'client', asset), 'export const probe = 2;');
  const changed = await get('/' + asset);
  assert.equal(changed.headers.get('cache-control'), 'no-store');
  assert.equal(await changed.text(), 'export const probe = 2;');
  await stop();
  const publish = spawnSync('dotnet', ['publish', 'server/DenClock.Server.csproj', '--no-restore', '-o', output], { cwd: root, encoding: 'utf8' });
  assert.equal(publish.status, 0, publish.stdout + publish.stderr);
  assert.equal(await readFile(join(output, 'wwwroot', asset), 'utf8'), 'export const probe = 2;');
  const published = await readdir(join(output, 'wwwroot'));
  for (const excluded of ['tests', 'docs', 'node_modules', 'package.json', 'package-lock.json', 'README.md']) assert.ok(!published.includes(excluded), excluded);
  for (const required of ['index.html', 'clock.js', 'clock.css', 'config.js', 'presentation.js', 'settings.js']) assert.ok(published.includes(required), required);
  assert.equal(JSON.parse(await readFile(join(output, 'presentation.json'), 'utf8')).temperatureUnit, 'F');
  // Start from an unrelated working directory, as a Windows Service may do.
  await start([join(output, 'DenClock.Server.dll')], tmpdir(), { ASPNETCORE_ENVIRONMENT: 'Production', DOTNET_ENVIRONMENT: 'Production', Client__TimeServerAddress: 'http://must-not-leak.example' });
  assert.equal(await (await get('/client-settings.js')).text(), 'export const serverAddress = "";');
  await get('/');
  await get('/clock.js');
  assert.equal(typeof (await (await get('/api/time')).json()).unixTimeMilliseconds, 'number');
  const allowed = await get('/api/time', { Origin: 'http://localhost:5000' });
  assert.equal(allowed.headers.get('access-control-allow-origin'), 'http://localhost:5000');
  const denied = await get('/api/time', { Origin: 'http://unapproved.example' });
  assert.equal(denied.headers.get('access-control-allow-origin'), null);
  const preflight = await fetch(base + '/api/time', { method: 'OPTIONS', headers: { Origin: 'http://localhost:5000', 'Access-Control-Request-Method': 'GET' } });
  assert.equal(preflight.status, 204);
  assert.equal(preflight.headers.get('access-control-allow-origin'), 'http://localhost:5000');
  assert.equal((await get('/client-settings.js', { Origin: 'http://localhost:5000' })).headers.get('access-control-allow-origin'), null);
  const presentation = await get('/api/presentation', { Origin: 'http://localhost:5000' });
  assert.equal(presentation.headers.get('cache-control'), 'no-store');
  assert.equal(presentation.headers.get('access-control-allow-origin'), 'http://localhost:5000');
  const snapshot = await presentation.json();
  assert.equal(snapshot.slots.length, 2);
  assert.equal(snapshot.slots[0].observation, null);
  assert.equal((await get('/api/presentation', { Origin: 'http://unapproved.example' })).headers.get('access-control-allow-origin'), null);
  await stop();
  // The initialized data file is authoritative across deployment/restart.
  const stored = await readFile(join(data, 'presentation.json'), 'utf8');
  await writeFile(join(output, 'presentation.json'), '{}');
  await start([join(output, 'DenClock.Server.dll')], tmpdir(), { DOTNET_ENVIRONMENT: 'Production', ASPNETCORE_ENVIRONMENT: 'Production' });
  const settingsResponse = await get('/api/settings/presentation', { Origin: 'http://localhost:5000' });
  assert.equal(settingsResponse.headers.get('cache-control'), 'no-store');
  assert.equal(settingsResponse.headers.get('access-control-allow-origin'), 'http://localhost:5000');
  const settings = await settingsResponse.json();
  assert.deepEqual(settings.configuration, JSON.parse(stored));
  const url = base + '/api/settings/presentation';
  const update = (body, origin = base, contentType = 'application/json') => fetch(url, {
    method: 'PUT', headers: { Origin: origin, 'Content-Type': contentType }, body: JSON.stringify(body),
  });
  const preflightSettings = await fetch(url, { method: 'OPTIONS', headers: {
    Origin: 'http://localhost:5000', 'Access-Control-Request-Method': 'PUT', 'Access-Control-Request-Headers': 'content-type',
  }});
  assert.equal(preflightSettings.status, 204);
  assert.match(preflightSettings.headers.get('access-control-allow-methods'), /PUT/);
  assert.match(preflightSettings.headers.get('access-control-allow-headers'), /content-type/i);
  const deniedSettingsPreflight = await fetch(url, { method: 'OPTIONS', headers: {
    Origin: 'http://unrelated.example', 'Access-Control-Request-Method': 'PUT',
  }});
  assert.equal(deniedSettingsPreflight.headers.get('access-control-allow-origin'), null);
  assert.equal((await update(settings, 'http://unrelated.example')).status, 403);
  assert.equal((await update(settings, 'null')).status, 403);
  assert.equal((await update(settings, base, 'text/plain')).status, 415);
  assert.equal((await update({ configuration: settings.configuration })).status, 400);
  const invalid = structuredClone(settings); invalid.configuration.temperatureUnit = 'bad';
  assert.equal((await update(invalid)).status, 400);
  assert.equal(await readFile(join(data, 'presentation.json'), 'utf8'), stored);
  const changedSettings = structuredClone(settings);
  changedSettings.configuration.temperatureUnit = 'C';
  changedSettings.configuration.sensors.push({ internalName: 'test-secondary', displayName: 'Preserved', position: 'secondary', maxAgeSeconds: 300, extra: { flag: true } });
  changedSettings.configuration.future = 'preserved';
  const savedResponse = await update(changedSettings, 'http://dev.denclock.test:8080');
  assert.equal(savedResponse.status, 200);
  assert.equal(savedResponse.headers.get('access-control-allow-origin'), 'http://dev.denclock.test:8080');
  assert.equal(savedResponse.headers.get('cache-control'), 'no-store');
  const saved = await savedResponse.json();
  assert.notEqual(saved.revision, settings.revision);
  assert.deepEqual(saved.configuration, changedSettings.configuration);
  assert.equal((await update(settings)).status, 409);
  assert.equal((await (await get('/api/presentation')).json()).temperatureUnit, 'C');
  assert.deepEqual(JSON.parse(await readFile(join(data, 'presentation.json'), 'utf8')), saved.configuration);
  // Make the storage directory temporarily unavailable; a failed save must not publish.
  await rename(data, data + '-away');
  try {
    const failed = structuredClone(saved); failed.configuration.temperatureUnit = 'F';
    const failure = await update(failed);
    assert.equal(failure.status, 500);
    assert.match((await failure.json()).error, /Previous settings remain active/);
    assert.equal((await (await get('/api/presentation')).json()).temperatureUnit, 'C');
    assert.equal((await (await get('/api/settings/presentation')).json()).revision, saved.revision);
  } finally { await rename(data + '-away', data); }
  await stop();
  await start([join(output, 'DenClock.Server.dll')], tmpdir(), { DOTNET_ENVIRONMENT: 'Production', ASPNETCORE_ENVIRONMENT: 'Production' });
  const afterRestart = await (await get('/api/settings/presentation')).json();
  assert.deepEqual(afterRestart.configuration, saved.configuration);
  assert.equal((await update(saved)).status, 409, 'pre-restart revisions are invalidated');
  await stop();
  // Invalid authoritative files fail without falling back to bundled settings.
  await writeFile(join(data, 'presentation.json'), '{}');
  const failure = spawnSync('dotnet', [join(output, 'DenClock.Server.dll')], {
    cwd: tmpdir(), encoding: 'utf8', timeout: 10000,
    env: { ...process.env, Presentation__ConfigurationDirectory: data, DOTNET_ENVIRONMENT: 'Production', ASPNETCORE_ENVIRONMENT: 'Production' },
  });
  assert.notEqual(failure.status, 0);
  assert.match(failure.stdout + failure.stderr, /presentation.json/);
  assert.doesNotMatch(failure.stdout, /Application started/);
  console.log('PASS: live source edits, recursive publish, exclusions, production same-origin configuration, API, restricted CORS, settings validation/conflicts/persistence, runtime application, and restart/deployment preservation.');
} finally {
  await stop();
  await rm(fixture, { recursive: true, force: true });
  await rm(output, { recursive: true, force: true });
  await rm(data, { recursive: true, force: true });
}
