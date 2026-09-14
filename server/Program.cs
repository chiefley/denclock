var builder = WebApplication.CreateBuilder(new WebApplicationOptions
{
    Args = args,
    // Windows Services may start with System32 as their working directory.
    ContentRootPath = AppContext.BaseDirectory
});

builder.Services.AddWindowsService(options => options.ServiceName = "DenClock.Server");

var app = builder.Build();

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
});

app.Run();
