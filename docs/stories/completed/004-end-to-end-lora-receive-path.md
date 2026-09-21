# Story: End-to-End LoRa Receive Path

## Objective

Implement the first end-to-end production receive path from the existing remote sensor, through the LoRa gateway, to the new LoRa Service.

At completion, a real `SENSOR_REPORT` transmitted by the existing remote sensor shall be received over LoRa by the gateway, transported unchanged over TCP using the Gateway Protocol, decoded by the LoRa Service, timestamped, and displayed in the LoRa Service diagnostic output.

This story deliberately stops at the LoRa Service.

MQTT and DenClock integration are not part of this story.

---

## Governing Documents

Implementation shall conform to the repository documentation, particularly:

- `ARCHITECTURE.md`
- ARCHITECTURE2.md
- `LORA-PROTOCOL.md`
- `GATEWAY-PROTOCOL.md`
- `LORA-SERVICE.md`

Where protocol details are concerned, the corresponding protocol document is authoritative.

Do not invent protocol extensions or additional fields to satisfy anticipated future requirements.

---

# 1. Existing State

The repository already contains:

- A working remote sensor implementation under `/remote-sensor`.
- A working LoRa gateway/test receiver under `/lora-gateway`.
- Known-good LoRa bringup programs retained under `/bringup`.
- A working `SENSOR_REPORT` implementation.
- Defined LoRa radio parameters.
- Defined Gateway Protocol.
- Defined LoRa Service responsibilities.

The existing remote sensor has already demonstrated successful transmission of:

- Temperature
- Relative humidity
- Battery voltage

The existing gateway receiver has already demonstrated successful LoRa reception and diagnostic decoding of these packets.

These working implementations should be preserved as useful baselines while the production receive path is added.

---

# 2. Target End-to-End Path

Implement this path:

```text
SHT40
  |
Remote Sensor
  |
SENSOR_REPORT
  |
LoRa
  |
Gateway
  |
raw LoRa packet + RSSI + SNR
  |
RX_PACKET
  |
persistent TCP
  |
LoRa Service
  |
Gateway Protocol parser
  |
LoRa protocol parser
  |
SENSOR_REPORT decoder
  |
UTC timestamp
  |
diagnostic output
```

---

# 3. Gateway Changes

Modify the production `/lora-gateway` implementation so that its normal receive path behaves as an application-agnostic LoRa transport.

The gateway shall no longer need to understand `SENSOR_REPORT` in its production transport path.

## 3.1 Wi-Fi

The gateway shall connect to the configured Wi-Fi network.

Wi-Fi credentials shall not be embedded in protocol definitions.

Follow the repository's existing configuration conventions if such conventions already exist.

Do not introduce a generalized configuration system solely for this story.

---

## 3.2 LoRa Service TCP Connection

The gateway shall establish a persistent TCP connection to a configured LoRa Service host/address and port.

The gateway initiates the connection.

When the connection is lost, the gateway shall attempt to reconnect.

A successful reconnection is treated as a new Gateway Protocol connection.

---

## 3.3 HELLO

Immediately after establishing the TCP connection, the gateway shall send a Gateway Protocol `HELLO` message.

For the initial gateway:

```text
Gateway ID = 0x01
```

The Gateway ID shall not be repeated in subsequent `RX_PACKET` messages.

---

## 3.4 RX\_PACKET

When a valid LoRa packet is received, the gateway shall construct and send an `RX_PACKET` according to `GATEWAY-PROTOCOL.md`.

The payload shall contain:

```text
RSSI
SNR
Raw LoRa packet bytes
```

The gateway shall convert RSSI and SNR only as required by the Gateway Protocol wire representation.

The raw LoRa packet bytes shall otherwise be forwarded unchanged.

The gateway shall not:

- parse the LoRa Message Class;
- parse the Source Node ID;
- parse the sequence number;
- parse TLVs;
- interpret temperature;
- interpret humidity;
- interpret battery voltage;
- add a timestamp;
- convert the packet into JSON;
- communicate with MQTT.

The gateway's responsibility is transport.

---

## 3.5 Disconnected Behavior

If a LoRa packet is received while the gateway is not connected to the LoRa Service, the first implementation may discard the packet.

Do not implement packet queuing or persistent buffering as part of this story.

Periodic sensor reports provide another opportunity for reception.

---

## 3.6 Existing Diagnostic Decoder

Existing gateway diagnostic decoding code may be retained if useful for bringup or testing.

However, it shall not be required by the production gateway transport path.

The production gateway must remain capable of forwarding a valid raw LoRa packet whose Message Class it does not understand.

---

# 4. LoRa Service Project

Create the initial LoRa Service implementation according to `LORA-SERVICE.md`.

The service shall target:

```text
.NET 10
```

Use the .NET Generic Host / Worker Service model.

The service shall initially support interactive console execution for development and testing.

Its design shall permit later hosting as a Windows Service without placing Windows-specific dependencies in application logic.

Installing it as a production Windows Service is not required by this story.

---

# 5. TCP Server

The LoRa Service shall listen for incoming Gateway Protocol TCP connections.

The listening address/port shall be configurable using ordinary .NET configuration mechanisms.

Do not introduce an elaborate configuration subsystem.

The implementation shall not assume that TCP message boundaries correspond to TCP read boundaries.

It must correctly handle:

- a Gateway Protocol message split across multiple TCP reads;
- multiple Gateway Protocol messages received in one TCP read.

Gateway Protocol framing shall be determined from the three-byte common header defined in `GATEWAY-PROTOCOL.md`.

---

# 6. HELLO Processing

The first valid application message on a newly established gateway connection shall be `HELLO`.

The service shall:

1. Parse the `HELLO`.
2. Validate its payload.
3. Reject Gateway ID `0x00`.
4. Associate the supplied Gateway ID with that TCP connection.
5. Log the successful gateway connection.

For example:

```text
Gateway 1 connected
```

Exact log wording is not part of the external contract.

---

# 7. RX\_PACKET Processing

When an `RX_PACKET` is received, the service shall:

1. Validate the Gateway Protocol message.
2. Determine the Gateway ID from the connection.
3. Decode RSSI.
4. Decode SNR.
5. Preserve the remaining payload bytes as the raw LoRa packet.
6. Assign the UTC reception timestamp.
7. Pass the raw LoRa packet to the LoRa application-protocol parser.

The timestamp shall be assigned by the LoRa Service, not the gateway.

---

# 8. LoRa Protocol Processing

The LoRa Service shall inspect the Message Class of the raw LoRa packet according to `LORA-PROTOCOL.md`.

The initial implementation shall support:

```text
0x01 SENSOR_REPORT
```

The design shall permit additional Message Class handlers to be added later without requiring unrelated protocol handlers to be rewritten.

Do not implement `COMMAND`, `RESPONSE`, or `EVENT` in this story.

---

# 9. SENSOR\_REPORT Decoder

Implement decoding of the existing `SENSOR_REPORT` format exactly as specified by `LORA-PROTOCOL.md`.

The decoder shall obtain:

- Source Node ID
- Sequence Number
- Temperature
- Relative humidity
- Battery voltage

The decoder shall obey the documented:

- byte order;
- signedness;
- field widths;
- TLV lengths;
- scaling;
- unknown-TLV handling;
- malformed-packet rules.

Protocol constants shall not be reinterpreted or assigned new meanings.

---

# 10. Diagnostic Output

For the first implementation, successful receipt of a sensor report shall be visible through normal .NET logging or console diagnostic output.

The output shall contain enough information to demonstrate the complete receive path, including:

- Gateway ID
- UTC reception timestamp
- Source Node ID
- Sequence Number
- Temperature
- Relative humidity
- Battery voltage
- RSSI
- SNR

For example:

```text
Gateway 1
2026-09-20T14:38:27.123Z
Node 1
Sequence 42
Temperature 21.43 C
Relative Humidity 43.21 %
Battery 3.912 V
RSSI -72 dBm
SNR +9.50 dB
```

Exact formatting is not part of the service contract.

---

# 11. Protocol Validation

Keep Gateway Protocol validation and LoRa application-protocol validation conceptually separate.

A malformed Gateway Protocol message is a Gateway Protocol error.

A correctly framed `RX_PACKET` containing an invalid LoRa packet is a LoRa protocol error.

Malformed or unsupported input shall be logged appropriately and shall not terminate the LoRa Service.

Do not add unspecified protocol recovery mechanisms or extensions merely to anticipate future requirements.

---

# 12. Tests

Add automated tests where practical for deterministic protocol behavior.

At minimum, tests should cover the important pure protocol logic, including:

- Gateway Protocol header parsing.
- Payload-length handling.
- Fragmented TCP input reconstruction.
- Multiple messages in one input buffer.
- Valid `HELLO`.
- Invalid Gateway ID zero.
- Valid `RX_PACKET`.
- Valid `SENSOR_REPORT`.
- Big-endian sequence decoding.
- Temperature scaling and signed values.
- Relative-humidity scaling.
- Battery-voltage scaling.
- Unknown TLV skipping.
- Malformed TLV length.
- Invalid Message Class zero.

Tests shall use the authoritative protocol documents rather than inventing alternate protocol behavior.

Do not mock hardware merely to obtain a coverage number.

---

# 13. Preserve Known-Good Bringup Code

Do not remove or substantially alter the known-good programs under `/bringup`.

They remain useful diagnostic tools for separating:

```text
radio/hardware problem
```

from:

```text
gateway/service/software problem
```

The existing successful remote sensor behavior should likewise not be unnecessarily redesigned as part of this story.

---

# 14. Explicitly Out of Scope

Do not implement the following as part of this story:

- MQTT
- Mosquitto integration
- DenClock integration
- Database storage
- Environmental history
- Web UI
- Sensor friendly-name mapping
- `TX_PACKET` behavior
- LoRa `COMMAND`
- LoRa `RESPONSE`
- LoRa `EVENT`
- Antenna switching
- SWR telemetry behavior
- Command authentication
- Gateway packet buffering
- Service discovery
- Automatic gateway provisioning
- General-purpose configuration management
- Cloud services

These are future increments.

---

# 15. Acceptance Criteria

The story is complete when the following can be demonstrated using the real existing hardware.

## End-to-End Acceptance Test

1. Start the LoRa Service interactively.
2. Start/power the LoRa gateway.
3. The gateway connects to Wi-Fi.
4. The gateway establishes a TCP connection to the LoRa Service.
5. The gateway sends `HELLO` with Gateway ID `0x01`.
6. The LoRa Service recognizes Gateway 1.
7. The existing remote sensor transmits a real `SENSOR_REPORT`.
8. The gateway receives the LoRa packet.
9. The gateway creates an `RX_PACKET` containing RSSI, SNR, and the unchanged raw LoRa packet.
10. The gateway sends the `RX_PACKET` over the persistent TCP connection.
11. The LoRa Service reconstructs and validates the Gateway Protocol message.
12. The LoRa Service assigns a UTC reception timestamp.
13. The LoRa Service recognizes the LoRa packet as `SENSOR_REPORT`.
14. The LoRa Service correctly decodes the sensor values.
15. Diagnostic output displays the Gateway ID, timestamp, Node ID, sequence number, temperature, humidity, battery voltage, RSSI, and SNR.
16. The displayed sensor values are consistent with the values transmitted by the existing remote sensor.

Additionally:

- Disconnecting/restarting the gateway shall not require restarting the LoRa Service.
- After reconnecting, the gateway shall send a new `HELLO` and subsequent sensor reports shall again be received.
- The production gateway transport path shall not depend on understanding `SENSOR_REPORT`.

---

# 16. Definition of Done

This story is done when:

- The end-to-end hardware acceptance test succeeds.
- Relevant automated protocol tests pass.
- Existing known-good bringup programs remain available.
- The implementation conforms to the governing protocol documents.
- No out-of-scope infrastructure has been added merely in anticipation of future stories.
- Repository documentation is updated where necessary to reflect the implemented state.

The resulting system should prove this single statement:

> A real LoRa sensor packet can travel unchanged through the application-agnostic gateway to the LoRa Service, where it is timestamped and correctly interpreted.

## Repository mapping and implementation choices

- Gateway: `firmware/sensors/lora-gateway/`; preserve its serial decoder as a
  selectable diagnostic build. Sensor and bringup programs remain unchanged.
- New .NET worker: `lora-service/`.
- Authoritative references: `docs/Gateway Protocol.md`, `docs/LoRa Protocol.md`,
  and `docs/LoRa Service.md`. Their TCP transport and service timestamp ownership
  supersede stale gateway-to-MQTT language in the architecture notes.
- Gateway configuration: ignored local header with a checked-in example. Service
  configuration: ordinary .NET JSON, environment variables, and command line.
- Malformed Gateway Protocol input closes only that connection; invalid LoRa
  input is logged and dropped while the initialized connection remains usable.

## Verification for human review

- .NET 10 worker build passes without warnings.
- 254 automated protocol/session/TCP checks pass, including fragmented and
  coalesced frames, reconnect, concurrent clients, and shutdown with a partial frame.
- Portable gateway encoder tests pass under address/undefined-behavior sanitizers
  (LeakSanitizer disabled because of the sandbox tracing limitation). Literal
  wire fixtures check identical raw bytes and signed radio metadata.
- Both production and preserved diagnostic PlatformIO environments build. The
  production build was also checked with example network settings; temporary
  example-only configuration was removed afterwards.
- The actual console host was exercised over loopback from a different working
  directory, with configuration overrides, sensor diagnostics, errors in both
  protocol layers, reconnect, and clean Ctrl+C shutdown.
- Self-contained Windows x64 publish succeeds. Windows execution and real RF /
  Wi-Fi end-to-end acceptance remain unverified by Codex.
- The DenClock server/client, remote sensor, shared radio setup, and known-good
  bringup programs are unchanged. No MQTT or DenClock integration is implemented.
## Human acceptance

The developer confirmed story 004 is tested and verified on 2026-09-21.
The end-to-end receive path is accepted as complete. The verification above
records the implementation checks; hardware acceptance is supplied by the
developer. MQTT and DenClock integration remain outside this story.
