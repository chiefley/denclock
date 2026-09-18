var builder = WebApplication.CreateBuilder(new WebApplicationOptions
{
    Args = args,
    // Windows Services may start with System32 as their working directory.
    ContentRootPath = AppContext.BaseDirectory
});

builder.Services.AddWindowsService(options => options.ServiceName = "DenClock.Server");

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

app.Urls.Add("http://0.0.0.0:5000");

app.Run();
