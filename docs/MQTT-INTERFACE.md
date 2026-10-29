# MQTT Interface

The [current LoRa worker](../lora-service/docs/contract.md) implements this
publishing contract. Numeric identity replaces the previous named-topic contract;
existing DenClock consumption requires adaptation in Story 2.

## 1. Purpose

MQTT provides the application-facing messaging boundary between the LoRa Service and applications that consume information received through the LoRa network.

The LoRa Service owns interpretation of the compact LoRa application protocol and publishes normalized, semantic information to MQTT.

Applications consuming MQTT shall not require knowledge of `LORA-PROTOCOL.md` or `GATEWAY-PROTOCOL.md` to interpret published sensor data.

The initial MQTT implementation supports received `SENSOR_REPORT` messages only.

## 2. Architecture

The initial data path is:

```text
Remote Sensor
    |
    | LoRa application protocol
    v
Gateway
    |
    | Gateway Protocol / TCP
    v
LoRa Service
    |
    | protocol normalization
    v
MQTT Broker
    |
    | MQTT normalized interface
    v
Applications
```

Mosquitto is the initial MQTT broker.

The LoRa Service is an MQTT publisher. DenClock is initially an MQTT subscriber, but MQTT is not DenClock-specific. Other applications may independently subscribe to the same information.

## 3. Semantic Boundary

The MQTT interface contains application-friendly semantic information rather than LoRa wire-protocol representations.

The LoRa Service decodes the fixed measurement definitions from the LoRa protocol
into canonical values, units, and precision metadata. It publishes integer
`gatewayId` and `nodeId` without device names or location interpretation.
Applications own semantic naming. No mapping file is loaded.

The original raw LoRa packet is not included in normal MQTT sensor messages.

## 4. Topic Namespace

The initial sensor topic namespace is:

```text
lora/sensors/{gatewayId}/{nodeId}
```

For example:

```text
lora/sensors/1/1
lora/sensors/2/1
```

A subscriber interested in all sensor nodes may subscribe using the MQTT wildcard:

```text
lora/sensors/+/+
```

The `lora` root is intentionally independent of DenClock. DenClock is one consumer of the LoRa infrastructure rather than its owner.

## 5. Numeric Sensor Identity

The pair `(gatewayId, nodeId)` uniquely identifies a sensor. Node IDs need only
be unique within a gateway. Topics use decimal integers without leading zeros;
Gateway 1 / Node 1 and Gateway 2 / Node 1 use distinct retained topics.
IDs need no configured names and generate no missing-name warnings.
Gateway IDs must be assigned distinctly by deployment; the service does not
authenticate IDs, enforce unique gateway sessions, or deduplicate RF receptions.

## 6. Message Granularity

One received LoRa `SENSOR_REPORT` produces one MQTT message.

Measurements from a report are not normally split into separate MQTT topics or messages.

This preserves the report as a coherent observation from the sensor node and permits a single retained MQTT message to represent the most recently received report from that node.

## 7. Payload Format

Sensor reports are published as JSON.

A representative payload is:

```json
{
  "gatewayId": 1,
  "nodeId": 1,
  "messageType": "sensor-report",
  "sequence": 47,
  "receivedUtc": "2026-09-21T20:42:17.123Z",
  "radio": {
    "rssiDbm": -72,
    "snrDb": 9.5
  },
  "measurements": {
    "temperature": {
      "value": 21.43,
      "units": "C",
      "precision": 4
    },
    "relativeHumidity": {
      "value": 43.21,
      "units": "percent",
      "precision": 4
    },
    "batteryVoltage": {
      "value": 3.126,
      "units": "V",
      "precision": 4
    }
  }
}
```

## 8. Gateway Identity

`gatewayId` is an integer (1–255) from the gateway connection's HELLO.
No gateway name is published.

## 9. Node Identity

`nodeId` is an integer (1–239) from the sensor report. Consumers must key sensors
by both identifiers, including retained state and any future name configuration.
No node name is published. Payload identity remains usable independently of topics.

## 10. Message Type

`messageType` contains the semantic name of the LoRa application message class.

For the initial implementation:

```text
sensor-report
```

Future LoRa message classes may use different MQTT topic namespaces and payload contracts. They are outside the scope of this document.

## 11. Sequence

`sequence` contains the LoRa `SENSOR_REPORT` sequence number.

Its semantics remain those defined by `LORA-PROTOCOL.md`.

The sequence number is provided for diagnostics and for consumers that may wish to recognize restarts, missing reports, or duplicate observations.

MQTT consumers are not required to perform sequence tracking unless their application requires it.

## 12. Reception Timestamp

`receivedUtc` contains the authoritative UTC reception timestamp assigned by the LoRa Service after receiving a complete `RX_PACKET` from the gateway.

It represents LoRa Service receipt time, not exact RF reception time or sensor measurement time.

The timestamp allows applications to determine the age of a retained observation and decide when a last-known value has become stale.

## 13. Radio Metadata

The `radio` object contains metadata reported by the receiving gateway.

Initially:

- `rssiDbm`
- `snrDb`

This information is included because it is inexpensive and may be useful for diagnostics even when an application does not normally display it.

## 14. Measurements

`measurements` contains all known and successfully decoded measurement TLVs in the received `SENSOR_REPORT`.

Measurements use fixed protocol-defined keys: `temperature` (C),
`relativeHumidity` (percent), and `batteryVoltage` (V). Values are decoded with
scales 0.01, 0.01, and 0.001 respectively; temperature is signed int16, the others
unsigned uint16. Precision remains 4 significant figures for each measurement.
Missing measurements remain absent. New measurement types require protocol
support in LoRaService, rather than configuration.

Each measurement initially contains:

- `value` — normalized numeric value.
- `units` — canonical protocol units.
- `precision` — recommended presentation precision in significant figures.

The LoRa Service preserves numeric fidelity. `precision` is metadata and does not require the service to round the underlying value.

Applications may convert units or choose different display precision as appropriate to their presentation requirements.

## 15. Unknown Measurement Types

Unknown TLV Types cannot be converted into semantic measurements because their representation and meaning are unknown.

The LoRa Service shall:

- skip the unknown TLV using its Length field as defined by `LORA-PROTOCOL.md`;
- continue processing other valid TLVs in the report;
- generate an appropriate infrastructure warning.

An unknown measurement type does not cause the entire sensor report to be discarded.

## 16. Retained Messages

Published `SENSOR_REPORT` MQTT messages shall be retained.

Each newly published report replaces the previous retained message for that node's topic.

For example:

```text
lora/sensors/1/1
```

always retains the most recently successfully published report from Gateway 1 / Node 1.

This allows a newly started or restarted subscriber to immediately obtain the last known sensor report without waiting for the next LoRa transmission.

A retained message represents the **last known observation**, not necessarily a current observation.

Consumers shall use `receivedUtc` when the age of a measurement matters.

MQTT retained messages are not measurement history.

## 17. Quality of Service

`SENSOR_REPORT` messages shall use MQTT QoS 1.

QoS 1 provides at-least-once delivery between MQTT participants.

Consumers shall therefore tolerate the possibility of receiving the same MQTT message more than once.

No MQTT QoS 2 behavior is required.

## 18. Broker Configuration

MQTT connection information shall be configuration rather than compiled constants.

The initial configuration needs only the information required to connect to the Mosquitto broker, such as:

- broker host;
- broker port;
- MQTT client identity where required by the client library.

The initial broker is expected to run on the NUC.

The standard unencrypted MQTT port may be used on the trusted local network.

Authentication and TLS are outside the MVP and may be introduced if deployment requirements later justify them.

## 19. Broker Failure and Recovery

The implementation may use the normal connection and reconnection facilities provided by the selected MQTT client library.

The MVP does not require:

- local message spooling;
- an offline publish queue;
- dead-letter handling;
- guaranteed replay after broker failure;
- application-specific retry persistence.

If a sensor report cannot be published while MQTT is unavailable, loss of that individual report is acceptable.

A later sensor report will replace the retained state after connectivity is restored.

Unexpected or unhandled failures shall flow through the normal `ILogger<T>` logging infrastructure and therefore to the configured Serilog sinks.

## 20. Logging

MQTT operational events are infrastructure events and follow the LoRa Service logging policy.

Appropriate events include:

- broker connection;
- broker disconnection;
- reconnection;
- configuration failures;
- publish failures;
- unexpected MQTT exceptions.

Routine successful sensor measurements shall not normally be written to the infrastructure log.

MQTT carries measurement information; the log is not a measurement-history mechanism.

## 21. Application Independence

No MQTT topic or payload defined by this interface shall require DenClock to exist.

Likewise, DenClock shall interact with the LoRa sensor infrastructure through the MQTT contract rather than through implementation knowledge of the LoRa Service.

Conceptually:

```text
                         +--> DenClock
                         |
LoRa Service --> MQTT ---+--> Diagnostic subscriber
                         |
                         +--> Future applications
```

A conforming subscriber should be able to understand a sensor message using this MQTT interface contract without requiring knowledge of the LoRa or Gateway wire protocols.

## 22. Out of Scope

The initial MQTT interface does not define:

- command topics;
- response topics;
- event topics;
- antenna-control semantics;
- measurement history;
- database persistence;
- DenClock display configuration;
- alarms or thresholds;
- user-specific unit preferences;
- raw LoRa diagnostic topics;
- MQTT authentication;
- MQTT TLS;
- elaborate broker-failure recovery;
- multi-gateway routing or deduplication.

These shall be defined only when an actual requirement exists.

## 23. MVP Acceptance

The MQTT interface MVP is complete when a valid `SENSOR_REPORT` received and decoded by the LoRa Service can be published as semantic JSON to:

```text
lora/sensors/{gatewayId}/{nodeId}
```

using:

```text
QoS:      1
Retained: true
```

and an independent MQTT subscriber can obtain and understand:

- gateway identity;
- node identity;
- message type;
- sequence number;
- reception timestamp;
- RSSI and SNR;
- all known normalized measurements;
- units and precision metadata.

A subscriber that connects after the report was published shall immediately receive the most recent retained report for the subscribed node.

The implementation need provide no additional MQTT functionality for the MVP.
