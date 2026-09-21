using System.Buffers.Binary;

namespace LoRa.Service;

public sealed record SensorReport(byte NodeId, ushort Sequence,
    decimal? TemperatureC, decimal? RelativeHumidityPercent, decimal? BatteryVolts);

public static class SensorReportDecoder
{
    public static SensorReport Decode(ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 4) throw new InvalidDataException("SENSOR_REPORT fixed header is truncated.");
        if (packet[0] != 1) throw new InvalidDataException("Expected SENSOR_REPORT Message Class.");
        if (packet[1] is 0 or >= 0xf0) throw new InvalidDataException("Reserved Source Node ID.");
        var sequence = BinaryPrimitives.ReadUInt16BigEndian(packet[2..]);
        decimal? temperature = null, humidity = null, battery = null;
        for (var position = 4; position < packet.Length;)
        {
            if (packet.Length - position < 2) throw new InvalidDataException("Truncated TLV header.");
            var type = packet[position++];
            var length = packet[position++];
            if (type == 0) throw new InvalidDataException("TLV Type zero is invalid.");
            if (length > packet.Length - position) throw new InvalidDataException("TLV extends beyond packet.");
            var value = packet.Slice(position, length);
            if (type is >= 1 and <= 3 && length != 2)
                throw new InvalidDataException("Known measurement TLV length must be 2.");
            switch (type)
            {
                case 1:
                    if (temperature.HasValue) throw new InvalidDataException("Duplicate temperature TLV.");
                    temperature = BinaryPrimitives.ReadInt16BigEndian(value) / 100m;
                    break;
                case 2:
                    if (humidity.HasValue) throw new InvalidDataException("Duplicate humidity TLV.");
                    humidity = BinaryPrimitives.ReadUInt16BigEndian(value) / 100m;
                    if (humidity > 100) throw new InvalidDataException("Relative humidity exceeds 100%.");
                    break;
                case 3:
                    if (battery.HasValue) throw new InvalidDataException("Duplicate battery TLV.");
                    battery = BinaryPrimitives.ReadUInt16BigEndian(value) / 1000m;
                    break;
                // Unknown nonzero types are skipped using their declared length.
            }
            position += length;
        }
        return new(packet[1], sequence, temperature, humidity, battery);
    }
}
