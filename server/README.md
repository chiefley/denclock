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

## Status and API

See [current status](STATUS.md) and the [implemented API contract](docs/contract.md).
The source adds an all-IPv4 listener on port 5000 while configuration names
localhost; confirm the effective listener on the NUC. Browser cross-origin
access is not configured.

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

## Existing WSL Deployment Helper

`deploy.sh` publishes a framework-dependent Windows x64 build to
`/tmp/denclock-server-publish` and mirrors it to `/mnt/denclock/server` with
`rsync --delete`. It requires the appropriate .NET/ASP.NET Core runtime installed
on the NUC and a correctly mounted deployment destination. It clears the staging
directory and removes destination files absent from the published output.
It does not stop or restart the service.

This differs from the self-contained manual procedure above. Confirm the intended
route and service stop/start sequence before deploying. WSL is a development
helper environment, not a production runtime requirement.
