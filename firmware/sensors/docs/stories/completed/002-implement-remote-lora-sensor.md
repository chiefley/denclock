# Story: Implement Remote LoRa Sensor

## Goal

Develop the first production-intent firmware for the Den Clock and Weather remote environmental sensor.

The sensor shall read temperature, relative humidity, and battery voltage and transmit them as a protocol-compliant `SENSOR_REPORT` over LoRa.

The existing LoRa TX/RX bringup programs are known-good hardware diagnostics and shall remain unchanged.

## Project Locations

Sensor firmware:

```text
/remote-sensor
```

Initial receiver/gateway test firmware:

```text
/lora-gateway
```

Existing hardware test programs remain under:

```text
/bringup
```

## Remote Sensor Requirements

The remote sensor shall:

1. Initialize the SX1262 LoRa radio.
2. Initialize the SHT40 temperature/humidity sensor.
3. Read:
   - temperature;
   - relative humidity;
   - battery voltage.
4. Construct a `SENSOR_REPORT` according to `LORA-PROTOCOL.md`.
5. Transmit the report over LoRa.
6. Maintain the defined report sequence number.
7. Initially repeat reports at a convenient development interval.

Deep sleep is not required for the first implementation. It will be added after packet generation and transmission have been verified.

## SENSOR_REPORT

The sensor shall transmit the defined four-byte fixed header:

```text
+---------------+---------------+--------------------+
| Message Class | Source Node ID| Sequence Number    |
| 1 byte        | 1 byte        | 2 bytes            |
+---------------+---------------+--------------------+
```

followed by TLVs.

### Message Class

```text
0x01 = SENSOR_REPORT
```

Enumeration value `0x00` is invalid/reserved.

### Source Node ID

The first development sensor shall use:

```text
Node ID = 0x01
```

Node ID `0x00` is invalid/reserved.

### Sequence Number

The first report following power-up or reset shall have:

```text
Sequence = 0
```

Subsequent reports shall increment:

```text
0, 1, 2, 3 ... 65535
```

After `65535`, the sequence shall roll over to:

```text
1
```

It shall not roll over to zero.

## Required TLVs

Each report shall contain:

```text
0x01 = TEMPERATURE
0x02 = RELATIVE_HUMIDITY
0x03 = BATTERY_VOLTAGE
```

TLV format:

```text
+----------+----------+----------------+
| Type     | Length   | Value          |
| 1 byte   | 1 byte   | Length bytes   |
+----------+----------+----------------+
```

### TEMPERATURE

```text
Type:           0x01
Length:         2
Representation: signed 16-bit integer
Units:          0.01 °C
```

### RELATIVE_HUMIDITY

```text
Type:           0x02
Length:         2
Representation: unsigned 16-bit integer
Units:          0.01 %RH
```

### BATTERY_VOLTAGE

```text
Type:           0x03
Length:         2
Representation: unsigned 16-bit integer
Units:          millivolts
```

All multibyte integers shall use network byte order (big-endian).

## LoRa Configuration

The sensor and receiver shall use:

```text
Radio:             SX1262
Frequency:         915.0 MHz
Bandwidth:         125.0 kHz
Spreading Factor:  9
Coding Rate:       7
TX Power:          10 dBm
Sync Word:         0xD3C7
CRC:               Enabled
Preamble:          RadioLib default
```

The sync word provides network discrimination but is not an authentication or security mechanism.

## Initial Gateway Test Receiver

The initial `/lora-gateway` implementation is primarily an acceptance test for the remote sensor.

It shall:

1. Configure the SX1262 with the same LoRa parameters.
2. Receive `SENSOR_REPORT` packets.
3. Reject invalid packets.
4. Parse the fixed header.
5. Parse the TLV sequence.
6. Recognize the three initial sensor TLVs.
7. Display the decoded packet and LoRa reception information over the serial console.

Example diagnostic output:

```text
RX node=1 seq=27 RSSI=-72 SNR=9.5

TEMPERATURE        22.37 C
RELATIVE_HUMIDITY  41.82 %
BATTERY_VOLTAGE    3976 mV
```

The TLV interpretation in this first gateway implementation is diagnostic scaffolding. The final gateway shall remain application-agnostic and shall not interpret environmental measurements.

## Acceptance Criteria

The story is complete when:

- the remote sensor obtains valid readings from the SHT40;
- battery voltage is measured;
- those readings are encoded into a valid 16-byte `SENSOR_REPORT`;
- the packet uses the specified Message Class, Node ID, sequence number, TLV Types, and byte order;
- the packet is transmitted using the specified LoRa configuration and network sync word;
- the `/lora-gateway` test receiver receives and validates the packet;
- the receiver correctly displays all three measurements;
- sequence `0` is observed after resetting the sensor;
- subsequent packets increment the sequence correctly;
- unrelated or structurally invalid packets are rejected;
- the existing `/bringup` programs remain available as known-good hardware diagnostics.

## Deferred

The following are explicitly outside this story:

- deep sleep;
- production reporting interval;
- Wi-Fi;
- MQTT;
- MQTT topic definitions;
- historical storage;
- DenClock integration;
- command/response protocol;
- command authentication;
- OTA firmware updates;
- configuration UI.

These will be addressed after the sensor-to-gateway LoRa path is working reliably.

## Repository mapping

Use the existing `firmware/sensors/remote-sensor/` and
`firmware/sensors/lora-gateway/` projects. Hardware diagnostics remain in
`firmware/bringup/`. The protocol reference is `docs/LoRa Protocol.md`.

## Implementation verification

- Both PlatformIO Heltec V3 builds pass with RadioLib 7.7.1.
- Portable packet tests pass with address and undefined-behavior sanitizers.
  Leak detection was disabled because the sandbox's tracing prevents LeakSanitizer
  from running; packet tests use no manual heap allocation.
- Bringup sources are unchanged. Existing user edits to the protocol are preserved.
- Hardware verification was performed by the developer, who reported:
  “Everything works as advertised.” Codex did not independently observe the boards.
- On invalid sensor readings the report is skipped and initialization retried.
  Sequence advances per generated report, even if radio transmission fails.
- Gateway accepts unknown TLVs and optional measurements as specified by the
  protocol; this sensor always emits all three required measurements.

## Acceptance

Accepted by the human developer on 2026-09-19: “Everything works as advertised.”
The sensor and diagnostic gateway implementation is complete for this story;
deferred features remain outside its scope.
