# LoRa Service

## Implementation status

The receive path, Windows Service hosting, fixed measurement normalization,
Serilog infrastructure logging, and MQTT publishing are implemented. Production
broker installation and deployment/acceptance of the MQTT path on the NUC remain
separate work.
See the [implemented contract](../lora-service/docs/contract.md).

## 1. Purpose

The LoRa Service is the application-level boundary between the DenClock LoRa radio network and applications that consume or produce LoRa messages.

Its primary responsibilities are:

- Accept connections from one or more LoRa/Wi-Fi gateways.
- Implement the Gateway Protocol defined in [Gateway Protocol](Gateway%20Protocol.md).
- Receive raw LoRa packets and associated radio metadata from gateways.
- Interpret LoRa application packets according to [LoRa Protocol](LoRa%20Protocol.md).
- Assign an authoritative UTC reception timestamp to received packets.
- Convert LoRa protocol messages into application-level representations.
- Publish normalized SENSOR_REPORT messages through MQTT.
- Eventually accept application requests that require transmission of LoRa messages.

The LoRa Service separates knowledge of the custom LoRa application protocol from the gateway firmware and from applications such as DenClock.

The basic architectural boundary is:

```text
LoRa Node
    |
    | LoRa
    v
Gateway
    |
    | Gateway Protocol / TCP
    v
+----------------------+
|     LoRa Service     |
|                      |
| Gateway transport    |
| LoRa interpretation  |
| UTC timestamping     |
| Message normalization|
+----------------------+
    |
    | MQTT
    v
Applications
```

The gateway knows how to move packets.

The LoRa Service knows what the packets mean.

Applications know what to do with them.

---

## 2. Implementation Platform

The initial LoRa Service shall be implemented as a .NET 10 application using the .NET Generic Host / Worker Service model.

The application shall support:

- Interactive console execution during development and testing.
- Unattended execution as a Windows Service on the DenClock NUC.

Application logic shall not depend on Windows-specific APIs unless a future requirement explicitly requires them.

Running as a Windows Service is a deployment choice, not an architectural dependency.

The service should therefore remain capable of being hosted on another suitable system in the future without redesigning its application-level interfaces.

---

## 3. Architectural Responsibilities

The LoRa Service owns the following responsibilities.

### 3.1 Gateway Connections

The LoRa Service acts as the TCP server for the Gateway Protocol.

Gateways initiate persistent TCP connections to the LoRa Service.

The service shall:

- Accept gateway TCP connections.
- Parse Gateway Protocol framing across arbitrary TCP read boundaries.
- Process the initial `HELLO` message.
- Associate the Gateway ID supplied by `HELLO` with that TCP connection.
- Receive subsequent Gateway Protocol messages from that connection.
- Detect connection loss and cleanly release connection state.

The Gateway Protocol itself is defined by [Gateway Protocol](Gateway%20Protocol.md).

---

## 4. Gateway Protocol Processing

The LoRa Service shall implement Gateway Protocol framing and validation.

The common Gateway Protocol header is:

```text
Byte 0:     Message Type
Bytes 1-2:  Payload Length, uint16, network byte order
```

Because TCP is a byte stream, the implementation must not assume that:

- one TCP read contains one complete Gateway Protocol message;
- a Gateway Protocol message arrives in a single TCP read; or
- a TCP read contains only one Gateway Protocol message.

The service must reconstruct messages according to the protocol framing.

### 4.1 Initial Message Types

The Gateway Protocol currently defines:

```text
0x00  INVALID / RESERVED
0x01  HELLO
0x02  RX_PACKET
0x03  TX_PACKET
```

The first implementation requires receive-side support for:

- `HELLO`
- `RX_PACKET`

`TX_PACKET` is defined by the Gateway Protocol but does not need to be implemented until bidirectional LoRa communication is required.

---

## 5. Gateway Identification

A gateway identifies itself by sending `HELLO` immediately after establishing its TCP connection.

The `HELLO` payload contains the Gateway ID.

Gateway ID `0x00` is invalid.

After a valid `HELLO`, the Gateway ID is associated with the TCP connection and does not need to be repeated in each subsequent Gateway Protocol message.

The initial DenClock gateway may use Gateway ID `0x01`.

Gateway ID assignment and administration are configuration concerns and are outside the Gateway Protocol itself.

---

## 6. Receiving LoRa Packets

A gateway sends a received LoRa packet to the service using an `RX_PACKET` Gateway Protocol message.

The `RX_PACKET` payload contains:

```text
RSSI
SNR
Raw LoRa packet bytes
```

RSSI and SNR representation are defined by [Gateway Protocol](Gateway%20Protocol.md).

The remaining bytes are the exact LoRa packet received by the gateway.

The LoRa Service shall not require the gateway to interpret, translate, normalize, or otherwise understand those bytes.

Upon receiving a valid `RX_PACKET`, the LoRa Service shall:

1. Record the associated Gateway ID.
2. Decode the radio metadata supplied by the gateway.
3. Assign a UTC reception timestamp.
4. Pass the raw LoRa packet to the LoRa application-protocol parser.
5. Process the packet according to its LoRa Message Class.

---

## 7. Reception Timestamp

The LoRa Service is the authoritative source of the reception timestamp for received LoRa packets.

The gateway does not timestamp packets.

The LoRa Service shall assign the UTC timestamp as soon as practical after receiving the complete `RX_PACKET`.

The timestamp represents:

> The time at which the LoRa Service received the `RX_PACKET` from the gateway.

It does **not** represent:

- the time at which a sensor measurement was taken;
- the exact time RF transmission began;
- the exact time the gateway radio received the packet; or
- a timestamp generated by the remote LoRa node.

For the intended applications, the small gateway-to-service TCP/Wi-Fi latency is acceptable.

This design avoids requiring synchronized real-time clocks or NTP timekeeping in the gateway or sleeping sensor nodes.

---

## 8. LoRa Application Protocol

The raw LoRa packet contained in `RX_PACKET` shall be interpreted according to [LoRa Protocol](LoRa%20Protocol.md).

The first byte of a LoRa application packet is the Message Class.

Message Class `0x00` is invalid/reserved.

The currently assigned Message Classes are:

```text
0x00  INVALID / RESERVED
0x01  SENSOR_REPORT
0x02  COMMAND
0x03  RESPONSE
0x04  EVENT
```

`COMMAND`, `RESPONSE`, and `EVENT` are reserved for future definition.

The initial LoRa Service implementation needs to interpret `SENSOR_REPORT`.

The service architecture shall permit additional Message Class handlers to be added without requiring unrelated handlers to be modified.

---

## 9. SENSOR_REPORT Processing

The `SENSOR_REPORT` wire format is defined by [LoRa Protocol](LoRa%20Protocol.md).

The initial service shall be capable of decoding:

- Source Node ID
- Sequence Number
- Temperature
- Relative Humidity
- Battery Voltage

The initial TLV types are:

```text
0x01  TEMPERATURE
0x02  RELATIVE_HUMIDITY
0x03  BATTERY_VOLTAGE
```

Units, scaling, lengths, signedness, byte order, and other wire-level semantics are defined exclusively by [LoRa Protocol](LoRa%20Protocol.md).

The LoRa Service shall not redefine those semantics.

Unknown TLV types shall be handled according to [LoRa Protocol](LoRa%20Protocol.md).

---

## 10. Message Normalization

The LoRa Service is responsible for translating LoRa wire-format messages into representations suitable for consumption by applications.

This includes converting protocol-specific representations such as:

- integer temperature scaling;
- integer humidity scaling;
- battery millivolts;
- radio SNR representation; and
- protocol identifiers

into useful application-level values.

The [MQTT interface](MQTT-INTERFACE.md) defines normalized measurements and
reception context: integer gatewayId and nodeId, sequence, UTC timestamp, RSSI,
and SNR. The pair of identifiers is sensor identity; node numbers need only be
unique within a gateway. No gateway/node names or semantic mapping file are used.
Fixed protocol decoding supplies measurement keys, units, scales, and precision.
Consumers need no wire-packet decoding; they own device names and locations.

---

## 11. MQTT Boundary

MQTT is the application-facing distribution mechanism for LoRa Service messages.

The LoRa gateway does **not** communicate with MQTT.

The intended flow is:

```text
LoRa
  |
Gateway
  |
Gateway Protocol
  |
LoRa Service
  |
MQTT
  |
Applications
```

The LoRa Service publishes normalized received sensor messages. Receiving and
translating application requests for LoRa transmission remains future work.

The [MQTT interface](MQTT-INTERFACE.md) defines the initial SENSOR_REPORT topic
hierarchy, payload schema, QoS, retained last-known state, and basic broker
failure/reconnection behavior. It does not require offline spooling or replay;
loss of a report while MQTT is unavailable is acceptable. Retained state does
not replace application measurement history.

Command, response, and event topics, authentication, and other bidirectional
behavior remain outside that interface. The Gateway Protocol already defines
TX_PACKET framing; this does not define application command semantics.

MQTT publishing is implemented; production broker integration remains separate work.

---

## 12. Application Boundaries

The LoRa Service is infrastructure shared by applications.

It shall not contain DenClock-specific behavior.

For example, the LoRa Service may report:

```text
Node 1
Temperature 21.43 C
Relative Humidity 43.21 %
Battery 3.912 V
Received 2026-09-20T14:38:27.123Z
```

Device names and physical locations belong to applications. LoRaService publishes
numeric identity and normalized measurements without resolving device names.

Likewise, the LoRa Service does not decide:

- how long temperature history is retained;
- how DenClock presents temperature or converts canonical units for display;
- what graphs DenClock displays;
- whether a temperature is considered high or low;
- how environmental history is stored.

Those responsibilities belong to consuming applications.

---

## 13. Persistence

The LoRa Service does not initially require persistent storage of received sensor measurements.

Applications consuming normalized messages own application history and persistent application state.

The service may maintain transient operational state required to perform its own responsibilities, including:

- active gateway connections;
- Gateway IDs associated with connections;
- protocol parsing state;
- outstanding transactions when bidirectional communication is implemented.

Persistent infrastructure state may be introduced later if an actual requirement emerges.

---

## 14. Error Handling and Validation

The LoRa Service shall validate both protocol layers independently.

### Gateway Protocol validation

Examples include:

- invalid Message Type;
- Message Type `0x00`;
- unsupported Message Type;
- invalid payload length;
- malformed fixed-length messages;
- messages received before required connection initialization.

### LoRa Protocol validation

Examples include:

- invalid Message Class;
- Message Class `0x00`;
- unsupported Message Class;
- packet shorter than the fixed header required by its class;
- malformed TLV;
- TLV length extending beyond the received packet.

A valid Gateway Protocol `RX_PACKET` containing an invalid LoRa application packet is a LoRa protocol error, not a Gateway Protocol framing error.

The distinction between the two protocol layers should remain visible in implementation structure and diagnostics.

Malformed input shall not cause termination of the service.

---

## 15. Logging and Diagnostics

The service shall provide useful operational logging.

The service uses the [semantic-mapping logging policy](Lora%20Service%20Semantic%20Mapping.md#9-logging)
and [MQTT logging policy](MQTT-INTERFACE.md#20-logging). Application ILogger calls
flow through Serilog to interactive console and daily rolling files retained for
seven days. Logs describe startup/shutdown, configuration validation, gateway and
MQTT connectivity, malformed packets, unknown TLV types, and operational errors.
Routine successful measurements are published through MQTT, not logged.

The original receive increment used measurement diagnostics before MQTT existed;
that historical verification path is described below.

---

## 16. Initial Implementation Increment

The first LoRa Service implementation exists to complete an end-to-end receive path using the already working remote sensor.

The target path is:

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
RX_PACKET
  |
TCP
  |
LoRa Service
  |
SENSOR_REPORT parser
  |
Console / diagnostic log
```

This increment proves:

1. The production sensor can generate a valid `SENSOR_REPORT`.
2. The gateway can transport that packet without understanding it.
3. Gateway Protocol TCP framing works.
4. The LoRa Service can associate a connection with a Gateway ID.
5. `RX_PACKET` radio metadata is preserved.
6. The service assigns the UTC reception timestamp.
7. The service correctly interprets the LoRa application protocol.
8. A complete real sensor report can be observed at the service.

The first increment does **not** require:

- MQTT
- DenClock integration
- Database storage
- Historical data
- `TX_PACKET`
- `COMMAND`
- `RESPONSE`
- `EVENT`
- Remote antenna control
- Production service installation

Windows Service hosting and MQTT publishing have since been implemented and accepted.
Semantic mapping has now been removed in favor of numeric device identity. The other capabilities remain
for later increments; see the implemented contract above.

---

## 17. Design Principles

The LoRa Service follows these principles:

### Keep transport and application semantics separate

The gateway transports opaque LoRa packets.

The LoRa Service interprets them.

### Keep infrastructure and applications separate

The LoRa Service provides LoRa infrastructure.

DenClock and future ham-radio applications own their application behavior.

### Specify only what is needed

Future capabilities shall not add fields, flags, abstractions, persistence, or configuration merely because they might someday be useful.

They shall be introduced when a concrete requirement exists.

### Preserve contracts

Once a protocol representation is published and depended upon, its meaning shall not be silently changed.

Changed semantics require an explicit protocol extension.

### Keep deployment flexible

The initial deployment on the DenClock NUC shall not unnecessarily bind the LoRa Service to that machine, Windows, or DenClock.

---

## 18. Related Documents

[Architecture](ARCHITECTURE.md)

Defines the overall DenClock system architecture and component responsibilities.

[LoRa Protocol](LoRa%20Protocol.md)

Defines the custom LoRa application protocol, radio configuration, Message Classes, `SENSOR_REPORT`, Node IDs, sequence semantics, and TLV definitions.

[Gateway Protocol](Gateway%20Protocol.md)

Defines the TCP/IP protocol used between gateways and the LoRa Service, including `HELLO`, `RX_PACKET`, and `TX_PACKET`.

[LoRa Service Semantic Mapping](Lora%20Service%20Semantic%20Mapping.md)

Describes removal of configuration-driven naming and the current normalization boundary.

[MQTT Interface](MQTT-INTERFACE.md)

Defines semantic sensor publication, including topics, payloads, QoS, retention,
and basic failure/recovery behavior.

This document defines the responsibilities of the **LoRa Service component**
that connects transport, wire interpretation, normalization, and publication.
