using System.Text.Json;
using Microsoft.Extensions.Options;
using MQTTnet;
using MQTTnet.Protocol;

namespace LoRa.Service;

public sealed class MqttOptions
{
    public string Host { get; set; } = "localhost";
    public int Port { get; set; } = 1883;
    public string ClientId { get; set; } = "lora-service";
}

public interface IReportPublisher
{
    Task PublishAsync(NormalizedReport report, CancellationToken cancellationToken);
}

public sealed class MqttPublisher : BackgroundService, IReportPublisher
{
    private readonly IMqttClient client = new MqttClientFactory().CreateMqttClient();
    private readonly MqttClientOptions options;
    private readonly ILogger<MqttPublisher> logger;
    private readonly MqttOptions config;
    private static readonly JsonSerializerOptions JsonOptions = new(JsonSerializerDefaults.Web);
    private static readonly TimeSpan RetryInterval = TimeSpan.FromSeconds(5);
    private static readonly TimeSpan OperationTimeout = TimeSpan.FromSeconds(5);

    public MqttPublisher(IOptions<MqttOptions> settings, ILogger<MqttPublisher> logger)
    {
        this.logger = logger;
        config = settings.Value;
        options = new MqttClientOptionsBuilder().WithTcpServer(config.Host, config.Port)
            .WithClientId(config.ClientId).WithProtocolVersion(MQTTnet.Formatter.MqttProtocolVersion.V311).WithCleanSession().WithTimeout(OperationTimeout).Build();
        client.DisconnectedAsync += args =>
        {
            if (args.ClientWasConnected) logger.LogWarning("MQTT disconnected: {Reason}", args.Reason);
            return Task.CompletedTask;
        };
    }

    public static MqttApplicationMessage CreateMessage(NormalizedReport report) =>
        new MqttApplicationMessageBuilder().WithTopic($"lora/sensors/{report.GatewayId}/{report.NodeId}")
            .WithPayload(JsonSerializer.SerializeToUtf8Bytes(report, JsonOptions))
            .WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce).WithRetainFlag().Build();

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        try
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                if (!client.IsConnected)
                {
                    try
                    {
                        var result = await client.ConnectAsync(options, stoppingToken);
                        if (result.ResultCode == MqttClientConnectResultCode.Success)
                            logger.LogInformation("MQTT connected to {Host}:{Port}", config.Host, config.Port);
                        else logger.LogWarning("MQTT connection rejected: {Reason}", result.ResultCode);
                    }
                    catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested) { break; }
                    catch (Exception ex) { logger.LogWarning(ex, "MQTT connection failed; retrying"); }
                }
                await Task.Delay(RetryInterval, stoppingToken);
            }
        }
        catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested) { }
        finally
        {
            if (client.IsConnected)
            {
                using var timeout = new CancellationTokenSource(OperationTimeout);
                try { await client.DisconnectAsync(new MqttClientDisconnectOptions(), timeout.Token); }
                catch (Exception ex) { logger.LogWarning(ex, "MQTT disconnect failed during shutdown"); }
            }
        }
    }

    public async Task PublishAsync(NormalizedReport report, CancellationToken cancellationToken)
    {
        if (!client.IsConnected)
        {
            logger.LogWarning("MQTT unavailable; report discarded for Gateway {GatewayId} Node {NodeId}", report.GatewayId, report.NodeId);
            return;
        }
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        timeout.CancelAfter(OperationTimeout);
        try
        {
            var result = await client.PublishAsync(CreateMessage(report), timeout.Token);
            if (!result.IsSuccess) logger.LogWarning("MQTT publish rejected: {Reason}", result.ReasonCode);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { throw; }
        catch (Exception ex) { logger.LogWarning(ex, "MQTT publish failed; report discarded for Gateway {GatewayId} Node {NodeId}", report.GatewayId, report.NodeId); }
    }

    public override void Dispose()
    {
        base.Dispose();
        client.Dispose();
    }
}
