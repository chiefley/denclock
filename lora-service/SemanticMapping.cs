using System.Globalization;
using System.Text.Json;

namespace LoRa.Service;

public sealed record MeasurementDefinition(string Name, string ValueType, int Length,
    decimal Scale, string Units, int Precision);

public sealed class SemanticMapping
{
    public IReadOnlyDictionary<byte, string> Gateways { get; }
    public IReadOnlyDictionary<byte, string> Nodes { get; }
    public IReadOnlyDictionary<byte, MeasurementDefinition> Measurements { get; }

    private SemanticMapping(Dictionary<byte, string> gateways, Dictionary<byte, string> nodes,
        Dictionary<byte, MeasurementDefinition> measurements) =>
        (Gateways, Nodes, Measurements) = (gateways, nodes, measurements);

    public static SemanticMapping Load(string path)
    {
        try { return Parse(File.ReadAllText(path)); }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidDataException)
        { throw new InvalidDataException($"Semantic mapping '{path}': {ex.Message}", ex); }
    }

    public static SemanticMapping Parse(string json)
    {
        try
        {
            using var document = JsonDocument.Parse(json);
            var root = document.RootElement;
            RejectDuplicateProperties(root, "mapping");
            var gateways = ReadNames(root.GetProperty("gateways"), "gateways", 255);
            var nodes = ReadNames(root.GetProperty("nodes"), "nodes", 239);
            // A configured name must not steal another node's fallback topic.
            foreach (var (id, name) in nodes)
                for (var other = 1; other <= 239; other++)
                    if (other != id && name == $"node-{other}")
                        throw new InvalidDataException($"nodes.{id}.name collides with Node {other}'s fallback.");
            var measurements = new Dictionary<byte, MeasurementDefinition>();
            var names = new HashSet<string>(StringComparer.Ordinal);
            foreach (var entry in root.GetProperty("measurements").EnumerateObject())
            {
                var path = $"measurements.{entry.Name}";
                var id = ReadId(entry.Name, path, 255);
                var value = entry.Value;
                var name = RequiredText(value, "name", path);
                var type = RequiredText(value, "valueType", path);
                var length = value.GetProperty("length").GetInt32();
                var scale = value.GetProperty("scale").GetDecimal();
                var units = RequiredText(value, "units", path);
                var precision = value.TryGetProperty("precision", out var p) ? p.GetInt32() : 4;
                if (type is not ("int16" or "uint16") || length != 2 || scale == 0 || precision < 1)
                    throw new InvalidDataException($"{path}: expected int16/uint16, length 2, nonzero scale, positive precision.");
                try { _ = checked(scale * 65535m); }
                catch (OverflowException) { throw new InvalidDataException($"{path}.scale exceeds supported numeric range."); }
                // Protocol immutability checks belong here, not in semantic decoding.
                var expected = id switch
                {
                    1 => ("int16", 0.01m, "C"),
                    2 => ("uint16", 0.01m, "percent"),
                    3 => ("uint16", 0.001m, "V"),
                    _ => (type, scale, units)
                };
                if ((type, scale, units) != expected)
                    throw new InvalidDataException($"{path}: representation, scale or units redefine the LoRa protocol.");
                if (!names.Add(name) || !measurements.TryAdd(id, new(name, type, length, scale, units, precision)))
                    throw new InvalidDataException($"{path}: duplicate numeric ID or measurement name.");
            }
            return new(gateways, nodes, measurements);
        }
        catch (Exception ex) when (ex is JsonException or KeyNotFoundException or InvalidOperationException or FormatException or OverflowException)
        { throw new InvalidDataException($"Invalid semantic mapping: {ex.Message}", ex); }
    }

    private static Dictionary<byte, string> ReadNames(JsonElement element, string domain, int maximum)
    {
        var result = new Dictionary<byte, string>();
        var names = new HashSet<string>(StringComparer.Ordinal);
        foreach (var entry in element.EnumerateObject())
        {
            var path = $"{domain}.{entry.Name}";
            var id = ReadId(entry.Name, path, maximum);
            var name = RequiredText(entry.Value, "name", path);
            if (domain == "nodes" && name.IndexOfAny(['/', '+', '#', '\0']) >= 0)
                throw new InvalidDataException($"{path}.name must be one literal MQTT topic level.");
            if (!result.TryAdd(id, name) || !names.Add(name))
                throw new InvalidDataException($"{path}: duplicate ID or name.");
            if (entry.Value.TryGetProperty("description", out var description) && description.ValueKind != JsonValueKind.String)
                throw new InvalidDataException($"{path}.description must be text.");
        }
        return result;
    }

    private static byte ReadId(string text, string path, int maximum)
    {
        if (!byte.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out var id) || id == 0 || id > maximum)
            throw new InvalidDataException($"{path}: invalid numeric ID.");
        return id;
    }

    private static string RequiredText(JsonElement value, string field, string path)
    {
        if (!value.TryGetProperty(field, out var item) || item.ValueKind != JsonValueKind.String || string.IsNullOrWhiteSpace(item.GetString()))
            throw new InvalidDataException($"{path}.{field}: required nonempty text.");
        return item.GetString()!;
    }

    private static void RejectDuplicateProperties(JsonElement value, string path)
    {
        if (value.ValueKind != JsonValueKind.Object) return;
        var names = new HashSet<string>(StringComparer.Ordinal);
        foreach (var property in value.EnumerateObject())
        {
            if (!names.Add(property.Name)) throw new InvalidDataException($"{path}.{property.Name}: duplicate property.");
            RejectDuplicateProperties(property.Value, $"{path}.{property.Name}");
        }
    }
}
