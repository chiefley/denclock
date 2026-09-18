// Run from the repository root: node server/tests/hosting.mjs
// Requires .NET 10, Node 18+, and an unused port 5000. No extra packages.
import assert from 'node:assert/strict';
import { spawn, spawnSync } from 'node:child_process';
import { mkdtemp, mkdir, writeFile, readFile, readdir, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { setTimeout as pause } from 'node:timers/promises';

const root = fileURLToPath(new URL('../../', import.meta.url));
const output = await mkdtemp(join(tmpdir(), 'denclock-hosting-'));
const fixture = await mkdtemp(join(root, 'client', 'hosting-check-'));
const asset = `${fixture.split(/[\\/]/).at(-1)}/nested/probe.js`;
let processHandle;
let logs = '';
const base = 'http://localhost:5000';
async function start(args, cwd = root, environment = {}) {
  logs = '';
  processHandle = spawn('dotnet', args, { cwd, env: { ...process.env, ...environment }, stdio: ['ignore', 'pipe', 'pipe'] });
  processHandle.stdout.on('data', data => { logs += data; });
  processHandle.stderr.on('data', data => { logs += data; });
  for (let attempt = 0; attempt < 100; attempt++) {
    if (processHandle.exitCode !== null) throw new Error(logs);
    if (logs.includes('Application started.')) return;
    await pause(100);
  }
  throw new Error(`Server did not start: ${logs}`);
}
async function stop() {
  if (!processHandle || processHandle.exitCode !== null) return;
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
  for (const required of ['index.html', 'clock.js', 'clock.css', 'config.js']) assert.ok(published.includes(required), required);
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
  console.log('PASS: live source edits, recursive publish, exclusions, production same-origin configuration, API and restricted CORS.');
} finally {
  await stop();
  await rm(fixture, { recursive: true, force: true });
  await rm(output, { recursive: true, force: true });
}
