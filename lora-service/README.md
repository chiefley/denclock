# LoRa Service

An independent .NET 10 Generic Host worker that accepts gateway TCP connections,
timestamps received packets in UTC, maps SENSOR_REPORT to semantic values, and
publishes retained QoS 1 JSON to a configured MQTT broker. It does not require
the DenClock time server or a database. Broker outages leave gateway reception
running; reports that cannot be published are discarded.
[Story 004](../docs/stories/completed/004-end-to-end-lora-receive-path.md) is accepted after developer testing and verification
of the receive path. [Semantic mapping and MQTT publishing](docs/stories/completed/002-implement-semantic-mapping-and-mqtt-publishing.md)
is implemented and accepted by the developer. The developer has verified live MQTT
publication on the NUC, retained delivery and persistence, and restart/reboot recovery.
See [deployment verification](../docs/stories/completed/007-deploy-mosquitto-and-verify-live-sensor-publishing.md)
for the accepted integration results.

See the [implemented contract](docs/contract.md),
[Gateway Protocol](../docs/Gateway%20Protocol.md),
[LoRa protocol](../docs/LoRa%20Protocol.md), and
[service responsibilities](../docs/LoRa%20Service.md).

## Run interactively

Install the .NET 10 SDK, then run from the repository root on Windows or the
development host:

```powershell
dotnet run --project lora-service/LoRa.Service.csproj
```

The default listener is `0.0.0.0:5210` (all IPv4 interfaces). Port 5210 is an
implementation configuration default, not a protocol assignment. Use a LAN
address reachable by the gateway; `localhost` on the gateway is not this host.
Allow incoming TCP on the configured port through the host firewall for the
local network. For the hardware test, running on Windows avoids WSL NAT/port
forwarding requirements. No DenClock service needs to be started or stopped.

Override settings using ordinary .NET command-line or environment configuration:

```powershell
dotnet run --project lora-service/LoRa.Service.csproj -- --Gateway:ListenAddress=0.0.0.0 --Gateway:Port=5210
```

Environment keys are `Gateway__ListenAddress` and `Gateway__Port`. The address
must be a numeric IPv4 or IPv6 address; port must be 1–65535. Invalid settings
fail startup with a configuration error. `appsettings.json` is copied alongside
the executable and read from that directory, independent of the working folder.
For `dotnet run`, edit the source JSON and rebuild; for a published copy, edit
its JSON or pass overrides. Ctrl+C stops the listener and active connections.

## Mapping, MQTT, and logging configuration

The published `semantic-mapping.json` is loaded and validated before network
workers start. The supplied example maps Gateway 1 to `den-gateway`, Node 1 to
`porch`, and TLVs 1–3 to temperature (C), relativeHumidity (percent), and
batteryVoltage (V). Edit identities for your installation. See the
[semantic-mapping contract](../docs/Lora%20Service%20Semantic%20Mapping.md).

`appsettings.json` provides these settings, also overridable by command line or
.NET environment keys such as `Mqtt__Host`:

| Setting | Default | Purpose |
| --- | --- | --- |
| `SemanticMapping:Path` | `semantic-mapping.json` | Mapping file; relative to executable directory |
| `Mqtt:Host` | `localhost` | Broker hostname/address |
| `Mqtt:Port` | `1883` | Broker TCP port |
| `Mqtt:ClientId` | `lora-service` | Client identity; distinct for each concurrent service instance |
| `Logging:FilePath` | `logs/lora-service-.log` | Daily log filename prefix; relative to executable directory |

For example, with an already available broker:

```powershell
dotnet run --project lora-service/LoRa.Service.csproj -- --Mqtt:Host=192.168.1.100
```

All three mapping domains must be JSON objects. IDs and names must be unambiguous;
node names must be one literal MQTT topic level and cannot take another node's
`node-{id}` fallback. Measurements require name, valueType, length, scale, and
units; precision defaults to four significant figures. Only int16/uint16 and
length 2 are supported. Scale must be nonzero and fit decimal arithmetic for the
wire range; precision must be positive. Assigned TLVs must retain their specified
wire representation, scale, and canonical units. Invalid configuration prevents
startup with a diagnostic. There is no hot reload; restart after changes.

Unmapped gateways use the decimal ID string; unmapped nodes use `node-{id}`.
Unknown measurement types are skipped with warnings. No stale/default measurement
is substituted. The generic normalization path applies configured types/scales
without rounding values to the presentation precision.

Each valid report attempts one publication on `lora/sensors/{node-name}`, QoS 1,
retained, using the [MQTT JSON contract](../docs/MQTT-INTERFACE.md). The publisher
uses MQTTnet 5.2.0.1603 with MQTT 3.1.1, clean sessions, a five-second operation
timeout, and a five-second reconnect polling interval. It publishes directly;
there is no offline queue, spool, or replay. Each gateway session awaits its
current publish attempt, bounded by timeout; other sessions remain independent.
QoS 1 permits duplicate delivery; retained state is not measurement history.

Application code uses `ILogger<T>` through Serilog. Interactive execution logs to
console; all execution modes also write daily rolling files with seven-day age
retention, cleaned when files roll. Ensure the service account can write the
configured log directory; choose an external directory if deployment replaces
files under the publish directory. Logs contain infrastructure events and errors,
not routine successful sensor values. Default Windows Event Log output has been
replaced by the required Serilog sinks. Startup configuration failures before
Serilog host setup use bootstrap console diagnostics.

Dependencies: MQTTnet supplies the MQTT client and QoS protocol handling without
an offline managed-client queue. Serilog.Extensions.Hosting bridges ILogger;
Serilog.Sinks.Console and Serilog.Sinks.File implement the required sinks.

## Verification scope

The automated suite uses a minimal loopback MQTT peer to inspect actual client
traffic, acknowledge QoS 1, and exercise disconnect/reconnect. It installs no broker.
The test peer does not verify Mosquitto retained-message storage or delivery to a
new subscriber. The developer has separately verified retained delivery/persistence and live sensor
publication with Mosquitto on the Windows NUC; these remain manual integration checks.

## Verified NUC MQTT deployment

The developer verified Mosquitto 2.1.2 on the Windows NUC on 2026-09-22.
The Windows service is named `mosquitto`, runs as LocalSystem, and starts
automatically. Its executable is `C:/Program Files/Mosquitto/mosquitto.exe`
with service argument `run`. The verified machine `MOSQUITTO_DIR` is
`C:/Program Files/Mosquitto`; the service loads `mosquitto.conf` there.

The broker listens only on `127.0.0.1:1883`. The LoRa Service and diagnostic
subscriber run on the NUC; no LAN MQTT listener is enabled. Configuration:

```conf
listener 1883 127.0.0.1
allow_anonymous true
persistence true
persistence_location C:/ProgramData/Mosquitto/
persistence_file mosquitto.db
autosave_interval 60
autosave_on_changes false
log_dest file C:/ProgramData/Mosquitto/mosquitto.log
log_type error
log_type warning
log_type notice
log_type information
connection_messages true
log_timestamp true
```

The persistence/log directory must exist and be writable by the service account.
Broker data is separate from the LoRa deployment directory. Full-output LoRa
deployment remains in effect under the deployment-preservation decision below.

On the NUC, observe reports with PowerShell:

```powershell
& 'C:\Program Files\Mosquitto\mosquitto_sub.exe' -h 127.0.0.1 -p 1883 -t 'lora/sensors/+' -q 1 -v
```

To repeat the retained-state check, stop sensor transmissions after a report,
then connect a new subscriber. The same sequence and receivedUtc should arrive
immediately. In elevated PowerShell, run `Restart-Service mosquitto`, then
reconnect the subscriber with the sensor still off; the same retained report
should survive. Resume the sensor afterward.

For outage recovery, use `Stop-Service mosquitto` and `Start-Service mosquitto`
in elevated PowerShell. Fresh reports resume automatically; outage reports are
not replayed. Restart the installed LoRa Windows service through Services and
verify gateway reconnection. After a NUC reboot, verify automatic service startup,
fresh subscriber reports, and the clock display. The developer verified all
these recovery checks, sensor warming response, and sequence zero after reset.

## Windows hostname deployment

Use the installation's unique Windows computer hostname with `.local` after
verifying a direct multicast A query. The current NUC answered
`denclock.local` as `192.168.50.90`; no LoRa application responder is added. See
[hostname setup, firewall, diagnostics and hardware acceptance](docs/mdns-deployment.md)
for the required ownership check and remaining deployment tests. The gateway uses
the dedicated ESP32 mDNS client for `.local` A queries without advertising its own
name or services; ordinary DNS and IPv4 destinations retain their existing paths.
The developer has confirmed `denclock.local` works with the corrected firmware.
[Story 004](docs/stories/completed/004-reliable-mdns-hostname-resolution.md) is
fully accepted by the developer.

## Gateway setup and later integration checks

1. Copy `firmware/sensors/lora-gateway/include/config.example.h` to
   `config.local.h` in the same directory. Fill in the Wi-Fi SSID/password and
   this service host's LAN IP or hostname and port. The local header is ignored
   by Git; do not put credentials in protocol docs. Rebuild after changing it.
2. Build and flash the production transport (replace `<port>` with the board's
   serial port, for example `COM5`):

   ```powershell
   pio run -d firmware/sensors/lora-gateway -e heltec_wifi_lora_32_V3 --target upload --upload-port <port>
   pio device monitor --port <port> --baud 115200
   ```

3. Start this service, power the gateway, and confirm `HELLO sent` on the gateway
   and `Gateway 1 connected` in the service. The gateway retries Wi-Fi/TCP and
   sends a new HELLO on every TCP connection.
4. Run the existing sensor unchanged. Compare its serial values and sequence
   with the MQTT subscriber's JSON. All three readings plus RSSI/SNR and UTC must appear.
5. Using a configured broker and diagnostic MQTT subscriber, reset the sensor; confirm sequence 0, then 1, 2, and so on.
6. Restart the gateway while leaving the service running. Confirm another HELLO
   and resumed reports. Then stop/restart the service while leaving the gateway
   running; confirm reconnect and recovery without reflashing.
7. Interrupt Wi-Fi and restore it; confirm reports resume. Reports received while
   disconnected are discarded, so sequence gaps are expected. There is no replay.
8. To check application independence with a test transmitter, send a CRC-valid
   packet using the same radio settings but an unknown Message Class. The gateway
   must log that it forwarded the bytes; the service must log a LoRa protocol
   error and continue receiving subsequent valid SENSOR_REPORT packets.

The previously accepted serial-only receiver remains available with
`pio run -d firmware/sensors/lora-gateway -e diagnostic --target upload --upload-port <port>`.
It requires no Wi-Fi credentials. The remote sensor and `firmware/bringup/` remain
unchanged. Hardware acceptance belongs to the developer; automated TCP tests do
not verify the radio or real sensor accuracy.

## Build, tests, and Windows publish

```powershell
dotnet build lora-service/LoRa.Service.csproj
dotnet run --project lora-service/tests/LoRa.Service.Tests.csproj
```

The test project is an executable harness using the service dependencies, with
no additional test framework or broker installation. It exits nonzero on failure. It covers literal protocol
fixtures, all split positions in a multi-frame stream, one-byte reads, coalesced
frames, length bounds, validation, signed values, scaling, optional/unknown TLVs,
UTC timestamps, and a real loopback listener with concurrent clients, reconnect,
and cancellation. It also validates mapping/JSON, direct MQTT wire publication,
reconnect/no replay, and real-host invalid-mapping startup. It requires permission
to open local TCP sockets and launch the local dotnet host.

Portable gateway encoder tests (host C++ compiler, choose your local output path):

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic firmware/sensors/tests/gateway_protocol_test.cpp -o gateway-protocol-test
./gateway-protocol-test
```

The C++ fixture matches the .NET fixture and checks exact frame bytes and opaque
forwarding. These tests do not mock hardware.

To create a framework-dependent Windows x64 deployment:

```powershell
dotnet publish lora-service/LoRa.Service.csproj -c Release -r win-x64 --self-contained false -o artifacts/lora-service
```

Copy the whole published directory to the Windows host. The NUC needs the .NET 10
x64 runtime installed; this publish does not bundle it. Run `LoRa.Service.exe`
interactively or use your existing scripts to install and start it as a service.

## Deployment preservation decision

The developer has deferred file exclusions and configuration/log preservation
changes while deployment is still evolving. Continue publishing and deploying
the complete publish output for now. Revisit exclusions and preservation rules
when the system is more stable.

The existing [deployment helper](Deploy.sh) mirrors the complete publish output
with `rsync --delete`. Published configuration replaces the deployed copies,
and destination-only files, including logs under the deployment directory, may
be removed. This behavior remains in place under the current decision.

## Windows Service hosting

[Support Windows Service Hosting](docs/stories/completed/001-support-windows-service-hosting.md)
is verified and accepted by the developer. `AddWindowsService()` activates the Windows
Service lifetime when the service manager launches the executable; direct execution
retains the console lifetime. Service stop requests cancel the worker and close
its listener/connections. Configuration remains relative to the executable directory,
including when Windows starts the service from System32. No appsettings change is
required to enable service hosting.

Continue using the existing publish/install/start scripts, with the registered
binary path pointing to the published `LoRa.Service.exe`. Stop the installed service
before replacing its files. Verify start, decoded gateway reports, stop, and restart
on the NUC; do not run a console copy on the same listener port simultaneously.

Windows Service operation uses the Serilog rolling files described above. Check
those files for startup failures and MQTT connectivity. Existing Windows Service
hosting is deployed on the NUC; the developer verified resumed sensor publication
after LoRa Service restart and NUC reboot.
