using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using LoRa.Service;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

// Literal fixtures follow docs/Gateway Protocol.md and docs/LoRa Protocol.md.
// No alternate serializers are used to generate expected protocol bytes.
byte[] hello = [1, 0, 1, 1];
byte[] sensor = [1, 12, 0x0f, 0x1f, 1, 2, 0x08, 0x5f, 2, 2, 0x10, 0xe1, 3, 2, 0x0f, 0x48];
byte[] rx = [2, 0, 18, 0xb8, 38, .. sensor];
var count = 0;
void Check(bool condition, string name)
{
    if (!condition) throw new Exception($"FAIL: {name}");
    count++;
}
void Invalid(Action action, string name)
{
    try { action(); }
    catch (InvalidDataException) { count++; return; }
    throw new Exception($"FAIL: expected invalid data: {name}");
}
async Task InvalidFrame(byte[] bytes, string name)
{
    try { await GatewayProtocol.ReadAsync(new MemoryStream(bytes), default); }
    catch (InvalidDataException) { count++; return; }
    throw new Exception($"FAIL: expected invalid frame: {name}");
}

Check(GatewayProtocol.ParseHeader([1, 0, 1]) == new GatewayHeader(1, 1), "HELLO header");
Check(GatewayProtocol.ParseHeader([2, 1, 1]).PayloadLength == 257, "big-endian length >255");
Check(GatewayProtocol.ParseHeader([2, 255, 255]).PayloadLength == 65535, "full uint16 length");
foreach (byte[] header in new byte[][] { [], [1, 0], [1, 0, 1, 0], [0, 0, 1], [3, 0, 1], [255, 0, 1], [1, 0, 0], [1, 0, 2], [2, 0, 0], [2, 0, 1] })
    Invalid(() => GatewayProtocol.ParseHeader(header), "invalid header/type/length");
Check(GatewayProtocol.ParseHello([1]) == 1 && GatewayProtocol.ParseHello([255]) == 255, "valid HELLO IDs");
foreach (byte[] payload in new byte[][] { [], [0], [1, 2] })
    Invalid(() => GatewayProtocol.ParseHello(payload), "invalid HELLO payload");
foreach (byte[] truncated in new byte[][] { [1], [1, 0], [1, 0, 1], [2, 0, 18, 0xb8, 38, 1] })
    await InvalidFrame(truncated, "EOF inside header/payload");
Check(await GatewayProtocol.ReadAsync(new MemoryStream(), default) is null, "clean EOF");

// Every possible single split of the combined stream, plus one-byte reads.
byte[] combined = [.. hello, .. rx, .. rx];
for (var split = 1; split < combined.Length; split++)
{
    using var stream = new FragmentedStream(combined, split, int.MaxValue);
    var first = await GatewayProtocol.ReadAsync(stream, default);
    var second = await GatewayProtocol.ReadAsync(stream, default);
    var third = await GatewayProtocol.ReadAsync(stream, default);
    Check(first?.Type == 1 && first.Payload.SequenceEqual(new byte[] { 1 }), "split HELLO");
    Check(second?.Type == 2 && second.Payload.SequenceEqual(rx[3..]), "split RX");
    Check(third?.Type == 2 && third.Payload.SequenceEqual(rx[3..]), "coalesced second RX");
    Check(await GatewayProtocol.ReadAsync(stream, default) is null, "all frames consumed");
}
using (var stream = new FragmentedStream(combined, 1, 1))
{
    Check((await GatewayProtocol.ReadAsync(stream, default))?.Type == 1, "byte-wise HELLO");
    Check((await GatewayProtocol.ReadAsync(stream, default))?.Payload.SequenceEqual(rx[3..]) == true, "byte-wise RX");
}
byte[] maxPayload = new byte[65535];
using (var stream = new MemoryStream([2, 255, 255, .. maxPayload]))
    Check((await GatewayProtocol.ReadAsync(stream, default))?.Payload.Length == 65535, "full payload read");

var timestamp = DateTimeOffset.Parse("2026-09-20T14:38:27.123Z");
var received = GatewayProtocol.ParseRx(1, rx[3..], timestamp);
Check(received.GatewayId == 1 && received.ReceivedAt == timestamp, "connection context");
Check(received.RssiDbm == -72 && received.SnrDb == 9.5m, "signed RSSI and SNR scaling");
Check(received.RawPacket.SequenceEqual(sensor), "unchanged raw packet");
Check(GatewayProtocol.ParseRx(1, [128, 244, 0xff], timestamp).SnrDb == -3m, "negative SNR");
Check(GatewayProtocol.ParseRx(1, [128, 128], timestamp).RssiDbm == -128, "RSSI boundary");
Invalid(() => GatewayProtocol.ParseRx(1, [0], timestamp), "missing metadata");
var report = SensorReportDecoder.Decode(sensor);
Check(report == new SensorReport(12, 3871, 21.43m, 43.21m, 3.912m), "all decoded fields");
byte[] negative = [1, 1, 0x12, 0x34, 1, 2, 0xfb, 0x1e, 2, 2, 0x27, 0x10, 3, 2, 0xff, 0xff];
report = SensorReportDecoder.Decode(negative);
Check(report.Sequence == 0x1234 && report.TemperatureC == -12.50m && report.RelativeHumidityPercent == 100m && report.BatteryVolts == 65.535m, "signed/scaled bounds");
Check(SensorReportDecoder.Decode([1, 1, 0, 0]).TemperatureC is null, "optional TLVs");
Check(SensorReportDecoder.Decode([1, 1, 255, 255]).Sequence == 65535, "max sequence");
Check(SensorReportDecoder.Decode([1, 1, 0, 0, 1, 2, 128, 0]).TemperatureC == -327.68m, "min temperature");
Check(SensorReportDecoder.Decode([1, 1, 0, 0, 1, 2, 127, 255]).TemperatureC == 327.67m, "max temperature");
byte[] unknown = [.. sensor[..4], 42, 3, 0, 1, 2, .. sensor[4..], 99, 0];
Check(SensorReportDecoder.Decode(unknown) == SensorReportDecoder.Decode(sensor), "unknown TLVs skipped");
byte[] reordered = [.. sensor[..4], .. sensor[12..], .. sensor[8..12], .. sensor[4..8]];
Check(SensorReportDecoder.Decode(reordered) == SensorReportDecoder.Decode(sensor), "TLV ordering");
foreach (byte[] bad in new byte[][] {
    [], [1], [1, 1, 0], [0, 1, 0, 0], [2, 1, 0, 0], [1, 0, 0, 0], [1, 240, 0, 0], [1, 255, 0, 0],
    [1, 1, 0, 0, 0, 0], [1, 1, 0, 0, 1], [1, 1, 0, 0, 42, 3, 0, 1],
    [1, 1, 0, 0, 1, 1, 0], [1, 1, 0, 0, 2, 0], [1, 1, 0, 0, 3, 3, 0, 0, 0],
    [1, 1, 0, 0, 2, 2, 0x27, 0x11], [.. sensor, .. sensor[4..8]] })
    Invalid(() => SensorReportDecoder.Decode(bad), "malformed sensor packet");

var log = new CaptureLogger();
var before = DateTimeOffset.UtcNow;
await new GatewaySession(log).RunAsync(new FragmentedStream(combined, 1, 1), default);
Check(log.Lines.Count(line => line.Contains("Node 12 Sequence 3871")) == 2, "session processes multiple reports");
Check(log.Lines.Any(line => line.Contains("Gateway 1 connected")), "HELLO logged");
Check(log.Lines.Any(line => line.Contains("Temperature 21.43 C Relative Humidity 43.21 % Battery 3.912 V RSSI -72 dBm SNR 9.50 dB")), "complete diagnostics");
Check(log.Timestamps.All(value => value >= before && value <= DateTimeOffset.UtcNow && value.Offset == TimeSpan.Zero) && log.Timestamps.Count == 2, "service UTC timestamps");
foreach (byte[] badSession in new byte[][] { rx, [1, 0, 1, 0], [.. hello, .. hello, .. rx], [0, 0, 0], [.. hello, 2, 0] })
{
    log = new CaptureLogger();
    await new GatewaySession(log).RunAsync(new MemoryStream(badSession), default);
    Check(log.Lines.Any(line => line.Contains("Gateway Protocol error")), "gateway errors classified");
    Check(!log.Lines.Any(line => line.Contains("Node 12")), "bad gateway frame not decoded");
}
log = new CaptureLogger();
await new GatewaySession(log).RunAsync(new MemoryStream([
    .. hello, 2, 0, 2, 0xb8, 38, // Empty raw LoRa payload is a LoRa error.
    2, 0, 3, 0xb8, 38, 0, // Invalid class.
    2, 0, 3, 0xb8, 38, 99, // Unsupported class.
    2, 0, 7, 0xb8, 38, 1, 1, 0, 0, 1, // Truncated TLV.
    .. rx]), default);
Check(log.Lines.Count(line => line.Contains("LoRa protocol error")) == 4, "LoRa errors classified");
Check(!log.Lines.Any(line => line.Contains("Gateway Protocol error")), "layers stay separate");
Check(log.Lines.Any(line => line.Contains("Node 12 Sequence 3871")), "valid report follows bad LoRa packets");

// Real TCP worker: concurrent connections, reconnect, cancellation during a frame.
// Port 0 is used only in this direct worker test to obtain an OS-assigned port.
var workerLog = new CaptureLogger();
using var worker = new GatewayWorker(Options.Create(new GatewayOptions { ListenAddress = "127.0.0.1", Port = 0 }), workerLog);
await worker.StartAsync(default);
await WaitUntil(() => workerLog.Endpoint != null || worker.ExecuteTask?.IsCompleted == true);
if (worker.ExecuteTask?.IsFaulted == true) await worker.ExecuteTask;
var endpoint = workerLog.Endpoint!;
using var idle = new TcpClient();
await idle.ConnectAsync(endpoint.Address, endpoint.Port);
await idle.GetStream().WriteAsync(new byte[] { 1, 0 });
for (var run = 1; run <= 2; run++)
{
    using var client = new TcpClient();
    await client.ConnectAsync(endpoint.Address, endpoint.Port);
    await client.GetStream().WriteAsync(combined);
    client.Client.Shutdown(SocketShutdown.Send);
    var expected = run * 2;
    await WaitUntil(() => workerLog.Lines.Count(line => line.Contains("Node 12 Sequence 3871")) == expected);
}
using (var bad = new TcpClient())
{
    await bad.ConnectAsync(endpoint.Address, endpoint.Port);
    await bad.GetStream().WriteAsync(new byte[] { 1, 0, 1, 0 });
    using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
    Check(await bad.GetStream().ReadAsync(new byte[1], timeout.Token) == 0, "malformed connection closed");
}
using (var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5))) await worker.StopAsync(timeout.Token);
Check(worker.ExecuteTask?.IsCompletedSuccessfully == true, "worker stops with a partial-frame client");
Console.WriteLine($"PASS: {count} protocol, session, and TCP checks");

static async Task WaitUntil(Func<bool> condition)
{
    using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
    while (!condition()) await Task.Delay(10, timeout.Token);
}

sealed class FragmentedStream(byte[] data, int boundary, int chunkSize) : MemoryStream(data)
{
    public override ValueTask<int> ReadAsync(Memory<byte> buffer, CancellationToken cancellationToken = default)
    {
        var length = Math.Min(buffer.Length, chunkSize);
        if (Position < boundary) length = Math.Min(length, boundary - (int)Position);
        return base.ReadAsync(buffer[..length], cancellationToken);
    }
}

sealed class CaptureLogger : ILogger<GatewayWorker>
{
    public ConcurrentQueue<string> Lines { get; } = new();
    public ConcurrentQueue<DateTimeOffset> Timestamps { get; } = new();
    public IPEndPoint? Endpoint;
    public IDisposable? BeginScope<TState>(TState state) where TState : notnull => null;
    public bool IsEnabled(LogLevel logLevel) => true;
    public void Log<TState>(LogLevel logLevel, EventId eventId, TState state, Exception? exception, Func<TState, Exception?, string> formatter)
    {
        if (state is IEnumerable<KeyValuePair<string, object?>> fields)
            foreach (var field in fields)
            {
                if (field.Key == "ReceivedAt" && field.Value is DateTimeOffset timestamp) Timestamps.Enqueue(timestamp);
                if (field.Key == "Endpoint" && field.Value is IPEndPoint endpoint) Endpoint = endpoint;
            }
        Lines.Enqueue(formatter(state, exception));
    }
}
