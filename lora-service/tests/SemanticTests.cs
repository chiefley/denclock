using System.Buffers;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Net;
using System.Net.Sockets;
using System.Buffers.Binary;
using System.Collections.Concurrent;
using Microsoft.Extensions.Options;
using LoRa.Service;
using MQTTnet.Protocol;

static class SemanticTests
{
    public static async Task RunAsync()
    {
        var count = 0;
        void Check(bool condition, string name)
        {
            if (!condition) throw new Exception($"FAIL: {name}");
            count++;
        }
        void Invalid(string json, string name)
        {
            try { SemanticMapping.Parse(json); }
            catch (InvalidDataException ex) { Check(ex.Message.Length > 0, name); return; }
            throw new Exception($"FAIL: invalid mapping accepted: {name}");
        }
        var json = File.ReadAllText(Path.Combine(AppContext.BaseDirectory, "semantic-mapping.json"));
        var mapping = SemanticMapping.Parse(json);
        Check(mapping.Nodes[1] == "porch" && mapping.Gateways[1] == "den-gateway", "valid identities");
        Check(mapping.Measurements.Count == 3, "valid measurements");
        foreach (var malformed in new[] { "{", "[]", "{}", json.Replace("\"nodes\":", "\"nodes\":{},\"nodes\":"),
            json.Replace("\"valueType\": \"int16\"", "\"valueType\": \"float32\""),
            json.Replace("\"length\": 2", "\"length\": 1"), json.Replace("\"scale\": 0.01", "\"scale\": 0"),
            json.Replace("\"scale\": 0.01", "\"scale\": 1e100"), json.Replace("\"units\": \"C\"", "\"units\": \"F\""),
            json.Replace("\"name\": \"porch\"", "\"name\": \"a/b\""),
            json.Replace("\"name\": \"porch\"", "\"name\": \"node-37\""),
            json.Replace("\"name\": \"temperature\"", "\"name\": \"batteryVoltage\""),
            json.Replace("\"1\":", "\"0\":"), json.Replace("\"1\":", "\"256\":"),
            json.Replace("\"name\": \"temperature\",", ""), json.Replace("\"precision\": 4", "\"precision\": 0") })
            Invalid(malformed, "mapping validation");
        foreach (var domain in new[] { "gateways", "nodes", "measurements" })
        {
            var duplicate = JsonNode.Parse(json)!;
            duplicate[domain]!["01"] = duplicate[domain]!["1"]!.DeepClone();
            Invalid(duplicate.ToJsonString(), "numeric alias duplicate");
        }
        var badNode = JsonNode.Parse(json)!;
        badNode["nodes"]!["240"] = badNode["nodes"]!["1"]!.DeepClone();
        Invalid(badNode.ToJsonString(), "reserved node range");
        foreach (var (id, field, value) in new[] { ("1", "scale", "0.02"), ("2", "valueType", "\"int16\""), ("3", "scale", "0.01") })
        {
            var changed = JsonNode.Parse(json)!;
            changed["measurements"]![id]![field] = JsonNode.Parse(value);
            Invalid(changed.ToJsonString(), "immutable wire definition");
        }
        var defaults = JsonNode.Parse(json)!;
        defaults["measurements"]!["1"]!.AsObject().Remove("precision");
        Check(SemanticMapping.Parse(defaults.ToJsonString()).Measurements[1].Precision == 4, "default precision");
        var log = new CaptureLogger();
        var mapper = new SemanticMapper(mapping, log);
        byte[] bytes = [1, 1, 0x12, 0x34, 42, 1, 9, 1, 2, 0xfb, 0x1e, 2, 2, 0x10, 0xe1, 3, 2, 0xff, 0xff];
        var timestamp = DateTimeOffset.Parse("2026-09-21T20:42:17.123Z");
        var packet = new ReceivedPacket(1, timestamp, -72, -3m, bytes);
        var report = mapper.Map(SensorReportDecoder.Decode(bytes), packet);
        Check(report.Measurements.Count == 3 && log.Lines.Any(x => x.Contains("Unknown TLV Type 42")), "unknown skipped with warning");
        Check(report.Measurements["temperature"].Value == -12.50m, "signed big endian normalization");
        Check(report.Measurements["relativeHumidity"].Value == 43.21m, "humidity normalization");
        Check(report.Measurements["batteryVoltage"].Value == 65.535m, "unsigned and fidelity");
        Check(report.Measurements["batteryVoltage"].Precision == 4 && report.Measurements["batteryVoltage"].Units == "V", "units precision metadata");
        Check(report.Gateway == new SemanticIdentity(1, "den-gateway") && report.Node == new SemanticIdentity(1, "porch"), "configured identity");
        var fallback = mapper.Map(SensorReportDecoder.Decode([1, 37, 0, 0]), packet with { GatewayId = 37 });
        Check(fallback.Gateway.Name == "37" && fallback.Node.Name == "node-37", "fallbacks");
        Check(log.Lines.Any(x => x.Contains("Unknown Gateway ID 37")) && log.Lines.Any(x => x.Contains("Unknown Node ID 37")), "identity warnings");
        // A new configured type proves the general path uses config, not named fields.
        var extra = JsonNode.Parse(json)!;
        extra["measurements"]!["42"] = JsonNode.Parse("""{"name":"extra","valueType":"uint16","length":2,"scale":0.125,"units":"custom","precision":2}""");
        var extraMapper = new SemanticMapper(SemanticMapping.Parse(extra.ToJsonString()), log);
        Check(extraMapper.Map(SensorReportDecoder.Decode([1, 1, 0, 0, 42, 2, 1, 1]), packet).Measurements["extra"].Value == 32.125m, "configured new type without rounding");
        var missing = JsonNode.Parse(json)!;
        missing["measurements"]!.AsObject().Remove("1");
        var missingMapper = new SemanticMapper(SemanticMapping.Parse(missing.ToJsonString()), log);
        Check(missingMapper.Map(SensorReportDecoder.Decode(bytes), packet).Measurements.Count == 2, "unmapped assigned type skipped");
        try
        {
            extraMapper.Map(SensorReportDecoder.Decode([1, 1, 0, 0, 42, 1, 9]), packet);
            throw new Exception("FAIL: mapped length accepted");
        }
        catch (InvalidDataException) { count++; }
        var publisher = new CapturePublisher();
        var handler = new LoRaPacketHandler(mapper, publisher, log);
        await handler.HandleAsync(packet, default);
        Check(publisher.Reports.Count == 1, "one report one publish");
        var message = MqttPublisher.CreateMessage(report);
        Check(message.Topic == "lora/sensors/porch", "topic");
        Check(message.Retain && message.QualityOfServiceLevel == MqttQualityOfServiceLevel.AtLeastOnce, "QoS and retain");
        using var payload = JsonDocument.Parse(message.Payload);
        var root = payload.RootElement;
        Check(root.GetProperty("gateway").GetProperty("id").GetInt32() == 1 && root.GetProperty("gateway").GetProperty("name").GetString() == "den-gateway", "gateway JSON");
        Check(root.GetProperty("node").GetProperty("id").GetInt32() == 1 && root.GetProperty("node").GetProperty("name").GetString() == "porch", "node JSON");
        Check(root.GetProperty("messageType").GetString() == "sensor-report" && root.GetProperty("sequence").GetInt32() == 0x1234, "type and sequence JSON");
        Check(root.GetProperty("receivedUtc").GetDateTimeOffset() == timestamp, "timestamp preserved");
        Check(root.GetProperty("radio").GetProperty("rssiDbm").GetInt32() == -72 && root.GetProperty("radio").GetProperty("snrDb").GetDecimal() == -3m, "radio preserved");
        Check(root.GetProperty("measurements").GetProperty("batteryVoltage").GetProperty("value").ValueKind == JsonValueKind.Number, "numeric JSON");
        Check(root.EnumerateObject().Count() == 7, "only contract envelope fields");
        Check(MqttPublisher.CreateMessage(fallback).Topic == "lora/sensors/node-37", "fallback topic");
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
        Check(wire!.Payload.SequenceEqual(message.Payload.ToArray()), "wire semantic payload");
        peer.DropConnection();
        await WaitUntil(() => log.Lines.Any(x => x.Contains("MQTT disconnected")));
        await mqtt.PublishAsync(report, default); // Discard while disconnected; never replay.
        await WaitUntil(() => peer.Connections >= 2 && log.Lines.Count(x => x.Contains("MQTT connected")) >= 2);
        Check(peer.Messages.Count == 1, "no outage replay");
        await mqtt.PublishAsync(report, default);
        await WaitUntil(() => peer.Messages.Count == 2);
        Check(peer.Messages.Count == 2, "publish after reconnect");
        using var stop = new CancellationTokenSource(TimeSpan.FromSeconds(10));
        await mqtt.StopAsync(stop.Token);
        Check(mqtt.ExecuteTask!.IsCompletedSuccessfully, "MQTT graceful stop");
        // Exercise the real host's startup failure before any listener starts.
        var temp = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(temp);
        try
        {
            var invalidPath = Path.Combine(temp, "invalid.json");
            await File.WriteAllTextAsync(invalidPath, "{}");
            var start = new System.Diagnostics.ProcessStartInfo("dotnet")
            {
                WorkingDirectory = temp, RedirectStandardOutput = true, RedirectStandardError = true
            };
            start.ArgumentList.Add(Path.Combine(AppContext.BaseDirectory, "LoRa.Service.dll"));
            start.ArgumentList.Add($"--SemanticMapping:Path={invalidPath}");
            start.ArgumentList.Add($"--Logging:FilePath={Path.Combine(temp, "service-.log")}");
            using var host = System.Diagnostics.Process.Start(start)!;
            var outputTask = host.StandardOutput.ReadToEndAsync();
            var errorTask = host.StandardError.ReadToEndAsync();
            await host.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(15));
            var output = await outputTask + await errorTask;
            Check(host.ExitCode != 0 && output.Contains("Semantic mapping"), "invalid mapping stops real host with diagnostic");
            Check(!output.Contains("LoRa Service listening"), "mapping validated before listener startup");
            var files = Directory.GetFiles(temp, "service-*.log");
            Check(files.Length == 1 && File.ReadAllText(files[0]).Contains("Semantic mapping"), "daily file receives startup failure");
        }
        finally { Directory.Delete(temp, true); }
        Console.WriteLine($"PASS: {count} semantic mapping and MQTT checks");
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
