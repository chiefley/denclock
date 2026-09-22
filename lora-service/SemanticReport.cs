using System.Buffers.Binary;
using System.Globalization;

namespace LoRa.Service;

public sealed record SemanticIdentity(byte Id, string Name);
public sealed record RadioMetadata(int RssiDbm, decimal SnrDb);
public sealed record SemanticMeasurement(decimal Value, string Units, int Precision);
public sealed record SemanticReport(SemanticIdentity Gateway, SemanticIdentity Node,
    string MessageType, ushort Sequence, DateTimeOffset ReceivedUtc, RadioMetadata Radio,
    IReadOnlyDictionary<string, SemanticMeasurement> Measurements);

public sealed class SemanticMapper(SemanticMapping mapping, ILogger<SemanticMapper> logger)
{
    public SemanticReport Map(SensorReport report, ReceivedPacket packet)
    {
        var gatewayName = Resolve(mapping.Gateways, packet.GatewayId,
            packet.GatewayId.ToString(CultureInfo.InvariantCulture), "Gateway");
        var nodeName = Resolve(mapping.Nodes, report.NodeId, $"node-{report.NodeId}", "Node");
        var values = new Dictionary<string, SemanticMeasurement>();
        foreach (var tlv in report.Measurements)
        {
            if (!mapping.Measurements.TryGetValue(tlv.Type, out var definition))
            {
                logger.LogWarning("Unknown TLV Type {Type} from Node {NodeId}; skipped", tlv.Type, report.NodeId);
                continue;
            }
            if (tlv.Value.Length != definition.Length)
                throw new InvalidDataException($"TLV {tlv.Type} length disagrees with mapping.");
            decimal raw = definition.ValueType == "int16"
                ? BinaryPrimitives.ReadInt16BigEndian(tlv.Value)
                : BinaryPrimitives.ReadUInt16BigEndian(tlv.Value);
            if (!values.TryAdd(definition.Name, new(raw * definition.Scale, definition.Units, definition.Precision)))
                throw new InvalidDataException($"Duplicate mapped measurement {definition.Name}.");
        }
        return new(new(packet.GatewayId, gatewayName), new(report.NodeId, nodeName),
            "sensor-report", report.Sequence, packet.ReceivedAt, new(packet.RssiDbm, packet.SnrDb), values);
    }

    private string Resolve(IReadOnlyDictionary<byte, string> names, byte id, string fallback, string kind)
    {
        if (names.TryGetValue(id, out var name)) return name;
        logger.LogWarning("Unknown {Kind} ID {Id}; using fallback {Fallback}", kind, id, fallback);
        return fallback;
    }
}
