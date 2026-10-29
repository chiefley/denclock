using System.Text.Json;

namespace DenClock.Server;

public sealed record SensorObservation(DateTimeOffset ReceivedUtc, double? TemperatureC, double? RelativeHumidity);
public sealed record PresentationSlot(string Position, int? GatewayId, int? NodeId, string? Name, string DisplayName,
    double? MaxAgeSeconds, SensorObservation? Observation);
public sealed record PresentationSnapshot(DateTimeOffset ServerUtc, string TemperatureUnit, IReadOnlyList<PresentationSlot> Slots);

public sealed class PresentationState(PresentationConfiguration configuration, ILogger<PresentationState> logger)
{
    private readonly object gate = new();
    private readonly Dictionary<SensorIdentity, SensorObservation> observations = new();
    private PresentationConfiguration configuration = configuration;
    private HashSet<SensorIdentity> configured = configuration.Sensors.Select(s => s.Identity).ToHashSet();

    public void ReplaceConfiguration(PresentationConfiguration next)
    {
        lock (gate)
        {
            var identities = next.Sensors.Select(s => s.Identity).ToHashSet();
            foreach (var identity in observations.Keys.Where(identity => !identities.Contains(identity)).ToArray())
                observations.Remove(identity);
            configured = identities;
            configuration = next;
        }
    }

    public bool Receive(string topic, ReadOnlyMemory<byte> payload)
    {
        const string prefix = "lora/sensors/";
        if (!topic.StartsWith(prefix, StringComparison.Ordinal)) return false;
        var segments = topic[prefix.Length..].Split('/');
        if (segments.Length != 2 || !int.TryParse(segments[0], out var gatewayId) ||
            !int.TryParse(segments[1], out var nodeId) || gatewayId is < 1 or > 255 || nodeId is < 1 or > 239 ||
            topic != $"{prefix}{gatewayId}/{nodeId}") return false;
        var identity = new SensorIdentity(gatewayId, nodeId);
        lock (gate) { if (!configured.Contains(identity)) return false; }
        try
        {
            using var doc = JsonDocument.Parse(payload);
            var root = doc.RootElement;
            PresentationConfiguration.RejectDuplicates(root);
            if (root.GetProperty("messageType").GetString() != "sensor-report" ||
                root.GetProperty("gatewayId").GetInt32() != gatewayId ||
                root.GetProperty("nodeId").GetInt32() != nodeId)
                throw new InvalidDataException("Topic and sensor-report identity must agree.");
            var stamp = root.GetProperty("receivedUtc");
            // Require an explicit UTC designator, not a host-local timestamp.
            var text = stamp.GetString() ?? "";
            if (!(text.EndsWith("Z", StringComparison.Ordinal) || text.EndsWith("+00:00", StringComparison.Ordinal)) ||
                !stamp.TryGetDateTimeOffset(out var received) || received > DateTimeOffset.UtcNow)
                throw new InvalidDataException("receivedUtc must be a valid, nonfuture UTC timestamp.");
            var measurements = root.GetProperty("measurements");
            if (measurements.ValueKind != JsonValueKind.Object)
                throw new InvalidDataException("measurements must be an object.");
            var temperature = Measurement(measurements, "temperature", "C");
            var humidity = Measurement(measurements, "relativeHumidity", "percent");
            if (humidity is < 0 or > 100) humidity = null;
            // Keep the canonical value, but require safe presentation in either unit.
            if (temperature.HasValue && !double.IsFinite(temperature.Value * 1.8 + 32)) temperature = null;
            var observation = new SensorObservation(received, temperature, humidity);
            lock (gate)
            {
                if (!configured.Contains(identity)) return false;
                if (observations.TryGetValue(identity, out var previous) && received <= previous.ReceivedUtc) return false;
                observations[identity] = observation;
            }
            return true;
        }
        catch (Exception ex) when (ex is JsonException or InvalidDataException or InvalidOperationException or KeyNotFoundException or FormatException)
        {
            logger.LogWarning("Ignoring invalid MQTT report for {Sensor}: {Reason}", identity, ex.Message);
            return false;
        }
    }

    private static double? Measurement(JsonElement measurements, string name, string units)
    {
        if (!measurements.TryGetProperty(name, out var item) || item.ValueKind != JsonValueKind.Object ||
            !item.TryGetProperty("units", out var unit) || unit.ValueKind != JsonValueKind.String || unit.GetString() != units ||
            !item.TryGetProperty("value", out var value) || value.ValueKind != JsonValueKind.Number ||
            !value.TryGetDouble(out var number) || !double.IsFinite(number)) return null;
        return number;
    }

    public PresentationSnapshot Snapshot()
    {
        lock (gate)
        {
            var slots = new[] { "upperLeft", "upperRight" }.Select(position =>
            {
                var sensor = configuration.Sensors.SingleOrDefault(s => s.Position == position);
                if (sensor is null) return new PresentationSlot(position, null, null, null, "", null, null);
                observations.TryGetValue(sensor.Identity, out var observation);
                return new PresentationSlot(position, sensor.GatewayId, sensor.NodeId, sensor.Name, sensor.DisplayName, sensor.MaxAgeSeconds, observation);
            }).ToArray();
            return new(DateTimeOffset.UtcNow, configuration.TemperatureUnit, slots);
        }
    }
}
