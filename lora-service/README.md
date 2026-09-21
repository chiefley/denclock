# LoRa Service

An independent .NET 10 Generic Host worker that accepts gateway TCP connections,
timestamps received packets in UTC, decodes SENSOR_REPORT, and logs measurements.
It does not call or require the DenClock time server, MQTT, or a database.
[Story 004](../docs/stories/completed/004-end-to-end-lora-receive-path.md) is accepted after developer testing and verification
of the end-to-end receive path.

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

Diagnostic output includes Gateway ID, UTC reception time, node, sequence,
temperature in C, relative humidity in %, battery in V, RSSI in dBm, and SNR in dB.
Missing legal TLVs display `N/A`. For example:

```text
Gateway 1 connected
Gateway 1 UTC 2026-09-20T14:38:27.1230000+00:00 Node 12 Sequence 3871 Temperature 21.43 C Relative Humidity 43.21 % Battery 3.912 V RSSI -72 dBm SNR 9.50 dB
```

## Gateway setup and hardware acceptance

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
   with the service's output. All three readings plus RSSI/SNR and UTC must appear.
5. Reset the sensor; confirm sequence 0, then 1, 2, and so on.
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

The test project is a dependency-free executable harness (beyond the service's
Hosting dependency). It exits nonzero on failure. It covers literal protocol
fixtures, all split positions in a multi-frame stream, one-byte reads, coalesced
frames, length bounds, validation, signed values, scaling, optional/unknown TLVs,
UTC timestamps, and a real loopback listener with concurrent clients, reconnect,
and cancellation. It requires permission to open local TCP sockets.

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

Windows Service hosting adds Windows Application Event Log output. Its default
threshold is Warning; to see Information-level connection and sensor reports, set
`Logging:EventLog:LogLevel:Default` to `Information` in the deployed appsettings.json.
Check Event Viewer → Windows Logs → Application for startup failures. An account
without permission to create an Event Log source may need administrator setup of
the source before logging works. Console logging remains available interactively.
See [Microsoft's Windows Service hosting documentation](https://learn.microsoft.com/en-us/dotnet/core/extensions/windows-service).
