# DenClock Architecture and Decisions

## Initial Milestone

A reliable, always-on Internet-synchronized time display using a Windows server
and browser client. LoRa environmental sensing now works as an independent
sensor-to-diagnostic-gateway path; weather display and server integration remain future work.

## Responsibilities and Rationale

- The Beelink NUC runs Windows. Windows maintains network time; the .NET 10
  ASP.NET Core server reports the system clock instead of independently querying NTP.
- The server is intended to run continuously as a native Windows Service.
  It supplies UTC and civil-time offset through its [implemented API](../server/docs/contract.md).
- The browser client targets the NUC touchscreen and advances time
  between synchronization requests. [Client decisions](../client/docs/decisions.md)
  explain monotonic advancement and separation of synchronization/presentation.
- The accepted remote sensor and diagnostic receiver live under `firmware/sensors/`.
  Their [binary LoRa protocol](LoRa%20Protocol.md) is independent of the server.
  Shared radio settings and a portable packet codec keep both firmware projects
  consistent and permit host-side packet tests.
- The receiver interprets measurements only as acceptance-test scaffolding. The
  eventual LoRa/Wi-Fi gateway will transport packets without interpreting sensor
  measurements. Server integration and MQTT remain deferred.
- Known-good diagnostics in `firmware/bringup/` remain independent and unchanged,
  preserving a hardware troubleshooting baseline.

Build incrementally and avoid speculative infrastructure. Production applications
run natively on Windows; WSL tooling is for development and deployment only.

The server returns UTC plus the host offset so clients need not interpret Windows
timezone IDs. It uses the executable directory as content root because Windows
Services may start from System32.

## Current Boundaries and Open Choices

The server and initial browser client are implemented; the client story is
accepted. Codex has not independently verified its NUC deployment. Existing
source explicitly adds an all-IPv4 port-5000 listener, differing from localhost
configuration. The server hosts the client assets and time API on the same
origin for production LAN and local viewing. Development serves `client/`
directly and can select the NUC API through a generated settings module.
The API permits configured exact development origins through CORS. Recursive
publishing keeps client runtime files with the server, without requiring a
separate static host. See the [server README](../server/README.md)
and [client README](../client/README.md) for operation and deployment limits.

See [hardware](HARDWARE.md) for the recorded device/address inventory and
[server instructions](../server/README.md) for publishing and service management.
