using System.Buffers.Binary;

namespace LoRa.Service;

public sealed record RadioMetadata(int RssiDbm, decimal SnrDb);
public sealed record NormalizedMeasurement(decimal Value, string Units, int Precision);
public sealed record NormalizedReport(int GatewayId, int NodeId,
    string MessageType, ushort Sequence, DateTimeOffset ReceivedUtc, RadioMetadata Radio,
    IReadOnlyDictionary<string, NormalizedMeasurement> Measurements);

public sealed class ReportNormalizer(ILogger<ReportNormalizer> logger)
{
    public NormalizedReport Normalize(SensorReport report, ReceivedPacket packet)
    {
        var values = new Dictionary<string, NormalizedMeasurement>();
        foreach (var tlv in report.Measurements)
        {
            // Assigned measurement meanings are fixed by the LoRa protocol.
            switch (tlv.Type)
            {
                case 1:
                    values.Add("temperature", new(BinaryPrimitives.ReadInt16BigEndian(tlv.Value) * 0.01m, "C", 4));
                    break;
                case 2:
                    values.Add("relativeHumidity", new(BinaryPrimitives.ReadUInt16BigEndian(tlv.Value) * 0.01m, "percent", 4));
                    break;
                case 3:
                    values.Add("batteryVoltage", new(BinaryPrimitives.ReadUInt16BigEndian(tlv.Value) * 0.001m, "V", 4));
                    break;
                default:
                    logger.LogWarning("Unknown TLV Type {Type} from Gateway {GatewayId} Node {NodeId}; skipped",
                        tlv.Type, packet.GatewayId, report.NodeId);
                    break;
            }
        }
        return new(packet.GatewayId, report.NodeId, "sensor-report", report.Sequence,
            packet.ReceivedAt, new(packet.RssiDbm, packet.SnrDb), values);
    }
}
