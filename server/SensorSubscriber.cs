using System.Buffers;
using Microsoft.Extensions.Options;
using MQTTnet;
using MQTTnet.Protocol;

namespace DenClock.Server;

public sealed class SensorMqttOptions
{
    public string Host { get; set; } = "localhost";
    public int Port { get; set; } = 1883;
    public string ClientId { get; set; } = "denclock-server";
}

public sealed class SensorSubscriber : BackgroundService
{
    private readonly IMqttClient client = new MqttClientFactory().CreateMqttClient();
    private readonly MqttClientOptions connection;
    private readonly ILogger<SensorSubscriber> logger;
    private readonly SensorMqttOptions settings;
    private readonly MqttClientSubscribeOptions subscription = new MqttClientSubscribeOptionsBuilder()
        .WithTopicFilter(f => f.WithTopic("lora/sensors/+/+").WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce)).Build();

    public SensorSubscriber(IOptions<SensorMqttOptions> options, PresentationState state, ILogger<SensorSubscriber> logger)
    {
        settings = options.Value;
        this.logger = logger;
        connection = new MqttClientOptionsBuilder().WithTcpServer(settings.Host, settings.Port)
            .WithClientId(settings.ClientId).WithProtocolVersion(MQTTnet.Formatter.MqttProtocolVersion.V311)
            .WithCleanSession().WithTimeout(TimeSpan.FromSeconds(5)).Build();
        client.ApplicationMessageReceivedAsync += args =>
        {
            state.Receive(args.ApplicationMessage.Topic, args.ApplicationMessage.Payload.ToArray());
            return Task.CompletedTask;
        };
        client.DisconnectedAsync += args =>
        {
            if (args.ClientWasConnected) logger.LogWarning("Sensor MQTT disconnected: {Reason}", args.Reason);
            return Task.CompletedTask;
        };
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        bool subscribed = false;
        try
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                try
                {
                    using var timeout = CancellationTokenSource.CreateLinkedTokenSource(stoppingToken);
                    timeout.CancelAfter(TimeSpan.FromSeconds(5));
                    if (!client.IsConnected)
                    {
                        subscribed = false;
                        var result = await client.ConnectAsync(connection, timeout.Token);
                        if (result.ResultCode != MqttClientConnectResultCode.Success)
                            throw new IOException($"MQTT connection rejected: {result.ResultCode}.");
                    }
                    if (!subscribed)
                    {
                        var result = await client.SubscribeAsync(subscription, timeout.Token);
                        if (result.Items.Any(item => item.ResultCode != MqttClientSubscribeResultCode.GrantedQoS1))
                            throw new IOException("MQTT subscription did not grant QoS 1.");
                        subscribed = true;
                        logger.LogInformation("Sensor MQTT subscribed at {Host}:{Port}", settings.Host, settings.Port);
                    }
                }
                catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested) { break; }
                catch (Exception ex) { logger.LogWarning("Sensor MQTT unavailable; retrying: {Reason}", ex.Message); }
                await Task.Delay(TimeSpan.FromSeconds(5), stoppingToken);
            }
        }
        catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested) { }
        finally
        {
            if (client.IsConnected)
            {
                using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
                try { await client.DisconnectAsync(new MqttClientDisconnectOptions(), timeout.Token); }
                catch (Exception ex) { logger.LogWarning("Sensor MQTT shutdown: {Reason}", ex.Message); }
            }
        }
    }

    public override void Dispose() { base.Dispose(); client.Dispose(); }
}
