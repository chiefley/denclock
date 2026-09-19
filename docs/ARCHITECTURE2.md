# Den Clock and Weather — Architecture

Status: agreed architectural direction; wire formats and implementation details remain TBD.

## Project context

DenClock provides a reliable clock and weather display as a web service with multiple browser clients. The den touchscreen is one client; other browsers can use the same service. Remote access belongs at the web-service/network boundary.

[Den-Clock-Setup.md](sources/Den-Clock-Setup.md) records the initial hardware: a Beelink Mini 8 NUC, ROADOM 10.1-inch 1024×600 touchscreen, ESP32 LoRa V3 boards with SX1262 radios and Wi-Fi, and an Adafruit SHT40 temperature/humidity sensor.

## System boundaries

```text
LoRa nodes <-> LoRa/Wi-Fi gateway <-> MQTT broker <-> Application services
                                                        |
                                                   Browser clients
```

- **Gateway:** an application-agnostic, intentionally dumb bridge. It publishes received LoRa packets to MQTT over Wi-Fi and transmits outgoing packets received through MQTT. It does not interpret application payloads or decide application persistence policy. The same bridge supports DenClock sensors and ham-radio nodes.
- **Transport metadata:** the gateway may attach facts such as a UTC reception timestamp, RSSI, and SNR while preserving the original packet. Reception time is distinct from measurement time.
- **MQTT broker:** initially runs on the DenClock NUC. It distributes messages through publish/subscribe; it is not historical storage. Delivery/session persistence and retained messages do not replace application history.
- **Application services:** DenClock and ham-radio services own payload semantics, current state, history, and application transaction handling. They also own the meaning and publication policy of any retained application topics. Gateway traffic represents packet transport; exact topic and retention configuration is TBD.
- **Browser clients:** present the service's information. Application state and history belong to the service rather than an individual display.

## Node behavior and message families

Sleeping DenClock sensors wake and spontaneously report measurements, normally expecting no response. An unsolicited sensor report needs no destination field.

Powered ham-radio nodes support command/response interactions and may also emit events. Commands are a separate message family, with their own addressing and interaction requirements; they need not share the sensor-report header.

Message class selects the header structure used to parse a packet. Sensor reports and command traffic may therefore have different headers instead of a universal envelope containing fields irrelevant to some messages.

## Sensor-report protocol direction

- A small fixed header is followed by Type-Length-Value (TLV) fields. Its fixed layout determines where TLVs begin.
- Once a TLV type is defined, its meaning and representation are immutable. Changed semantics or representations require a new type.
- Receivers can skip unknown TLV types using their lengths and continue parsing known fields.
- No protocol version field is currently needed. TLVs provide payload extensibility; this does not imply permission to reinterpret existing headers or types.
- No destination field is needed for unsolicited sensor reports.

## TBD — deliberately unresolved

- Exact message-class values, header field layouts and widths, node identification, and sequence-number behavior. Byte layouts discussed so far are illustrative, not a finalized wire specification.
- TLV type assignments, type/length encoding, units, value encodings, byte order, and packet-size limits.
- Command/response and event formats, destination addressing, transaction correlation, timeouts, retries, and delivery guarantees.
- MQTT topic names, packet/metadata envelope encoding, QoS, and gateway-topic retention settings.
- Application storage, history retention, retained application topics, deployment software, security, and remote-access configuration.
- Parser implementation and any factory/polymorphic class design.

## Decision source

[Gateway architecture design](chatgpt-conversation://6aae93e1-8e04-83e9-bb06-8563d690bd69), with hardware context from the setup document linked above. This note records architectural boundaries rather than prescribing an implementation.
