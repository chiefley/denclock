// Requires a server build, Node 18+, and free HTTP port 5000. Uses a local MQTT peer.
import assert from 'node:assert/strict';
import net from 'node:net';
import { mkdtemp, writeFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { setTimeout as pause } from 'node:timers/promises';

const root = fileURLToPath(new URL('../../', import.meta.url));
const serverDll = fileURLToPath(new URL('../bin/Debug/net10.0/DenClock.Server.dll', import.meta.url));
const data = await mkdtemp(join(tmpdir(), 'denclock-mqtt-'));
await writeFile(join(data, 'presentation.json'), JSON.stringify({ temperatureUnit: 'F', gateways: [], sensors: [
  { gatewayId: 1, nodeId: 1, name: 'den', displayName: 'Den', position: 'upperLeft', maxAgeSeconds: 60 },
  { gatewayId: 2, nodeId: 1, name: 'porch', displayName: 'Outside', position: 'upperRight', maxAgeSeconds: 60 },
]}));
const sockets = new Set();
let subscriptions = 0, clientIds = [], protocolError;
let active, processHandle, logs = '';
function packet(header, body) {
  const length = []; let size = body.length;
  do { let b = size % 128; size = Math.floor(size / 128); if (size) b |= 128; length.push(b); } while (size);
  return Buffer.concat([Buffer.from([header, ...length]), body]);
}
function text(value) {
  const bytes = Buffer.from(value);
  const length = Buffer.alloc(2); length.writeUInt16BE(bytes.length);
  return Buffer.concat([length, bytes]);
}
const observed = new Date(Date.now() - 120000).toISOString();
const report = (stamp, value = 22.15) => JSON.stringify({
  messageType: 'sensor-report', gatewayId: 2, nodeId: 1, receivedUtc: stamp,
  measurements: { temperature: { value, units: 'C' }, relativeHumidity: { value: 52.26, units: 'percent' } },
});
let retained = report(observed);
let id = 0;
function publish(payload, topic = 'lora/sensors/2/1') {
  const identifier = Buffer.alloc(2); identifier.writeUInt16BE(++id);
  active.write(packet(0x33, Buffer.concat([text(topic), identifier, Buffer.from(payload)])));
}
const peer = net.createServer(socket => {
  sockets.add(socket); active = socket;
  let buffered = Buffer.alloc(0);
  socket.on('error', () => {});
  socket.on('close', () => sockets.delete(socket));
  socket.on('data', chunk => {
    buffered = Buffer.concat([buffered, chunk]);
    try {
      while (buffered.length >= 2) {
        let size = 0, multiplier = 1, offset = 1, byte;
        do {
          if (offset >= buffered.length) return;
          byte = buffered[offset++]; size += (byte & 127) * multiplier; multiplier *= 128;
        } while (byte & 128);
        if (buffered.length < offset + size) return;
        const header = buffered[0], body = buffered.subarray(offset, offset + size);
        buffered = buffered.subarray(offset + size);
        switch (header >> 4) {
          case 1: {
            assert.equal(body[7] & 2, 2, 'clean session');
            const length = body.readUInt16BE(10);
            clientIds.push(body.toString('utf8', 12, 12 + length));
            socket.write(Buffer.from([0x20, 2, 0, 0])); break;
          }
          case 8: {
            const length = body.readUInt16BE(2);
            assert.equal(body.toString('utf8', 4, 4 + length), 'lora/sensors/+/+');
            assert.equal(body[4 + length], 1, 'QoS1 subscription');
            subscriptions++;
            socket.write(Buffer.from([0x90, 3, body[0], body[1], 1]));
            publish(retained); break;
          }
          case 4: break;
          case 12: socket.write(Buffer.from([0xd0, 0])); break;
          case 14: socket.end(); break;
          default: throw Error('Unexpected MQTT packet ' + header);
        }
      }
    } catch (error) { protocolError = error; socket.destroy(); }
  });
});
await new Promise(resolve => peer.listen(0, '127.0.0.1', resolve));
const brokerPort = peer.address().port;
async function waitFor(condition, label) {
  const end = Date.now() + 15000;
  while (Date.now() < end) {
    if (protocolError) throw protocolError;
    if (await condition()) return;
    await pause(50);
  }
  throw Error('Timed out: ' + label + '\n' + logs);
}
async function snapshot() { return (await fetch('http://127.0.0.1:5000/api/presentation')).json(); }
async function start() {
  logs = '';
  processHandle = spawn('dotnet', [serverDll], { cwd: root, stdio: ['ignore','pipe','pipe'], env: {
    ...process.env, Presentation__ConfigurationDirectory: data, DOTNET_ENVIRONMENT: 'Production', ASPNETCORE_ENVIRONMENT: 'Production',
    Mqtt__Host: '127.0.0.1', Mqtt__Port: String(brokerPort), Mqtt__ClientId: 'denclock-integration-test',
  }});
  processHandle.stdout.on('data', b => logs += b);
  processHandle.stderr.on('data', b => logs += b);
  await waitFor(() => logs.includes('Application started.'), 'HTTP startup');
}
async function stop() {
  if (!processHandle || (processHandle.exitCode !== null || processHandle.signalCode !== null)) return;
  const exit = new Promise(resolve => processHandle.once('exit', resolve));
  processHandle.kill('SIGTERM');
  await exit;
}
try {
  await start();
  await waitFor(async () => (await snapshot()).slots[1].observation !== null, 'retained report');
  assert.equal(Date.parse((await snapshot()).slots[1].observation.receivedUtc), Date.parse(observed));
  assert.equal((await snapshot()).slots[1].observation.temperatureC, 22.15);
  assert.equal((await snapshot()).slots[0].observation, null);
  assert.equal(clientIds[0], 'denclock-integration-test');
  publish('{bad');
  publish(report(observed, 99));
  publish(report(new Date(Date.parse(observed) - 1000).toISOString(), 100));
  await pause(100);
  assert.equal((await snapshot()).slots[1].observation.temperatureC, 22.15);
  const newer = new Date(Date.now() - 1000).toISOString();
  retained = report(newer, 24);
  publish(retained);
  await waitFor(async () => (await snapshot()).slots[1].observation.temperatureC === 24, 'fresh observation');
  publish(JSON.stringify({ ...JSON.parse(report(newer, -10)), gatewayId: 1 }), 'lora/sensors/1/1');
  await waitFor(async () => (await snapshot()).slots[0].observation?.temperatureC === -10, 'other gateway same node');
  assert.equal((await snapshot()).slots[1].observation.temperatureC, 24);
  publish(report(newer, 99), 'lora/sensors/1/1'); // mismatched payload gateway
  publish(JSON.stringify({ ...JSON.parse(report(newer)), gatewayId: 3 }), 'lora/sensors/3/1'); // unmapped
  await pause(100);
  assert.equal((await snapshot()).slots[0].observation.temperatureC, -10);
  // Apply settings with MQTT connected and retain the original observation timestamp.
  const settings = await (await fetch('http://127.0.0.1:5000/api/settings/presentation')).json();
  settings.configuration.temperatureUnit = 'C';
  Object.assign(settings.configuration.sensors[1], { displayName: 'Garden', maxAgeSeconds: 30 });
  const saved = await fetch('http://127.0.0.1:5000/api/settings/presentation', {
    method: 'PUT', headers: { 'Content-Type': 'application/json', Origin: 'http://127.0.0.1:5000' }, body: JSON.stringify(settings),
  });
  assert.equal(saved.status, 200);
  const applied = await snapshot();
  assert.equal(applied.temperatureUnit, 'C');
  assert.equal(applied.slots[1].displayName, 'Garden');
  assert.equal(applied.slots[1].maxAgeSeconds, 30);
  assert.equal(Date.parse(applied.slots[1].observation.receivedUtc), Date.parse(newer));
  const duringSave = new Date(Date.now() - 500).toISOString();
  retained = report(duringSave, 24);
  publish(retained);
  await waitFor(async () => Date.parse((await snapshot()).slots[1].observation.receivedUtc) === Date.parse(duringSave), 'MQTT continues after save');
  // Simulate a real broker outage, while checking the time service remains available.
  for (const socket of sockets) socket.destroy();
  await new Promise(resolve => peer.close(resolve));
  await pause(5500);
  assert.equal((await fetch('http://127.0.0.1:5000/api/time')).status, 200);
  assert.equal((await snapshot()).slots[1].observation.temperatureC, 24);
  await new Promise(resolve => peer.listen(brokerPort, '127.0.0.1', resolve));
  await waitFor(() => subscriptions >= 2, 'resubscribe after broker restart');
  assert.equal((await snapshot()).slots[1].observation.temperatureC, 24);
  const prior = subscriptions;
  await stop(); await start();
  await waitFor(() => subscriptions > prior, 'resubscribe after server restart');
  await waitFor(async () => (await snapshot()).slots[1].observation?.temperatureC === 24, 'retained state after server restart');
  assert.equal(Date.parse((await snapshot()).slots[1].observation.receivedUtc), Date.parse(duringSave));
  console.log('PASS: real MQTT client subscription/QoS1, retained MQTT-to-HTTP values, malformed/duplicate/older rejection, broker outage recovery, server restart, and preserved timestamps.');
} finally {
  await stop();
  for (const socket of sockets) socket.destroy();
  if (peer.listening) await new Promise(resolve => peer.close(resolve));
  await rm(data, { recursive: true, force: true });
}
