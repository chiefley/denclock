using System.Globalization;

namespace LoRa.Service;

public sealed class LoRaPacketHandler(ILogger logger)
{
    public void Handle(ReceivedPacket packet)
    {
        try
        {
            if (packet.RawPacket.Length == 0) throw new InvalidDataException("Empty LoRa packet.");
            // Dispatch remains separate from individual message decoders. Future
            // message classes can add their own handler without changing SENSOR_REPORT.
            switch (packet.RawPacket[0])
            {
                case 1:
                    var report = SensorReportDecoder.Decode(packet.RawPacket);
                    logger.LogInformation(
                        "Gateway {GatewayId} UTC {ReceivedAt:O} Node {NodeId} Sequence {Sequence} " +
                        "Temperature {Temperature} C Relative Humidity {Humidity} % Battery {Battery} V " +
                        "RSSI {Rssi} dBm SNR {Snr} dB",
                        packet.GatewayId, packet.ReceivedAt, report.NodeId, report.Sequence,
                        Format(report.TemperatureC, "F2"), Format(report.RelativeHumidityPercent, "F2"),
                        Format(report.BatteryVolts, "F3"), packet.RssiDbm, Format(packet.SnrDb, "F2"));
                    break;
                default:
                    throw new InvalidDataException($"Unsupported LoRa Message Class 0x{packet.RawPacket[0]:X2}.");
            }
        }
        catch (InvalidDataException ex)
        {
            logger.LogWarning("LoRa protocol error from Gateway {GatewayId}: {Reason}", packet.GatewayId, ex.Message);
        }
    }

    private static string Format(decimal? value, string format) =>
        value?.ToString(format, CultureInfo.InvariantCulture) ?? "N/A";
}
