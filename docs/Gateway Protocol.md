# Den Clock and Weather — Gateway Protocol

## 1. Scope

This document defines the protocol between a LoRa/Wi-Fi Gateway and the LoRa Service.

The purpose of the Gateway Protocol is to transport LoRa packets between the radio network and the LoRa Service over an IP network.

The gateway is intentionally application-agnostic. It does not interpret the Den Clock and Weather LoRa application protocol and does not know the meaning of the packets it transports.

In particular, the gateway does not interpret:

- LoRa Message Classes;
- Node IDs;
- sensor TLVs;
- temperature;
- humidity;
- battery voltage;
- antenna-control commands;
- application responses.

The gateway moves packets and reports radio reception information.

The LoRa Service owns interpretation of the LoRa application protocol.

MQTT is outside the scope of the Gateway Protocol. The gateway does not communicate with MQTT.

---

## 2. Architecture

The Gateway Protocol occupies the following system boundary:

```text
                         Gateway Protocol
                               TCP/IP
                                 |
                                 |
LoRa Node <-- LoRa --> Gateway <-------> LoRa Service <-- MQTT --> Applications
```

The responsibilities are:

```text
LoRa Node
    Owns application-specific sensing or control behavior.

Gateway
    Owns LoRa radio transport and IP transport.

LoRa Service
    Owns the LoRa application protocol.

MQTT
    Provides application-level message distribution.

Applications
    Own application semantics, state, history, and presentation.
```

The Gateway and LoRa Service may run on entirely different devices.

The physical location of the LoRa Service is not part of this protocol.

---

## 3. Transport

The Gateway Protocol uses a persistent TCP connection.

The gateway initiates the TCP connection to a configured LoRa Service address and port.

After establishing a connection, the gateway immediately sends a `HELLO` message.

The connection remains open for bidirectional traffic.

If the TCP connection is lost, the gateway shall attempt to reconnect. After each successful reconnection, it shall send a new `HELLO` message.

The mechanism by which the gateway obtains its Wi-Fi credentials, LoRa Service address, TCP port, and Gateway ID is outside the scope of this protocol.

---

## 4. General Message Format

Every Gateway Protocol message begins with a three-byte common header:

```text
 Byte
   0          1          2
+----------+---------------------+
| Message  |   Payload Length    |
| Type     |      16 bits        |
+----------+---------------------+
```

The fields are:

```text
Message Type       uint8
Payload Length     uint16
```

`Payload Length` specifies the number of bytes following the common header.

It does not include the three-byte common header itself.

All multibyte integer values are transmitted in network byte order (big-endian).

Because TCP is a byte-stream protocol, receivers shall use `Payload Length` to determine message boundaries. A single Gateway Protocol message is not assumed to correspond to a single TCP read operation.

---

## 5. Enumeration Rules

Enumerated protocol identifiers begin at 1.

The value zero is reserved and invalid.

Receipt of an enumerated value of zero indicates a malformed message, uninitialized value, or implementation error.

Initial Gateway Protocol Message Types are:

```text
0x00    INVALID / RESERVED
0x01    HELLO
0x02    RX_PACKET
0x03    TX_PACKET
```

Once assigned, a Message Type and its wire representation shall not be redefined with different semantics.

New functionality shall be introduced using new Message Types.

---

## 6. HELLO

`HELLO` identifies a gateway to the LoRa Service.

The gateway shall send `HELLO` immediately after establishing a TCP connection.

### Payload

```text
+------------+
| Gateway ID |
|   uint8    |
+------------+
```

Payload Length:

```text
1
```

### Gateway ID

Gateway ID is an unsigned 8-bit identifier assigned to the gateway.

Gateway ID zero is invalid.

The initial gateway may use:

```text
Gateway ID = 0x01
```

The Gateway ID applies to the TCP connection on which the `HELLO` was received. It therefore does not need to be repeated in each subsequent message.

The assignment and administration of Gateway IDs is outside the wire protocol.

---

## 7. RX_PACKET

`RX_PACKET` reports a LoRa packet received by the gateway.

The gateway does not interpret the LoRa packet.

### Payload

```text
+----------+----------+-----------------------------+
| RSSI     | SNR      | Raw LoRa Packet             |
| int8     | int8     | N bytes                     |
+----------+----------+-----------------------------+
```

Payload Length is:

```text
2 + LoRa packet length
```

### RSSI

RSSI is represented as a signed 8-bit integer in dBm.

Example:

```text
-72 = -72 dBm
```

### SNR

SNR is represented as a signed 8-bit integer in units of 0.25 dB.

Examples:

```text
 38 =  +9.5 dB
  0 =   0.0 dB
-12 =  -3.0 dB
```

This representation avoids floating-point values in the Gateway Protocol while preserving the useful resolution of the radio measurement.

### Raw LoRa Packet

All remaining bytes in the payload are the LoRa packet exactly as received from the radio.

The gateway shall not modify, decode, translate, or reformat these bytes.

For example, if the gateway receives a 16-byte LoRa packet, the `RX_PACKET` payload length is:

```text
2 bytes reception metadata
16 bytes LoRa packet
--------------------------
18 bytes payload
```

---

## 8. Reception Timestamp

The gateway does not timestamp received LoRa packets.

The LoRa Service shall assign the authoritative UTC reception timestamp immediately upon receiving an `RX_PACKET`.

The timestamp therefore means:

> The time at which the LoRa Service received the `RX_PACKET` from the gateway.

It does not represent the exact time at which:

- a sensor measurement was taken;
- LoRa transmission began;
- the gateway received the RF packet.

Gateway-to-service transport latency over the local IP network is considered negligible for the intended applications.

Keeping the authoritative timestamp in the LoRa Service avoids requiring the gateway to maintain a synchronized real-time clock.

---

## 9. TX_PACKET

`TX_PACKET` instructs the gateway to transmit a LoRa packet.

It is sent from the LoRa Service to the gateway.

The LoRa Service constructs the complete LoRa application packet. The gateway does not interpret it.

### Payload

```text
+-----------------------------+
| Raw LoRa Packet             |
| N bytes                     |
+-----------------------------+
```

Payload Length is equal to the number of LoRa packet bytes to transmit.

Upon receipt of a valid `TX_PACKET`, the gateway transmits the payload bytes using the configured LoRa radio parameters.

The gateway shall not modify, decode, translate, or reformat the LoRa packet.

Command acknowledgements, application responses, retries, and transaction semantics are not part of `TX_PACKET`. They belong to the LoRa application protocol and the LoRa Service.

---

## 10. Radio Configuration

The Gateway Protocol does not specify LoRa radio parameters.

Radio parameters are defined by the LoRa network specification in `LORA-PROTOCOL.md`.

The gateway shall use the same configured radio parameters as the participating LoRa nodes.

These currently include:

```text
Frequency:          915.0 MHz
Bandwidth:          125.0 kHz
Spreading Factor:   9
Coding Rate:        7
TX Power:           10 dBm
Sync Word:          0xD3C7
CRC:                Enabled
Preamble:           RadioLib default
```

The Gateway Protocol transports the resulting LoRa packets without interpreting their contents.

---

## 11. Connection Loss

The gateway shall automatically attempt to reconnect if its TCP connection to the LoRa Service is lost.

After reconnecting, the gateway shall send a new `HELLO`.

The first implementation is not required to queue received LoRa packets while disconnected from the LoRa Service.

If a LoRa packet is received while no service connection is available, the gateway may discard the packet.

Periodic environmental sensor reports are expected to be transmitted again at their normal reporting interval.

Reliability requirements for future command/control traffic shall be handled at an appropriate higher protocol layer rather than by adding application semantics to the gateway transport.

---

## 12. Protocol Validation

A Gateway Protocol receiver shall reject a message if:

- Message Type is zero;
- Message Type is unsupported;
- Payload Length is invalid for the specified Message Type;
- a fixed-length message does not contain its required number of bytes;
- the message cannot otherwise be structurally parsed according to its Message Type.

An invalid Gateway Protocol message shall not be interpreted as a LoRa application packet.

Detailed connection recovery behavior following malformed TCP data may be defined by the implementation.

---

## 13. Protocol Extensibility

The Gateway Protocol intentionally contains only the information required to transport LoRa packets between the gateway and the LoRa Service.

New capabilities shall be introduced through new Message Types rather than by changing the interpretation of existing Message Types.

The common header contains no protocol version and no flags field.

Fields shall not be added merely because they may be useful in the future.

A proposed addition should satisfy the following design test:

> Does the gateway need this information to transport a LoRa packet between the radio and the LoRa Service?

If the answer is no, the information belongs at another layer.

---

## 14. Outside This Specification

The following are deliberately outside the Gateway Protocol:

- LoRa application Message Classes;
- Node IDs;
- sensor TLV definitions;
- sensor measurement interpretation;
- application commands and responses;
- application transaction semantics;
- MQTT;
- MQTT topics;
- MQTT QoS or retention;
- application history;
- DenClock behavior;
- antenna-control behavior;
- authentication of application commands;
- gateway Wi-Fi configuration;
- service discovery;
- deployment location of the LoRa Service;
- user-interface behavior.

These concerns may evolve independently without changing the Gateway Protocol.

---

## 15. Initial Protocol Summary

The initial Gateway Protocol contains only three valid Message Types:

```text
HELLO
    Gateway ID

RX_PACKET
    RSSI
    SNR
    Raw LoRa packet

TX_PACKET
    Raw LoRa packet
```

The gateway knows how to move packets.

The LoRa Service knows what the packets mean.
