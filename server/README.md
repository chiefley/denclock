# DenClock server

A small ASP.NET Core application targeting .NET 10. Windows maintains the
system clock; the server reports it to the display client. It does not query
an NTP server or verify that Windows has recently synchronized.

## Run during development

Install the .NET 10 SDK, then run from the repository root:

```powershell
dotnet run --project server/DenClock.Server.csproj
```

Open `http://localhost:5000/api/time` or use PowerShell:

```powershell
Invoke-RestMethod http://localhost:5000/api/time
```

Press Ctrl+C to stop. The server also runs in a console under WSL for
development, where it reports the development host's clock and time zone.

## Time API

`GET /api/time` returns HTTP 200 with JSON, for example:

```json
{
  "utcTime": "2026-09-14T16:00:00+00:00",
  "unixTimeMilliseconds": 1789401600000,
  "timeZoneId": "Eastern Standard Time",
  "utcOffsetSeconds": -14400
}
```

Both timestamps describe the same instant sampled while handling the request.
The time zone ID is the host's native ID (normally a Windows ID on the NUC),
and the offset is in seconds east of UTC, including daylight saving time at
that instant. Restart the server after changing the host's configured time zone.
Responses use `Cache-Control: no-store` so clients obtain a fresh sample.

The client should periodically fetch time and advance it between requests using
a monotonic elapsed-time clock. This endpoint is a system-clock snapshot,
not a precision network time protocol. The offset describes the sampled instant;
clients should refresh it periodically, including across daylight saving changes.

The default listener is local to the NUC at `http://localhost:5000`.
Change `Urls` in the deployed `appsettings.json`, or pass `--urls` on the
command line to override it. No authentication or cross-origin browser access
is configured for this initial local API.

## Publish for the NUC

From the repository root with the .NET 10 SDK:

```powershell
dotnet publish server/DenClock.Server.csproj -c Release -r win-x64 --self-contained true -o artifacts/server
```

Copy the entire `artifacts/server` folder to `C:\DenClock\Server` on the NUC.
This includes the runtime, so the NUC does not need a separate .NET install.

In an elevated PowerShell on the NUC, register and start the service:

```powershell
New-Service -Name 'DenClock.Server' -DisplayName 'DenClock Server' -BinaryPathName '"C:\DenClock\Server\DenClock.Server.exe"' -StartupType Automatic -Description 'Provides system time to the DenClock display.'
Start-Service -Name 'DenClock.Server'
Get-Service -Name 'DenClock.Server'
Invoke-RestMethod http://localhost:5000/api/time
```

The service starts automatically with Windows. These registration commands use
the default LocalSystem service account. Windows Service hosting follows
[Microsoft's ASP.NET Core guidance](https://learn.microsoft.com/en-us/aspnet/core/host-and-deploy/windows-service?view=aspnetcore-10.0).

To update, stop the service, replace the deployed files with the newly published
folder, and start it again:

```powershell
Stop-Service -Name 'DenClock.Server'
# Copy the newly published files to C:\DenClock\Server here.
Start-Service -Name 'DenClock.Server'
```

Warnings and errors are sent to the Windows Application event log by the
Windows Service integration. For startup troubleshooting, stop the service
and run `C:\DenClock\Server\DenClock.Server.exe` in a console.

## Verify on Windows

After starting the service, confirm that the endpoint returns fresh timestamps,
the configured Windows time zone, and `Cache-Control: no-store`:

```powershell
Invoke-WebRequest http://localhost:5000/api/time
Start-Sleep -Seconds 2
Invoke-WebRequest http://localhost:5000/api/time
```

Reboot the NUC and confirm that the service and endpoint are available again.
Check Windows Settings > Time & language > Date & time for automatic time
synchronization and the correct time zone.
