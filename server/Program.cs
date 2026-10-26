using DenClock.Server;

var builder = WebApplication.CreateBuilder(new WebApplicationOptions
{
    Args = args,
    // Windows Services may start with System32 as their working directory.
    ContentRootPath = AppContext.BaseDirectory
});

builder.Services.AddWindowsService(options => options.ServiceName = "DenClock.Server");
var settingsDirectory = builder.Configuration["Presentation:ConfigurationDirectory"];
if (string.IsNullOrWhiteSpace(settingsDirectory))
{
    if (builder.Environment.IsDevelopment() || !OperatingSystem.IsWindows())
        throw new InvalidOperationException("Set Presentation:ConfigurationDirectory to an isolated directory for development/tests.");
    settingsDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "DenClock", "Server");
}
var settings = new PresentationSettings(settingsDirectory, Path.Combine(AppContext.BaseDirectory, "presentation.json"));
builder.Services.AddSingleton(settings);
builder.Services.AddSingleton(settings.Read().Configuration);
builder.Services.AddSingleton<PresentationState>();
builder.Services.AddOptions<SensorMqttOptions>().Bind(builder.Configuration.GetSection("Mqtt"))
    .Validate(o => !string.IsNullOrWhiteSpace(o.Host), "Mqtt:Host is required.")
    .Validate(o => o.Port is > 0 and <= 65535, "Mqtt:Port must be 1-65535.")
    .Validate(o => !string.IsNullOrWhiteSpace(o.ClientId), "Mqtt:ClientId is required.")
    .ValidateOnStart();
builder.Services.AddHostedService<SensorSubscriber>();

if (builder.Environment.IsDevelopment())
{
    // dotnet run uses the project directory; an explicit path supports other launchers.
    var clientRoot = Path.GetFullPath(
        builder.Configuration["Client:SourcePath"] ?? "../client",
        Directory.GetCurrentDirectory());
    if (!File.Exists(Path.Combine(clientRoot, "index.html")))
        throw new InvalidOperationException("Client:SourcePath must point to the client source directory.");
    builder.Environment.WebRootPath = clientRoot;
    builder.Environment.WebRootFileProvider = new Microsoft.Extensions.FileProviders.PhysicalFileProvider(clientRoot);
}

var developmentOrigins = builder.Configuration.GetSection("Cors:DevelopmentOrigins").Get<string[]>() ?? [];
builder.Services.AddCors(options => options.AddPolicy("DevelopmentClient", policy =>
{
    if (developmentOrigins.Length > 0)
        policy.WithOrigins(developmentOrigins).WithMethods("GET");
}));
builder.Services.AddCors(options => options.AddPolicy("SettingsClient", policy =>
{
    if (developmentOrigins.Length > 0)
        policy.WithOrigins(developmentOrigins).WithMethods("GET", "PUT").WithHeaders("Content-Type");
}));

var app = builder.Build();

app.UseCors();
app.UseDefaultFiles();
app.UseStaticFiles(new StaticFileOptions
{
    OnPrepareResponse = context =>
    {
        if (app.Environment.IsDevelopment())
            context.Context.Response.Headers.CacheControl = "no-store";
    }
});

// Keep the deployed client same-origin regardless of development profile settings.
app.MapGet("/client-settings.js", (HttpResponse response) =>
{
    response.Headers.CacheControl = "no-store";
    var address = app.Environment.IsDevelopment()
        ? app.Configuration["Client:TimeServerAddress"] ?? ""
        : "";
    return Results.Text($"export const serverAddress = {System.Text.Json.JsonSerializer.Serialize(address)};", "text/javascript");
});

app.MapGet("/api/time", (HttpResponse response) =>
{
    response.Headers.CacheControl = "no-store";

    var utcNow = DateTimeOffset.UtcNow;
    var timeZone = TimeZoneInfo.Local;

    return Results.Ok(new
    {
        utcTime = utcNow,
        unixTimeMilliseconds = utcNow.ToUnixTimeMilliseconds(),
        timeZoneId = timeZone.Id,
        utcOffsetSeconds = (int)timeZone.GetUtcOffset(utcNow).TotalSeconds
    });
}).RequireCors("DevelopmentClient");

app.MapGet("/api/presentation", (HttpResponse response, PresentationState state) =>
{
    response.Headers.CacheControl = "no-store";
    return Results.Ok(state.Snapshot());
}).RequireCors("DevelopmentClient");

app.MapGet("/api/settings/presentation", (HttpResponse response, PresentationSettings store) =>
{
    response.Headers.CacheControl = "no-store";
    return Results.Ok(store.Read());
}).RequireCors("SettingsClient");

app.MapPut("/api/settings/presentation", async (HttpRequest request, HttpResponse response,
    PresentationSettings store, PresentationState state, ILogger<PresentationSettings> logger) =>
{
    response.Headers.CacheControl = "no-store";
    var origin = request.Headers.Origin.ToString();
    var servedOrigin = $"{request.Scheme}://{request.Host}";
    if (request.Headers.ContainsKey("Origin") &&
        !string.Equals(origin, servedOrigin, StringComparison.OrdinalIgnoreCase) &&
        !developmentOrigins.Contains(origin, StringComparer.Ordinal))
        return Results.Json(new { error = "This browser origin is not allowed to save settings." }, statusCode: 403);
    if (!request.HasJsonContentType())
        return Results.Json(new { error = "Settings updates require application/json." }, statusCode: 415);
    try
    {
        using var body = await System.Text.Json.JsonDocument.ParseAsync(request.Body);
        PresentationConfiguration.RejectDuplicates(body.RootElement);
        var revision = body.RootElement.GetProperty("revision").GetString();
        if (string.IsNullOrWhiteSpace(revision)) throw new InvalidDataException("A settings revision is required.");
        var configuration = body.RootElement.GetProperty("configuration").GetRawText();
        return Results.Ok(store.Save(revision, configuration, state));
    }
    catch (SettingsConflictException ex) { return Results.Json(new { error = ex.Message }, statusCode: 409); }
    catch (Exception ex) when (ex is System.Text.Json.JsonException or InvalidDataException or KeyNotFoundException or InvalidOperationException or FormatException)
    { return Results.BadRequest(new { error = ex.Message }); }
    catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
    {
        logger.LogError(ex, "Could not persist presentation settings in {Directory}", settingsDirectory);
        return Results.Json(new { error = "Could not save settings to disk. Previous settings remain active. Check service permissions and disk space." }, statusCode: 500);
    }
}).RequireCors("SettingsClient");

app.Urls.Add("http://0.0.0.0:5000");

app.Run();
