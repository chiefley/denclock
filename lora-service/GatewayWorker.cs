using System.Net;
using System.Net.Sockets;
using Microsoft.Extensions.Options;

namespace LoRa.Service;

public sealed class GatewayOptions
{
    public string ListenAddress { get; set; } = "0.0.0.0";
    public int Port { get; set; } = 5210;
}

public sealed class GatewayWorker(IOptions<GatewayOptions> options, ILogger<GatewayWorker> logger) : BackgroundService
{
    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var listener = new TcpListener(IPAddress.Parse(options.Value.ListenAddress), options.Value.Port);
        var sessions = new List<Task>();
        using var sessionsCancellation = CancellationTokenSource.CreateLinkedTokenSource(stoppingToken);
        listener.Start();
        logger.LogInformation("LoRa Service listening on {Endpoint}", listener.LocalEndpoint);
        try
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                var client = await listener.AcceptTcpClientAsync(stoppingToken);
                sessions.RemoveAll(task => task.IsCompleted);
                sessions.Add(HandleClientAsync(client, sessionsCancellation.Token));
            }
        }
        catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested) { }
        finally
        {
            listener.Stop();
            await sessionsCancellation.CancelAsync();
            await Task.WhenAll(sessions);
        }
    }

    private async Task HandleClientAsync(TcpClient client, CancellationToken cancellationToken)
    {
        using (client)
        {
            var endpoint = client.Client.RemoteEndPoint;
            try
            {
                await new GatewaySession(logger).RunAsync(client.GetStream(), cancellationToken);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { }
            catch (IOException ex)
            {
                logger.LogWarning("Gateway connection {Endpoint} ended: {Reason}", endpoint, ex.Message);
            }
            catch (SocketException ex)
            {
                logger.LogWarning("Gateway connection {Endpoint} failed: {Reason}", endpoint, ex.Message);
            }
        }
    }
}
