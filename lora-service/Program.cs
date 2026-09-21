using LoRa.Service;

var builder = Host.CreateApplicationBuilder(new HostApplicationBuilderSettings
{
    Args = args,
    ContentRootPath = AppContext.BaseDirectory
});
builder.Services.AddOptions<GatewayOptions>()
    .Bind(builder.Configuration.GetSection("Gateway"))
    .Validate(options => System.Net.IPAddress.TryParse(options.ListenAddress, out _),
        "Gateway:ListenAddress must be an IP address.")
    .Validate(options => options.Port is > 0 and <= 65535,
        "Gateway:Port must be between 1 and 65535.")
    .ValidateOnStart();
builder.Services.AddHostedService<GatewayWorker>();
await builder.Build().RunAsync();
