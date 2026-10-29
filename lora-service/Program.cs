using LoRa.Service;
using Microsoft.Extensions.Hosting.WindowsServices;
using Serilog;

// Bootstrap diagnostics cover configuration failures before the host exists.
Log.Logger = new LoggerConfiguration().WriteTo.Console().CreateLogger();
try
{
    var builder = Host.CreateApplicationBuilder(new HostApplicationBuilderSettings
    {
        Args = args,
        ContentRootPath = AppContext.BaseDirectory
    });
    builder.Services.AddWindowsService();
    var logPath = Path.GetFullPath(builder.Configuration["Logging:FilePath"] ?? "logs/lora-service-.log", AppContext.BaseDirectory);
    builder.Logging.ClearProviders();
    builder.Services.AddSerilog((services, config) =>
    {
        config.MinimumLevel.Information().Enrich.FromLogContext()
            .WriteTo.File(logPath, rollingInterval: RollingInterval.Day,
                retainedFileCountLimit: null, retainedFileTimeLimit: TimeSpan.FromDays(7));
        if (!WindowsServiceHelpers.IsWindowsService()) config.WriteTo.Console();
    });
    builder.Services.AddSingleton<ReportNormalizer>();
    builder.Services.AddSingleton<LoRaPacketHandler>();
    builder.Services.AddSingleton<GatewaySession>();
    builder.Services.AddOptions<GatewayOptions>()
        .Bind(builder.Configuration.GetSection("Gateway"))
        .Validate(options => System.Net.IPAddress.TryParse(options.ListenAddress, out _), "Gateway:ListenAddress must be an IP address.")
        .Validate(options => options.Port is > 0 and <= 65535, "Gateway:Port must be between 1 and 65535.")
        .ValidateOnStart();
    builder.Services.AddOptions<MqttOptions>()
        .Bind(builder.Configuration.GetSection("Mqtt"))
        .Validate(options => !string.IsNullOrWhiteSpace(options.Host), "Mqtt:Host is required.")
        .Validate(options => options.Port is > 0 and <= 65535, "Mqtt:Port must be between 1 and 65535.")
        .Validate(options => !string.IsNullOrWhiteSpace(options.ClientId), "Mqtt:ClientId is required.")
        .ValidateOnStart();
    builder.Services.AddSingleton<MqttPublisher>();
    builder.Services.AddSingleton<IReportPublisher>(services => services.GetRequiredService<MqttPublisher>());
    builder.Services.AddHostedService(services => services.GetRequiredService<MqttPublisher>());
    builder.Services.AddHostedService<GatewayWorker>();
    using var host = builder.Build();
    var logger = host.Services.GetRequiredService<ILogger<ReportNormalizer>>();
    try
    {
        await host.RunAsync();
    }
    catch (Exception ex)
    {
        logger.LogCritical(ex, "LoRa Service startup or execution failed");
        Environment.ExitCode = 1;
    }
}
catch (Exception ex)
{
    Log.Fatal(ex, "LoRa Service host configuration failed");
    Environment.ExitCode = 1;
}
finally { await Log.CloseAndFlushAsync(); }
