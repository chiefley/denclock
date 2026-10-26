using System.Text.Json;
using System.Text.Json.Serialization;

namespace DenClock.Server;

public sealed record SensorConfiguration(string InternalName, string DisplayName, string Position, double MaxAgeSeconds)
{
    [JsonExtensionData]
    public Dictionary<string, JsonElement>? Extra { get; init; }
}
public sealed record PresentationConfiguration(string TemperatureUnit, IReadOnlyList<SensorConfiguration> Sensors)
{
    [JsonExtensionData]
    public Dictionary<string, JsonElement>? Extra { get; init; }
    public static PresentationConfiguration Load(string path)
    {
        try { return Parse(File.ReadAllText(path)); }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidDataException)
        { throw new InvalidDataException($"Presentation configuration '{path}': {ex.Message}", ex); }
    }

    public static PresentationConfiguration Parse(string json)
    {
        try
        {
            using var doc = JsonDocument.Parse(json);
            var root = doc.RootElement;
            RejectDuplicates(root);
            var unit = Text(root, "temperatureUnit");
            if (unit is not ("C" or "F")) throw new InvalidDataException("temperatureUnit must be C or F.");
            var sensors = new List<SensorConfiguration>();
            var names = new HashSet<string>(StringComparer.Ordinal);
            var positions = new HashSet<string>(StringComparer.Ordinal);
            foreach (var item in root.GetProperty("sensors").EnumerateArray())
            {
                var name = Text(item, "internalName");
                var label = Text(item, "displayName");
                var position = Text(item, "position");
                if (name.IndexOfAny(['/', '+', '#', '\0']) >= 0)
                    throw new InvalidDataException($"Sensor {name}: internalName must be one literal MQTT topic level.");
                if (!names.Add(name)) throw new InvalidDataException($"Duplicate internalName: {name}.");
                if (position is not ("upperLeft" or "upperRight" or "secondary"))
                    throw new InvalidDataException($"Sensor {name}: invalid position.");
                if (position != "secondary" && !positions.Add(position))
                    throw new InvalidDataException($"Duplicate primary position: {position}.");
                var age = item.GetProperty("maxAgeSeconds").GetDouble();
                if (!double.IsFinite(age) || age <= 0)
                    throw new InvalidDataException($"Sensor {name}: maxAgeSeconds must be positive and finite.");
                sensors.Add(new(name, label, position, age) {
                    Extra = Extras(item, "internalName", "displayName", "position", "maxAgeSeconds")
                });
            }
            return new(unit, sensors.AsReadOnly()) { Extra = Extras(root, "temperatureUnit", "sensors") };
        }
        catch (Exception ex) when (ex is JsonException or KeyNotFoundException or InvalidOperationException or FormatException)
        { throw new InvalidDataException($"Invalid presentation configuration: {ex.Message}", ex); }
    }

    private static Dictionary<string, JsonElement> Extras(JsonElement item, params string[] known) =>
        item.EnumerateObject().Where(p => !known.Contains(p.Name))
            .ToDictionary(p => p.Name, p => p.Value.Clone(), StringComparer.Ordinal);

    private static string Text(JsonElement item, string field)
    {
        if (!item.TryGetProperty(field, out var value) || value.ValueKind != JsonValueKind.String ||
            string.IsNullOrWhiteSpace(value.GetString()))
            throw new InvalidDataException($"{field} must be a nonempty string.");
        return value.GetString()!;
    }

    internal static void RejectDuplicates(JsonElement value)
    {
        if (value.ValueKind == JsonValueKind.Object)
        {
            var names = new HashSet<string>(StringComparer.Ordinal);
            foreach (var property in value.EnumerateObject())
            {
                if (!names.Add(property.Name)) throw new InvalidDataException($"Duplicate JSON field: {property.Name}.");
                RejectDuplicates(property.Value);
            }
        }
        else if (value.ValueKind == JsonValueKind.Array)
            foreach (var child in value.EnumerateArray()) RejectDuplicates(child);
    }
}
