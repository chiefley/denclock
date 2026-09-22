using System.Buffers.Binary;

namespace LoRa.Service;

public sealed record MeasurementTlv(byte Type, byte[] Value);
public sealed record SensorReport(byte NodeId, ushort Sequence, IReadOnlyList<MeasurementTlv> Measurements);

public static class SensorReportDecoder
{
    public static SensorReport Decode(ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 4) throw new InvalidDataException("SENSOR_REPORT fixed header is truncated.");
        if (packet[0] != 1) throw new InvalidDataException("Expected SENSOR_REPORT Message Class.");
        if (packet[1] is 0 or >= 0xf0) throw new InvalidDataException("Reserved Source Node ID.");
        var sequence = BinaryPrimitives.ReadUInt16BigEndian(packet[2..]);
        var measurements = new List<MeasurementTlv>();
        var known = new HashSet<byte>();
        for (var position = 4; position < packet.Length;)
        {
            if (packet.Length - position < 2) throw new InvalidDataException("Truncated TLV header.");
            var type = packet[position++];
            var length = packet[position++];
            if (type == 0) throw new InvalidDataException("TLV Type zero is invalid.");
            if (length > packet.Length - position) throw new InvalidDataException("TLV extends beyond packet.");
            var value = packet.Slice(position, length);
            // Preserve accepted wire validation independently of configured mapping.
            if (type is >= 1 and <= 3)
            {
                if (length != 2) throw new InvalidDataException("Known measurement TLV length must be 2.");
                if (!known.Add(type)) throw new InvalidDataException("Duplicate known measurement TLV.");
                if (type == 2 && BinaryPrimitives.ReadUInt16BigEndian(value) > 10000)
                    throw new InvalidDataException("Relative humidity exceeds 100%.");
            }
            measurements.Add(new(type, value.ToArray()));
            position += length;
        }
        return new(packet[1], sequence, measurements);
    }
}
