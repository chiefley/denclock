using System.Buffers.Binary;

namespace LoRa.Service;

public sealed record GatewayMessage(byte Type, byte[] Payload);
public readonly record struct GatewayHeader(byte Type, ushort PayloadLength);

public static class GatewayProtocol
{
    public const byte Hello = 1;
    public const byte RxPacket = 2;

    public static GatewayHeader ParseHeader(ReadOnlySpan<byte> header)
    {
        if (header.Length != 3) throw new InvalidDataException("Common header must contain 3 bytes.");
        var type = header[0];
        var length = BinaryPrimitives.ReadUInt16BigEndian(header[1..]);
        switch (type)
        {
            case Hello when length != 1:
                throw new InvalidDataException("HELLO payload length must be 1.");
            case RxPacket when length < 2:
                throw new InvalidDataException("RX_PACKET must contain RSSI and SNR.");
            case Hello:
            case RxPacket:
                return new(type, length);
            default:
                throw new InvalidDataException($"Unsupported Gateway Message Type 0x{type:X2}.");
        }
    }

    // Read exactly one frame, regardless of TCP read boundaries. Leave subsequent
    // frames in the stream. The wire's uint16 length bounds allocation to 65535.
    public static async ValueTask<GatewayMessage?> ReadAsync(Stream stream, CancellationToken cancellationToken)
    {
        byte[] header = new byte[3];
        var first = await stream.ReadAsync(header.AsMemory(0, 1), cancellationToken);
        if (first == 0) return null; // Clean EOF only at a frame boundary.
        try
        {
            await stream.ReadExactlyAsync(header.AsMemory(1), cancellationToken);
            var parsed = ParseHeader(header);
            byte[] payload = new byte[parsed.PayloadLength];
            await stream.ReadExactlyAsync(payload, cancellationToken);
            return new(parsed.Type, payload);
        }
        catch (EndOfStreamException ex)
        {
            throw new InvalidDataException("Connection ended inside a Gateway Protocol message.", ex);
        }
    }

    public static byte ParseHello(ReadOnlySpan<byte> payload)
    {
        if (payload.Length != 1 || payload[0] == 0)
            throw new InvalidDataException("HELLO requires exactly one nonzero Gateway ID.");
        return payload[0];
    }

    public static ReceivedPacket ParseRx(byte gatewayId, ReadOnlySpan<byte> payload, DateTimeOffset receivedAt)
    {
        if (payload.Length < 2) throw new InvalidDataException("RX_PACKET is missing radio metadata.");
        return new(gatewayId, receivedAt, unchecked((sbyte)payload[0]),
            unchecked((sbyte)payload[1]) / 4m, payload[2..].ToArray());
    }
}

public sealed record ReceivedPacket(byte GatewayId, DateTimeOffset ReceivedAt,
    int RssiDbm, decimal SnrDb, byte[] RawPacket);
