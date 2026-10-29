# DenClock server

A small ASP.NET Core application targeting .NET 10. Windows maintains the
system clock; the server reports it to the display client. It does not query
an NTP server or verify that Windows has recently synchronized.

## Run during development

Install the .NET 10 SDK, then run from the repository root:

```powershell
dotnet run --project server/DenClock.Server.csproj -- --Presentation:ConfigurationDirectory=../artifacts/dev-settings
```

Open `http://localhost:5000/api/time` or use PowerShell:

```powershell
Invoke-RestMethod http://localhost:5000/api/time
```

Press Ctrl+C to stop. The server also runs in a console under WSL for
development, where it reports the development host's clock and time zone.

## Implemented API and Operational Context

See the [implemented API contract](docs/contract.md).
The source adds an all-IPv4 listener on port 5000 while configuration names
localhost; confirm the effective listener on the NUC. Limited development
browser access is configured as described below.

Repository history reports local testing, NUC deployment, and subnet availability;
the documentation audits have not revalidated the running NUC. Confirm effective
listeners, intended deployment method, and the installed NUC runtime before the
next deployment. Manual publishing is self-contained; the WSL helper below is
framework-dependent and does not stop/start the service.

The server serves the [browser client](../client/README.md) at `/`, with its
runtime assets copied recursively into `wwwroot` during build and publish.
Development runs serve the client source directly; see the
[client workflow](../client/README.md). Open
`http://localhost:5000/` on the NUC or `http://DenClock:5000/` over the LAN.
The production client uses the same origin for `/api/time`, so CORS is not required.
No authentication is configured. Restart after
changing the host timezone and verify Windows synchronization independently of
the API. See [architecture and rationale](../docs/ARCHITECTURE.md).

## Development Browser Access (CORS)

`Cors:DevelopmentOrigins` in `appsettings.json` lists exact browser origins
(scheme, hostname, port; no trailing slash) allowed to read `GET /api/time` and `GET /api/presentation`.
The default is `http://localhost:5000`, the XPS browser's development URL.
This policy applies on the NUC in Production as well as during development,
but only to the time, presentation, and presentation-settings endpoints.
`/api/settings/presentation` additionally permits GET/PUT and the Content-Type
header, including preflight. It permits no credentials or arbitrary origins.
Writes require JSON and independently reject Origin headers other than the
served origin or these explicitly configured origins. Requests without Origin
are allowed for non-browser tools; this is not authentication.
Unapproved origins receive no CORS access header; CORS is a browser read
restriction, not API authentication.

To change the XPS browser URL, update this list on the NUC and restart its
service. `http://127.0.0.1:5000` is a different origin from `http://localhost:5000`.
Use an empty list to disable cross-origin grants. Standard .NET configuration
overrides also work, e.g. `Cors__DevelopmentOrigins__0` for the first origin.
Client API selection and edit/save/refresh instructions live in the
[client README](../client/README.md).

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

## Primary Sensor Display

Story 008 implements the server/browser path and is verified and accepted by
the developer. Configuration is described in
[the presentation specification](docs/presentation-configuration.md).

Display settings are service-owned in `%ProgramData%\DenClock\Server\presentation.json`.
Use the gear in a server-hosted browser on the NUC or LAN to change units,
primary assignments, labels, stale limits, numeric sensor mappings, and optional
gateway names without restarting. The browser
loads actual settings on each open; Cancel/Escape discards the draft. Save
persists before applying. Concurrent edits produce a conflict and an explicit
reload action; failures keep the draft open. Secondary entries and fields not
edited by this view are preserved; displaced primary sensors become secondary.

For the numeric MQTT upgrade, stop the service and manually update authoritative
ProgramData settings: rename sensor `internalName` to `name` and add actual integer
`gatewayId` and `nodeId` to every entry. Preserve other settings. The old format
cannot infer numeric identity and fails validation; see the
[configuration adjustment](docs/presentation-configuration.md#one-time-configuration-adjustment).

On first run only, the server validates and copies executable-local
`presentation.json` unchanged into ProgramData. If both files are absent, it
creates Fahrenheit with no assigned sensors. An existing invalid file fails
startup with its path and diagnostic; it is never silently replaced. The bundled
file is only an initialization source. Subsequent publishing/copying to
`C:\DenClock\Server` leaves ProgramData settings intact. Do not mirror/delete
or copy bundled examples into the ProgramData directory during deployment.

The service account needs read, create, write, replace, and delete permissions
in `%ProgramData%\DenClock\Server` (including creating that directory on first
run). The documented LocalSystem account normally has these rights; for a custom
service account, provision the directory and grant it Modify permission. A write
or initialization failure is reported; no successful save is claimed.

For development/tests, explicitly set `Presentation:ConfigurationDirectory`
(CLI as above or `Presentation__ConfigurationDirectory` environment variable)
to an isolated directory. Relative override paths resolve from the process's
working directory. Development and non-Windows runs require this override so
they cannot accidentally touch production settings. Automated tests use unique
temporary directories. Neither appsettings.json nor LoRa Service settings move.

Mqtt settings in appsettings.json default to localhost:1883 and client ID
denclock-server, distinct from lora-service. Environment overrides use Mqtt__Host,
Mqtt__Port, and Mqtt__ClientId. The production broker is NUC-only; an XPS-local
server cannot reach it. The established development profile therefore continues
to serve local browser assets while fetching both APIs from the NUC. Deploy the
updated NUC server first for real-sensor browser development. Local tests use a
loopback peer and need no production broker. MQTT outages leave the clock and
HTTP service operating.

GET /api/presentation supplies both panels in one no-store response; its JSON
and CORS behavior are documented in [the server contract](docs/contract.md).
No browser MQTT connection or extra broker listener is required.

Verification from the repository root (port 5000 must be free):

~~~sh
dotnet build server/DenClock.Server.csproj
dotnet run --project server/tests/DenClock.Server.Tests.csproj
node --test client/tests/*.test.js
node server/tests/hosting.mjs
node server/tests/mqtt.mjs
~~~

The executable .NET harness checks configuration and state validation. The
hosting test checks publishing, scoped CORS/preflight, settings validation,
conflicts, disk failures, runtime application, restart/deployment preservation,
and invalid authoritative configuration failure.
The MQTT test uses an ephemeral loopback peer to exercise the real subscriber,
retained state, broker restart and server restart. It never contacts the NUC.

Continue full-output deployment. Confirm the ProgramData settings survive
copying and restarting, then verify real sensor readings, stop reports long enough to
observe red stale values, restore reports, and assess shelf readability.
