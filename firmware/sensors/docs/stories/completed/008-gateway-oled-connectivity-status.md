# User Story: Gateway OLED Connectivity Status

**Project:** DenClock
**Component:** Heltec ESP32 LoRa/Wi-Fi Gateway
**Priority:** Medium
**Type:** Firmware enhancement

## Background

On October 9, 2026, DenClock stopped receiving sensor updates even though all LoRa sensors were functioning normally and the gateway continued receiving their packets.

The problem was traced to the ESP32 gateway losing its Wi-Fi connection. The ASUS RT-AC86U router's Smart Connect band-steering feature was implicated. Disabling Smart Connect restored normal operation.

The gateway continued displaying received LoRa sensor data, but offered no visible indication that its Wi-Fi and TCP connections were down. Diagnosis required connecting a serial monitor.

We want to make gateway connectivity immediately visible on its OLED.

## User Story

As the DenClock operator, I want the gateway OLED to display Wi-Fi and LoRaService connectivity status so that I can distinguish a communication failure from a sensor failure without connecting a serial monitor.

## Functional Requirements

### 1. Wi-Fi status

Display one of the following:

- `WiFi: OK -46dBm` when connected, showing the current RSSI.
- `WiFi: DOWN` when disconnected or attempting reconnection.

Use `WiFi.status()` and `WiFi.RSSI()`.

### 2. LoRaService status

Display:

- `Server: OK` when the TCP client is connected.
- `Server: DOWN` when disconnected.

Use the existing `service.connected()` state.

Wi-Fi connectivity and TCP connectivity must be reported independently. A working Wi-Fi connection does not imply that LoRaService is reachable.

### 3. OLED integration

- Inspect the existing `updateDisplay()` implementation and fit the status indicators into the existing 128×64 OLED layout.
- Preserve as much existing sensor information as possible.
- Use small, readable text appropriate for the display.
- Avoid excessive redraws and visible flicker.
- Refresh status when connectivity changes, even when no LoRa packets are arriving.
- RSSI may be refreshed periodically, approximately once per second, but avoid redrawing for insignificant fluctuations.

### 4. Serial diagnostics

Preserve the existing serial diagnostics.

Ensure useful messages are logged when:

- Wi-Fi connects and obtains an IP address.
- Wi-Fi disconnects, including the ESP32 reason code.
- TCP connects successfully to LoRaService.
- TCP connection attempts fail.
- TCP disconnects.

Avoid repetitive logging on every loop iteration.

### 5. Preserve existing behavior

Do not change:

- LoRa radio configuration or receive handling.
- Sensor packet decoding.
- Gateway protocol or HELLO frame format.
- TCP server address or port configuration.
- Existing Wi-Fi/TCP reconnection policy unless a demonstrable defect is found.

Do not introduce blocking delays into the main loop or display update path.

## Implementation Guidance

The existing gateway firmware already contains:

- `maintainConnection()`
- `updateDisplay()`
- `WiFi.status()`
- `WiFi.RSSI()`
- `service.connected()`
- Periodic Wi-Fi and TCP reconnection attempts using `RetryMs`

Reuse those facilities rather than introducing another network manager or reconnection mechanism.

Implement lightweight connectivity-state tracking and invoke the display refresh when either connection state changes.

Keep the connectivity logic separate from LoRa packet reception so that the OLED can show connection failures even if all sensors stop transmitting.

Inspect the actual OLED layout before deciding how to accommodate the two status lines.

## Acceptance Criteria

- [ ] Normal operation displays Wi-Fi connected, RSSI, and LoRaService connected.
- [ ] Disabling Wi-Fi causes `WiFi: DOWN` to appear without waiting for a sensor packet.
- [ ] Stopping LoRaService displays `Server: DOWN` while Wi-Fi remains connected.
- [ ] Restoring either connection updates the OLED automatically.
- [ ] RSSI updates periodically without excessive redraws.
- [ ] LoRa reception and existing sensor information remain functional.
- [ ] The gateway reconnects automatically after network recovery.
- [ ] Relevant connection transitions are reported over Serial.
- [ ] Firmware compiles successfully using the existing PlatformIO configuration.

## Testing

Verify the following scenarios:

1. Normal startup with Wi-Fi and LoRaService available.
2. Gateway startup with Wi-Fi unavailable.
3. Wi-Fi lost after successful operation.
4. Wi-Fi restored after an outage.
5. LoRaService stopped while Wi-Fi remains connected.
6. LoRaService restarted.
7. Sensor packets received while TCP is disconnected.
8. Prolonged operation with normal sensor traffic.

## Deliverables

- Implement the firmware changes directly in the existing gateway project.
- Build using PlatformIO.
- Report which files were modified.
- Summarize the implementation and any limitations.
- Do not modify the NUC's LoRaService, Mosquitto, or DenClock display applications.

**Scope:** Small, self-contained firmware enhancement. No architectural changes or new dependencies expected.

## Implementation scope

Accepted story 007 already provides the connectivity indicators, independent
loop-driven status checks, and preserved sensor diagnostics. This increment
uses one-second RSSI sampling and a 3 dBm threshold measured from the last
displayed value to suppress insignificant fluctuations, and adds an explicit
failed-TCP-attempt diagnostic. Existing reconnection policy remains unchanged.

## Acceptance

The developer accepted the story as complete on 2026-10-09. The gateway now
samples Wi-Fi RSSI every second, suppresses changes below 3 dBm from the displayed
value, and explicitly logs failed TCP connection attempts. Existing connectivity
indicators, sensor diagnostics, packet handling, protocol, and reconnection policy
are preserved. Production and diagnostic PlatformIO builds, portable diagnostics
tests, and diff checks passed. Codex did not independently verify hardware
outage/recovery or prolonged operation.
