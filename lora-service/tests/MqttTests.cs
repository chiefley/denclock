using System.Buffers;
using System.Text.Json;
using System.Net;
using System.Net.Sockets;
using System.Buffers.Binary;
using System.Collections.Concurrent;
using Microsoft.Extensions.Options;
using LoRa.Service;
using MQTTnet.Protocol;

static class MqttTests
{
    public static async Task RunAsync()
    {
        var count = 0;
        void Check(bool condition, string name)
        {
            if (!condition) throw new Exception($"FAIL: {name}");
            count++;
        }
        var log = new CaptureLogger();
        var mapper = new ReportNormalizer(log);
        byte[] bytes = [1, 1, 0x12, 0x34, 42, 1, 9, 1, 2, 0xfb, 0x1e, 2, 2, 0x10, 0xe1, 3, 2, 0xff, 0xff];
        var timestamp = DateTimeOffset.Parse("2026-09-21T20:42:17.123Z");
        var packet = new ReceivedPacket(1, timestamp, -72, -3m, bytes);
        var report = mapper.Normalize(SensorReportDecoder.Decode(bytes), packet);
        Check(report.Measurements.Count == 3 && log.Lines.Any(x => x.Contains("Unknown TLV Type 42")), "unknown skipped with warning");
        Check(report.Measurements["temperature"].Value == -12.50m, "signed big endian normalization");
        Check(report.Measurements["relativeHumidity"].Value == 43.21m, "humidity normalization");
        Check(report.Measurements["batteryVoltage"].Value == 65.535m, "unsigned and fidelity");
        Check(report.Measurements["batteryVoltage"].Precision == 4 && report.Measurements["batteryVoltage"].Units == "V", "units precision metadata");
        Check(report.GatewayId == 1 && report.NodeId == 1, "numeric identity");
        var other = mapper.Normalize(SensorReportDecoder.Decode(bytes), packet with { GatewayId = 2 });
        Check(other.NodeId == report.NodeId && other.GatewayId != report.GatewayId, "same node across gateways");
        Check(MqttPublisher.CreateMessage(other).Topic == "lora/sensors/2/1", "distinct retained topic");
        var unknown = mapper.Normalize(SensorReportDecoder.Decode([1, 37, 0, 0]), packet with { GatewayId = 255 });
        Check(unknown.GatewayId == 255 && unknown.NodeId == 37 && unknown.Measurements.Count == 0, "identities need no configuration");
        var publisher = new CapturePublisher();
        var handler = new LoRaPacketHandler(mapper, publisher, log);
        await handler.HandleAsync(packet, default);
        Check(publisher.Reports.Count == 1, "one report one publish");
        var message = MqttPublisher.CreateMessage(report);
        Check(message.Topic == "lora/sensors/1/1", "topic");
        Check(message.Retain && message.QualityOfServiceLevel == MqttQualityOfServiceLevel.AtLeastOnce, "QoS and retain");
        using var payload = JsonDocument.Parse(message.Payload);
        var root = payload.RootElement;
        Check(root.GetProperty("gatewayId").GetInt32() == 1, "integer gateway JSON");
        Check(root.GetProperty("nodeId").GetInt32() == 1, "integer node JSON");
        Check(!root.TryGetProperty("gateway", out _) && !root.TryGetProperty("node", out _), "no named identities");
        Check(root.GetProperty("messageType").GetString() == "sensor-report" && root.GetProperty("sequence").GetInt32() == 0x1234, "type and sequence JSON");
        Check(root.GetProperty("receivedUtc").GetDateTimeOffset() == timestamp, "timestamp preserved");
        Check(root.GetProperty("radio").GetProperty("rssiDbm").GetInt32() == -72 && root.GetProperty("radio").GetProperty("snrDb").GetDecimal() == -3m, "radio preserved");
        Check(root.GetProperty("measurements").GetProperty("batteryVoltage").GetProperty("value").ValueKind == JsonValueKind.Number, "numeric JSON");
        Check(root.EnumerateObject().Count() == 7, "only contract envelope fields");
        // Real MQTTnet client against a minimal loopback MQTT 3.1.1 peer. No broker install.
        using var peer = new MqttPeer();
        using var mqtt = new MqttPublisher(Options.Create(new MqttOptions { Host = "127.0.0.1", Port = peer.Port, ClientId = "test-publisher" }), log);
        await mqtt.PublishAsync(report, default);
        var outageHandler = new LoRaPacketHandler(mapper, mqtt, log);
        await outageHandler.HandleAsync(packet, default);
        await outageHandler.HandleAsync(packet, default);
        Check(log.Lines.Count(x => x.Contains("MQTT unavailable")) == 3, "receive handling continues without MQTT");
        Check(log.Lines.Any(x => x.Contains("MQTT unavailable")), "unavailable drops report");
        await mqtt.StartAsync(default);
        await peer.Connected.Task.WaitAsync(TimeSpan.FromSeconds(10));
        await WaitUntil(() => log.Lines.Any(x => x.Contains("MQTT connected")));
        await mqtt.PublishAsync(report, default);
        await WaitUntil(() => peer.Messages.Count == 1);
        Check(peer.Messages.TryPeek(out var wire) && wire.Header == 0x33 && wire.Topic == message.Topic, "wire QoS1 retained publish");
        Check(wire!.Payload.SequenceEqual(message.Payload.ToArray()), "wire normalized payload");
        peer.DropConnection();
        await WaitUntil(() => log.Lines.Any(x => x.Contains("MQTT disconnected")));
        await mqtt.PublishAsync(report, default); // Discard while disconnected; never replay.
        await WaitUntil(() => peer.Connections >= 2 && log.Lines.Count(x => x.Contains("MQTT connected")) >= 2);
        Check(peer.Messages.Count == 1, "no outage replay");
        await mqtt.PublishAsync(report, default);
        await WaitUntil(() => peer.Messages.Count == 2);
        Check(peer.Messages.Count == 2, "publish after reconnect");
        await mqtt.PublishAsync(other, default);
        await WaitUntil(() => peer.Messages.Count == 3);
        var otherWire = peer.Messages.Last();
        using (var otherPayload = JsonDocument.Parse(otherWire.Payload))
            Check(otherWire.Topic == "lora/sensors/2/1" &&
                otherPayload.RootElement.GetProperty("gatewayId").GetInt32() == 2 &&
                otherPayload.RootElement.GetProperty("nodeId").GetInt32() == 1,
                "same node on second gateway published independently over MQTT");
        using var stop = new CancellationTokenSource(TimeSpan.FromSeconds(10));
        await mqtt.StopAsync(stop.Token);
        Check(mqtt.ExecuteTask!.IsCompletedSuccessfully, "MQTT graceful stop");
        // Launch an isolated published-style host directory containing no mapping file.
        var temp = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(temp);
        try
        {
            foreach (var file in Directory.GetFiles(AppContext.BaseDirectory))
                if (Path.GetFileName(file) != "semantic-mapping.json")
                    File.Copy(file, Path.Combine(temp, Path.GetFileName(file)));
            using var portProbe = new TcpListener(IPAddress.Loopback, 0);
            portProbe.Start();
            var port = ((IPEndPoint)portProbe.LocalEndpoint).Port;
            portProbe.Stop();
            var start = new System.Diagnostics.ProcessStartInfo("dotnet")
            {
                WorkingDirectory = temp, RedirectStandardOutput = true, RedirectStandardError = true
            };
            start.ArgumentList.Add(Path.Combine(temp, "LoRa.Service.dll"));
            start.ArgumentList.Add("--Gateway:ListenAddress=127.0.0.1");
            start.ArgumentList.Add($"--Gateway:Port={port}");
            start.ArgumentList.Add($"--Mqtt:Port={peer.Port}");
            start.ArgumentList.Add($"--Logging:FilePath={Path.Combine(temp, "service-.log")}");
            using var host = System.Diagnostics.Process.Start(start)!;
            var output = host.StandardOutput.ReadToEndAsync();
            var error = host.StandardError.ReadToEndAsync();
            try
            {
                await WaitUntil(() => Directory.GetFiles(temp, "service-*.log")
                    .Any(file => File.ReadAllText(file).Contains("LoRa Service listening")));
                Check(!host.HasExited && !File.Exists(Path.Combine(temp, "semantic-mapping.json")),
                    "real host starts without mapping configuration");
            }
            finally
            {
                if (!host.HasExited) host.Kill(entireProcessTree: true);
                await host.WaitForExitAsync();
                await output;
                await error;
            }
        }
        finally { Directory.Delete(temp, true); }
        Console.WriteLine($"PASS: {count} normalization and MQTT checks");
    }
    private static async Task WaitUntil(Func<bool> condition)
    {
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(15));
        while (!condition()) await Task.Delay(10, timeout.Token);
    }
}

// Only CONNECT, QoS1 PUBLISH/PUBACK, PING, DISCONNECT needed for this component test.
sealed class MqttPeer : IDisposable
{
    private readonly TcpListener listener = new(IPAddress.Loopback, 0);
    private readonly CancellationTokenSource stop = new();
    private readonly Task task;
    private TcpClient? active;
    public TaskCompletionSource Connected { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);
    public ConcurrentQueue<(byte Header, string Topic, byte[] Payload)> Messages { get; } = new();
    public int Connections;
    public int Port => ((IPEndPoint)listener.LocalEndpoint).Port;
    public MqttPeer() { listener.Start(); task = RunAsync(); }
    public void DropConnection() => active?.Dispose();
    private async Task RunAsync()
    {
        try
        {
            while (!stop.IsCancellationRequested)
            {
                using var client = await listener.AcceptTcpClientAsync(stop.Token);
                active = client;
                try
                {
                    var stream = client.GetStream();
                    while (!stop.IsCancellationRequested)
                    {
                        var first = new byte[1];
                        await stream.ReadExactlyAsync(first, stop.Token);
                        int length = 0, multiplier = 1;
                        byte next;
                        do
                        {
                            var b = new byte[1]; await stream.ReadExactlyAsync(b, stop.Token); next = b[0];
                            length += (next & 127) * multiplier; multiplier *= 128;
                        } while ((next & 128) != 0);
                        var body = new byte[length]; await stream.ReadExactlyAsync(body, stop.Token);
                        switch (first[0] >> 4)
                        {
                            case 1:
                                await stream.WriteAsync(new byte[] { 0x20, 2, 0, 0 }, stop.Token);
                                Interlocked.Increment(ref Connections); Connected.TrySetResult(); break;
                            case 3:
                                var size = BinaryPrimitives.ReadUInt16BigEndian(body);
                                var topic = System.Text.Encoding.UTF8.GetString(body, 2, size);
                                Messages.Enqueue((first[0], topic, body[(size + 4)..]));
                                await stream.WriteAsync(new byte[] { 0x40, 2, body[size + 2], body[size + 3] }, stop.Token); break;
                            case 12: await stream.WriteAsync(new byte[] { 0xd0, 0 }, stop.Token); break;
                            case 14: client.Dispose(); break;
                            default: throw new InvalidDataException("Unexpected MQTT packet in test peer.");
                        }
                    }
                }
                catch (Exception ex) when (ex is IOException or ObjectDisposedException or SocketException) { }
            }
        }
        catch (OperationCanceledException) when (stop.IsCancellationRequested) { }
    }
    public void Dispose()
    {
        stop.Cancel(); active?.Dispose(); listener.Stop();
        task.GetAwaiter().GetResult(); stop.Dispose();
    }
}
