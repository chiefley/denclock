# Gateway error-indication investigation

Source inspection date: 2026-09-24. No firmware code was modified and no hardware tests were performed. This investigation covers the production gateway in `../lora-gateway/`.

## Finding

No Wi-Fi or TCP error directly triggers the gateway's full-screen inverse `PACKET ERROR` indication. There are exactly two triggering call paths: a LoRa packet read failure and a received packet that fails sensor-report validation.

[`updateDisplay()`](../lora-gateway/src/main.cpp#L18) draws the indication when `diagnostics.errorVisible` is true: a white 128×64 rectangle with black `PACKET ERROR` text. Only [`GatewayDiagnostics::reject()`](../lora-gateway/include/gateway_diagnostics.h#L21) sets that flag true.

## Triggering call paths

### 1. LoRa packet read failure

`packetReceived()` sets `received` → `loop()` calls `radio.readData()` → any result other than `RADIOLIB_ERR_NONE` calls `diagnostics.reject("radio packet failure", state, millis())` → `updateDisplay()` draws the error.

This includes observable CRC/read failures. See [`main.cpp`, lines 144–162](../lora-gateway/src/main.cpp#L144). Reception is rearmed before the rejection is processed; if that rearm fails, the firmware halts in `startRadioReceive()` instead.

### 2. Sensor-report validation failure

Successful `radio.readData()` → `diagnostics.observe()` → `SensorReport::decode()` returns an error → `reject()` → `updateDisplay()`.

See [`GatewayDiagnostics::observe()`](../lora-gateway/include/gateway_diagnostics.h#L30). The complete decoder rejection conditions are:

- Packet length below 4 or above 255 bytes.
- Message class other than 1.
- Source node 0 or above `0xEF`.
- Truncated TLV header or value.
- Reserved TLV type 0.
- Known measurement TLV (types 1–3) whose length is not 2.
- Duplicate temperature, humidity, or battery measurement.
- Humidity above 10000 (100.00%).

See [`SensorReport::decode()`](../common/sensor_report.h#L34). Missing measurements and unknown nonzero TLV types are accepted. Silent radio filtering, such as a mismatched sync word that produces no receive event, does not reach these paths.

## Wi-Fi and network conditions

| Condition | Code path and behavior | Triggers indication? |
|---|---|---|
| Wi-Fi disconnected, including initial connection failure | `maintainConnection()` stops TCP and calls `WiFi.reconnect()` at five-second intervals. | No |
| Wi-Fi reconnect attempts or unsuccessful reconnects | Same path; the reconnect return value is not checked. Automatic reconnect is also enabled during setup. | No |
| TCP connection loss or failed connection attempt | `maintainConnection()` retries TCP at five-second intervals; a failed `connect()` returns. | No |
| Failed HELLO or packet transmission | `sendFrame()` logs failure, stops TCP, and resets retry timing if the entire frame was not written. Partial writes are retried while the connection remains active. | No |
| Packet arrives while Wi-Fi or TCP is disconnected | `loop()` logs “service disconnected” and drops forwarding. | No |
| Forwarding metadata cannot be encoded | `loop()` logs “radio metadata outside wire range.” | No |
| Missing network configuration | `setup()` logs configuration guidance; connection maintenance is disabled. | No |

Relevant source:

- [`maintainConnection()`](../lora-gateway/src/main.cpp#L76)
- [`sendFrame()`](../lora-gateway/src/main.cpp#L61)
- [Forwarding branches in `loop()`](../lora-gateway/src/main.cpp#L163)
- [Network setup](../lora-gateway/src/main.cpp#L131)

A malformed LoRa packet can still trigger the indication while Wi-Fi is disconnected because local validation runs before the forwarding connection check. Conversely, a locally valid packet still updates the accepted counter and latest report while disconnected.

OLED validation is observational: a packet rejected by the sensor decoder can still be forwarded over TCP if the radio read succeeded, the connection is available, and forwarding encoding succeeds.

## Other errors and display timing

- Radio initialization failures log and halt; they do not activate the indication. See [`setup()`](../lora-gateway/src/main.cpp#L124).
- `radio.startReceive()` failures log and halt; they do not activate the indication. A halt can leave whatever was already on the OLED displayed indefinitely. See [`startRadioReceive()`](../lora-gateway/src/main.cpp#L102).
- Each rejection increments `ERR`, records the reason and radio error code, and restarts the three-second indication interval.
- Valid packets received during that interval update the latest report but do not dismiss the indication early. Expiry restores the latest valid report, or placeholders if none has been accepted.
- Expiry is checked by `diagnostics.tick()` inside `updateDisplay()`. Blocking network operations can delay display servicing and expiry, but cannot activate the indication.
- The separate `diagnostic` PlatformIO environment builds the serial-only receiver and has no OLED indication. See [`platformio.ini`](../lora-gateway/platformio.ini).

See [`GatewayDiagnostics`](../lora-gateway/include/gateway_diagnostics.h#L21) for rejection state and timing behavior.
