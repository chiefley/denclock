# DenClock Architecture and Decisions

## Initial Milestone

A reliable, always-on Internet-synchronized time display using a Windows server
and browser client. Weather, LoRa communication, and environmental sensing are
future extensions, not implemented system capabilities.

## Responsibilities and Rationale

- The Beelink NUC runs Windows. Windows maintains network time; the .NET 10
  ASP.NET Core server reports the system clock instead of independently querying NTP.
- The server is intended to run continuously as a native Windows Service.
  It supplies UTC and civil-time offset through its [implemented API](../server/docs/contract.md).
- The planned browser client runs on the NUC touchscreen and advances time
  between synchronization requests. [Client decisions](../client/docs/decisions.md)
  explain monotonic advancement and separation of synchronization/presentation.
- A future LoRa/Wi-Fi gateway links remote sensors to the server. Gateway and
  sensor folders currently contain no firmware implementation. Shared protocols
  should remain independent of server implementation; no wire format is specified yet.

Build incrementally and avoid speculative infrastructure. Production applications
run natively on Windows; WSL tooling is for development and deployment only.

The server returns UTC plus the host offset so clients need not interpret Windows
timezone IDs. It uses the executable directory as content root because Windows
Services may start from System32.

## Current Boundaries and Open Choices

The server is implemented; the client is specified but not implemented. Existing
source explicitly adds an all-IPv4 port-5000 listener, differing from localhost
configuration. Browser hosting/origin and deployment choices remain unresolved
in the [server README](../server/README.md) and [client README](../client/README.md). Network reachability does not itself establish
browser cross-origin access.

See [hardware](HARDWARE.md) for the recorded device/address inventory and
[server instructions](../server/README.md) for publishing and service management.
