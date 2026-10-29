# DenClock Architecture and Decisions

## Initial Milestone

A reliable, always-on Internet-synchronized time display using a Windows server
and browser client. LoRa environmental sensing has an accepted sensor and
diagnostic gateway baseline. The new independent gateway-to-LoRa-Service TCP path is
tested and accepted by the developer; DenClock primary-panel integration is implemented, verified, and accepted by the developer.

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
  production LoRa/Wi-Fi gateway transports raw packets and RSSI/SNR directly to
  the separate [LoRa Service](../lora-service/README.md) using the
  [Gateway Protocol](Gateway%20Protocol.md) over persistent TCP. The LoRa Service
  timestamps and interprets the custom wire protocol. MQTT publishing and Mosquitto deployment on the NUC are verified by the developer;
  DenClock primary-panel integration is implemented, verified, and accepted by the developer; DenClock consumes the semantic MQTT output without participating in LoRa decoding.
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

## Agreed LoRa Service Boundary

The receive path through the LoRa Service is implemented in
[story 004](stories/completed/004-end-to-end-lora-receive-path.md), accepted after developer testing and verification.
Mapping and MQTT publishing are implemented and accepted by the developer; Mosquitto deployment and live MQTT recovery checks are verified by the developer.
DenClock now subscribes to MQTT and serves latest primary observations to browsers; story 008 awaits deployment and acceptance. The accepted serial diagnostic
receiver remains selectable as a separate firmware build.

```text
Sensor / ham-radio nodes
          |
         LoRa
          |
Gateway firmware (radio transport)
          |
          | Gateway Protocol / persistent TCP over Wi-Fi
          | raw LoRa packet + RSSI + SNR
          v
LoRa Service (UTC timestamp, custom protocol parsing and normalization)
          |
          | normalized/application-friendly messages
          v
MQTT broker (application distribution)
          |
          +--> DenClock service --> Browser clients
          |
          +--> Future ham-radio applications
```

MQTT belongs after the LoRa Service. The gateway never communicates with MQTT.

- **Gateway firmware owns LoRa/radio transport.** It configures and operates the
  radio, receives packets, and forwards original packet bytes plus RSSI/SNR over
  TCP. HELLO identifies the gateway once per connection. The gateway does not
  timestamp packets. Forwarding preserves raw bytes independently of custom
  message classes, Node IDs, sequence semantics, or sensor TLVs. Radio reception
  and CRC checks belong here; application acceptance belongs in the LoRa Service.
  The production OLED separately observes sensor-decoder results to show node,
  battery and radio diagnostics and packet errors. This narrow local diagnostic
  exception preserves the transport boundary: OLED rejection does not suppress
  raw forwarding, and environmental measurements are not displayed.
- **MQTT is the specified application distribution boundary.** The
  [MQTT interface](MQTT-INTERFACE.md) defines the SENSOR_REPORT topic structure,
  payload, QoS, retention, and basic reconnect/delivery behavior. Sensor reports
  use `lora/sensors/{gatewayId}/{nodeId}`, QoS 1, and retained messages. Retained state is
  the last-known observation, not measurement history. Publishing is implemented; live integration with Mosquitto on the NUC is verified by the developer.
- **LoRa Service owns the custom LoRa protocol.** It accepts gateway TCP
  connections, assigns UTC reception timestamps, validates and parses
  message-class headers and TLVs, and interprets Node IDs and sequence numbers.
  Fixed protocol normalization resolves TLV types to measurement keys, canonical
  units, and values. Integer gatewayId and nodeId identify sensors as a pair;
  no device naming configuration is loaded. Routine
  observations go to MQTT and operational logs describe infrastructure events. It preserves relevant source and
  reception context so applications need not decode packet bytes. Unknown TLVs
  and malformed packets follow the wire specification. This shared protocol
  knowledge is independent of DenClock and future ham-radio applications.
- **DenClock owns environmental history and presentation.** It consumes normalized
  LoRa Service outputs, maintains application state/history, and supplies clock
  and weather information to browsers. Device names and locations belong to DenClock. Its presentation settings and existing gear dialog own sensor mappings keyed by numeric gateway/node pairs, with independent logical and display names and optional gateway names. Display-unit preferences/conversion, history retention, alarms, and display behavior belong to applications. DenClock does not parse the LoRa wire protocol.
- **Future ham-radio applications consume LoRa Service outputs.** They own their
  equipment-specific behavior and application transactions. Shared wire-protocol
  understanding stays in the LoRa Service. Future outbound commands would be
  encoded by that service and transported through the Gateway Protocol for radio
  transmission; command formats, authentication, correlation, retries, and
  delivery guarantees remain undefined.
- **Browser clients present DenClock information.** The den touchscreen is one
  client; other browsers can use the same service. Environmental history belongs
  to the service rather than an individual display. Remote access belongs at the
  web-service/network boundary.

The MQTT interface defines SENSOR_REPORT publication policy. Applications own
their measurement history and application-specific policy; gateway firmware
does not decide persistence policy.

## Deployment Independence

Mosquitto, LoRa Service, and DenClock may initially coexist on the Windows NUC.
They remain separate, independently deployable and movable services, connected
through network endpoints rather than shared process memory, local files,
or a shared application database. Gateway-to-service transport is TCP;
service-to-application distribution is MQTT. Addresses and ports are configurable
rather than requiring localhost. The independent .NET 10 Generic Host worker
supports console and Windows Service lifetimes; NUC service execution is verified
and accepted by the developer. Windows hosting integration stays in the host bootstrap, outside
protocol logic. Framework-dependent publishing uses the installed .NET 10 runtime.

Moving Mosquitto, LoRa Service, or both to another host must leave DenClock able
to remain on the NUC, retaining its environmental history and browser service.
Such a move changes connection/deployment configuration, not DenClock's protocol
implementation or application responsibilities. Co-location is a deployment
convenience, not an ownership boundary. This does not require containers or alter
the native Windows production requirement.

## Node Behavior and Protocol Authority

The [LoRa protocol](LoRa%20Protocol.md) is the authoritative wire specification.
Its `SENSOR_REPORT` header, assigned Message Class and initial TLV values, Node ID
rules, sequence-zero restart semantics, byte order, and radio configuration
(including sync word `0xD3C7`) are defined; they are no longer architecture TBDs.

Sleeping sensors are intended to wake and report spontaneously without a
destination field or expected response. The sensor now uses five-second timer deep sleep between reports, verified
and accepted by the developer. RTC memory preserves sequence continuity without flash
writes; Vext removes SHT40 power and the radio enters cold sleep before the
ESP32 sleeps. See the [sensor README](../firmware/sensors/README.md).

Powered ham-radio nodes may support command/response interactions and events.
Message Class selects the header structure, so future commands need not share
the sensor-report header or carry irrelevant fields in a universal envelope.

A small fixed sensor-report header is followed by TLVs. Defined TLV meanings and
representations are immutable; new meanings require new types. Parsers skip
unknown types by length while validating packet boundaries. There is no protocol
version field, and extensibility does not permit reinterpretation of existing
headers or types. These rules guide the LoRa Service without putting a wire
parser in DenClock.

## Deliberately Unresolved

- Command/response and event wire formats, destination addressing, authentication,
  transaction correlation, timeouts, retries, and delivery guarantees.
- Application storage and history retention. SENSOR_REPORT publication and
  retention are already defined by the MQTT interface.
- Security and remote-access configuration; the radio sync word provides network
  discrimination, not authentication.
- Registry policy for future Message Classes and TLV Types, and any additional
  radio tuning or packet limits not specified by the protocol.

Implement these incrementally as stories require them. The service boundary is
agreed; it does not itself implement any of the deferred infrastructure.

## Document Reconciliation

This document is the primary architecture reference. It incorporates the still
valid gateway, MQTT, browser, node-behavior, and TLV principles previously recorded
in [ARCHITECTURE2.md](ARCHITECTURE2.md), while preserving the implemented clock,
Windows hosting, and diagnostic baseline decisions above. The separate LoRa
Service supersedes the earlier assignment of wire-payload interpretation directly
to application services. The Gateway Protocol and LoRa Service specifications
supersede the earlier gateway-to-MQTT transport and gateway timestamp suggestions.
Using direct framed TCP keeps gateway transport independent of both the LoRa application
protocol and the separately defined MQTT schema. The service assigns reception time so the
gateway needs no synchronized clock.

Decision context:
[Gateway architecture design](chatgpt-conversation://6aae93e1-8e04-83e9-bb06-8563d690bd69).
Repository hardware references are in [HARDWARE.md](HARDWARE.md) and the
[sensor README](../firmware/sensors/README.md).

## Primary Presentation Boundary

DenClock owns ProgramData presentation.json, initializes it from an executable-local
file (or defaults) on first run, and subscribes to
numeric-identity MQTT reports. It resolves names from its own presentation settings,
keeps latest observations in memory and supplies two
primary slots through /api/presentation. Browsers poll that API, convert canonical
Celsius to the configured display unit, and render freshness using server-derived
UTC plus monotonic elapsed time. History remains deferred. The MQTT broker stays
NUC-local; remote browsers use DenClock's HTTP boundary.

## LoRa Destination Hostname Ownership

Prefer the existing Windows computer hostname plus `.local`, validated by an
ordinary multicast A query on the gateway LAN. The current NUC answered
`denclock.local` at `192.168.50.90`; this avoids an additional application
responder, address cache, or DNS-SD selection mechanism. Hostname availability
belongs to deployment independently of LoRa Service lifetime. Installed responder
ownership and detailed recovery/conflict observations were not individually
reported during the accepted story; see
[Windows hostname deployment](../lora-service/docs/mdns-deployment.md). Each
installation uses a unique computer name and an explicitly configured gateway
destination. Ordinary DNS/IP remains the alternative where link-local multicast
is unavailable. No radio, MQTT namespace or protocol change is required.

The gateway uses the ESP32 mDNS client for explicit `.local` A queries without
setting its own mDNS hostname or registering services. Ordinary DNS and IPv4
retain their separate lookup paths. This avoids the legacy one-shot query mode
that failed on the observed Windows path. The developer confirmed the corrected
gateway works with `denclock.local`. Responder ownership, recovery and conflict
checks remain listed separately in the deployment procedure above.

## Runtime Display Configuration

The server owns presentation settings independently of deployment assets and
LoRa Service configuration. Windows CommonApplicationData provides production
storage; an explicit directory override isolates development and automated tests.
A bundled presentation.json is a migration source only, preserving installed
settings across deployment.

The HTTP settings API uses opaque revisions and serialized saves to prevent
silent lost updates across LAN clients. It flushes a same-directory temporary
file and atomically replaces the destination before publishing one active
configuration. Presentation snapshots and MQTT reception share a short state
lock; disk writes do not stop reception. Identity-preserving edits retain
observations and timestamps. File watching is deliberately omitted.

The browser edits a temporary draft and preserves fields outside its primary
panel editor. Explicit reload resolves conflicts. JSON-only writes check browser
origins and use narrowly scoped development CORS; authentication remains out of
scope. Implementation is accepted in
[story 009](stories/completed/009-load-save-and-apply-display-settings.md).
