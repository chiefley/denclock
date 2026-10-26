using System.Text.Json;

namespace DenClock.Server;

public sealed record SensorObservation(DateTimeOffset ReceivedUtc, double? TemperatureC, double? RelativeHumidity);
public sealed record PresentationSlot(string Position, string? InternalName, string DisplayName,
    double? MaxAgeSeconds, SensorObservation? Observation);
public sealed record PresentationSnapshot(DateTimeOffset ServerUtc, string TemperatureUnit, IReadOnlyList<PresentationSlot> Slots);

public sealed class PresentationState(PresentationConfiguration configuration, ILogger<PresentationState> logger)
{
    private readonly object gate = new();
    private readonly Dictionary<string, SensorObservation> observations = new(StringComparer.Ordinal);
    private PresentationConfiguration configuration = configuration;
    private HashSet<string> configured = configuration.Sensors.Select(s => s.InternalName).ToHashSet(StringComparer.Ordinal);

    public void ReplaceConfiguration(PresentationConfiguration next)
    {
        lock (gate)
        {
            var names = next.Sensors.Select(s => s.InternalName).ToHashSet(StringComparer.Ordinal);
            foreach (var name in observations.Keys.Where(name => !names.Contains(name)).ToArray())
                observations.Remove(name);
            configured = names;
            configuration = next;
        }
    }

    public bool Receive(string topic, ReadOnlyMemory<byte> payload)
    {
        const string prefix = "lora/sensors/";
        if (!topic.StartsWith(prefix, StringComparison.Ordinal)) return false;
        var name = topic[prefix.Length..];
        lock (gate) { if (!configured.Contains(name)) return false; }
        try
        {
            using var doc = JsonDocument.Parse(payload);
            var root = doc.RootElement;
            PresentationConfiguration.RejectDuplicates(root);
            if (root.GetProperty("messageType").GetString() != "sensor-report" ||
                root.GetProperty("node").GetProperty("name").GetString() != name)
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
                if (!configured.Contains(name)) return false;
                if (observations.TryGetValue(name, out var previous) && received <= previous.ReceivedUtc) return false;
                observations[name] = observation;
            }
            return true;
        }
        catch (Exception ex) when (ex is JsonException or InvalidDataException or InvalidOperationException or KeyNotFoundException or FormatException)
        {
            logger.LogWarning("Ignoring invalid MQTT report for {Sensor}: {Reason}", name, ex.Message);
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
                if (sensor is null) return new PresentationSlot(position, null, "", null, null);
                observations.TryGetValue(sensor.InternalName, out var observation);
                return new PresentationSlot(position, sensor.InternalName, sensor.DisplayName, sensor.MaxAgeSeconds, observation);
            }).ToArray();
            return new(DateTimeOffset.UtcNow, configuration.TemperatureUnit, slots);
        }
    }
}
