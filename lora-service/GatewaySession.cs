namespace LoRa.Service;

public sealed class GatewaySession(ILogger logger)
{
    public async Task RunAsync(Stream stream, CancellationToken cancellationToken)
    {
        byte? gatewayId = null;
        var handler = new LoRaPacketHandler(logger);
        try
        {
            while (await GatewayProtocol.ReadAsync(stream, cancellationToken) is { } message)
            {
                // Capture completion time before application decoding or diagnostic output.
                var receivedAt = DateTimeOffset.UtcNow;
                if (gatewayId is null)
                {
                    if (message.Type != GatewayProtocol.Hello)
                        throw new InvalidDataException("First message must be HELLO.");
                    gatewayId = GatewayProtocol.ParseHello(message.Payload);
                    logger.LogInformation("Gateway {GatewayId} connected", gatewayId);
                    continue;
                }
                if (message.Type != GatewayProtocol.RxPacket)
                    throw new InvalidDataException("HELLO is only allowed at connection initialization.");
                var packet = GatewayProtocol.ParseRx(gatewayId.Value, message.Payload, receivedAt);
                logger.LogDebug("Gateway {GatewayId} RX_PACKET: {Length} LoRa bytes", gatewayId, packet.RawPacket.Length);
                handler.Handle(packet);
            }
        }
        catch (InvalidDataException ex)
        {
            logger.LogWarning("Gateway Protocol error (Gateway {GatewayId}): {Reason}", gatewayId, ex.Message);
        }
        finally
        {
            if (gatewayId.HasValue) logger.LogInformation("Gateway {GatewayId} disconnected", gatewayId);
        }
    }
}
