using System.Text.Json;
using System.Text.Json.Serialization;

namespace DenClock.Server;

public sealed record SensorConfiguration(int GatewayId, int NodeId, string Name, string DisplayName, string Position, double MaxAgeSeconds)
{
    [JsonIgnore]
    public SensorIdentity Identity => new(GatewayId, NodeId);
    [JsonExtensionData]
    public Dictionary<string, JsonElement>? Extra { get; init; }
}
public readonly record struct SensorIdentity(int GatewayId, int NodeId);
public sealed record GatewayConfiguration(int GatewayId, string? Name, string? DisplayName)
{
    [JsonExtensionData]
    public Dictionary<string, JsonElement>? Extra { get; init; }
}
public sealed record PresentationConfiguration(string TemperatureUnit, IReadOnlyList<SensorConfiguration> Sensors)
{
    [JsonExtensionData]
    public Dictionary<string, JsonElement>? Extra { get; init; }
    public IReadOnlyList<GatewayConfiguration> Gateways { get; init; } = [];
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
            var identities = new HashSet<SensorIdentity>();
            var positions = new HashSet<string>(StringComparer.Ordinal);
            foreach (var item in root.GetProperty("sensors").EnumerateArray())
            {
                var name = Text(item, "name");
                var gatewayId = Id(item, "gatewayId", 255);
                var nodeId = Id(item, "nodeId", 239);
                if (!identities.Add(new(gatewayId, nodeId)))
                    throw new InvalidDataException($"Duplicate sensor identity: {gatewayId}/{nodeId}.");
                var label = Text(item, "displayName");
                var position = Text(item, "position");
                if (!names.Add(name)) throw new InvalidDataException($"Duplicate sensor name: {name}.");
                if (position is not ("upperLeft" or "upperRight" or "secondary"))
                    throw new InvalidDataException($"Sensor {name}: invalid position.");
                if (position != "secondary" && !positions.Add(position))
                    throw new InvalidDataException($"Duplicate primary position: {position}.");
                var age = item.GetProperty("maxAgeSeconds").GetDouble();
                if (!double.IsFinite(age) || age <= 0)
                    throw new InvalidDataException($"Sensor {name}: maxAgeSeconds must be positive and finite.");
                sensors.Add(new(gatewayId, nodeId, name, label, position, age) {
                    Extra = Extras(item, "gatewayId", "nodeId", "name", "displayName", "position", "maxAgeSeconds")
                });
            }
            var gateways = new List<GatewayConfiguration>();
            var gatewayIds = new HashSet<int>();
            var gatewayNames = new HashSet<string>(StringComparer.Ordinal);
            if (root.TryGetProperty("gateways", out var entries))
                foreach (var item in entries.EnumerateArray())
                {
                    var id = Id(item, "gatewayId", 255);
                    var name = OptionalText(item, "name");
                    if (!gatewayIds.Add(id) || (name is not null && !gatewayNames.Add(name)))
                        throw new InvalidDataException("Duplicate gateway identity or name.");
                    gateways.Add(new(id, name, OptionalText(item, "displayName")) {
                        Extra = Extras(item, "gatewayId", "name", "displayName")
                    });
                }
            return new(unit, sensors.AsReadOnly()) {
                Gateways = gateways.AsReadOnly(), Extra = Extras(root, "temperatureUnit", "sensors", "gateways")
            };
        }
        catch (Exception ex) when (ex is JsonException or KeyNotFoundException or InvalidOperationException or FormatException)
        { throw new InvalidDataException($"Invalid presentation configuration: {ex.Message}", ex); }
    }

    private static int Id(JsonElement item, string field, int maximum)
    {
        if (!item.TryGetProperty(field, out var value) || value.ValueKind != JsonValueKind.Number ||
            !value.TryGetInt32(out var id) || id < 1 || id > maximum)
            throw new InvalidDataException($"{field} must be an integer from 1 to {maximum}.");
        return id;
    }

    private static string? OptionalText(JsonElement item, string field)
    {
        if (!item.TryGetProperty(field, out var value) || value.ValueKind == JsonValueKind.Null) return null;
        return Text(item, field);
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
