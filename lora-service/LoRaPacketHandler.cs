namespace LoRa.Service;

public sealed class LoRaPacketHandler(SemanticMapper mapper, IReportPublisher publisher, ILogger<LoRaPacketHandler> logger)
{
    public async Task HandleAsync(ReceivedPacket packet, CancellationToken cancellationToken)
    {
        SemanticReport report;
        try
        {
            if (packet.RawPacket.Length == 0) throw new InvalidDataException("Empty LoRa packet.");
            if (packet.RawPacket[0] != 1)
                throw new InvalidDataException($"Unsupported LoRa Message Class 0x{packet.RawPacket[0]:X2}.");
            report = mapper.Map(SensorReportDecoder.Decode(packet.RawPacket), packet);
        }
        catch (InvalidDataException ex)
        {
            logger.LogWarning("LoRa protocol error from Gateway {GatewayId}: {Reason}", packet.GatewayId, ex.Message);
            return;
        }
        await publisher.PublishAsync(report, cancellationToken);
    }
}
