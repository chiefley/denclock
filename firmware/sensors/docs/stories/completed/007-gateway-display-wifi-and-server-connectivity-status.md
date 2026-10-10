# Gateway: Display Wi-Fi and Server Connectivity Status

**Type:** User Story

**Component:** Heltec ESP32 LoRa/Wi-Fi Gateway

**Priority:** Medium

**Motivation:** DenClock outage of October 9, 2026

As the DenClock operator, I want the gateway's OLED display to show its Wi-Fi
and LoRaService connection status so that I can immediately distinguish a
network failure from a LoRa sensor failure without connecting a serial monitor.

## Requirements

- Show Wi-Fi connectivity using `WiFi.status()`: `WiFi: OK` with RSSI in dBm
  when connected, and `WiFi: DOWN` when disconnected.
- Independently show TCP connectivity using `service.connected()` as
  `Server: OK` or `Server: DOWN`. Wi-Fi connectivity does not imply server connectivity.
- Integrate indicators into the existing OLED layout, preserving readable node,
  LoRa RSSI/SNR, battery, RX/ERR counters, and the existing packet-error alert.
  The supplied temperature/humidity layout is illustrative; preserve the existing
  infrastructure-only sensor diagnostics rather than adding environmental readings.
- Update indicators on connection-state changes; avoid unnecessary refreshes and
  flicker. Refresh connected Wi-Fi signal strength periodically.
- Preserve Serial connection/reconnection events and Wi-Fi disconnection reason
  codes, automatic Wi-Fi/TCP reconnection, LoRa reception, packet handling, and
  the gateway protocol.
- Do not introduce blocking network operations into the display update path.

## Acceptance criteria

1. Normal operation displays connected Wi-Fi, RSSI, and connected server.
2. Disabling router Wi-Fi displays `WiFi: DOWN` and `Server: DOWN`.
3. Stopping LoRaService displays `Server: DOWN` while Wi-Fi remains connected.
4. Restoring connectivity automatically updates both indicators without reset.
5. LoRa reception and existing sensor diagnostics continue during outages.
6. Existing information and both status indicators fit and remain readable.
7. Serial retains connection/reconnection events and Wi-Fi reason codes.

## Verification

Build production and diagnostic environments and run portable diagnostics tests.
On hardware, check normal operation, router outage/recovery, service stop/restart,
RSSI updates, sensor reception during both outages, unchanged forwarding after
recovery, and packet-error-screen restoration to current connectivity.
Human acceptance is required before closure.

## Out of scope

Gateway TCP protocol changes, MQTT on the gateway, sensor firmware changes,
router changes, and remote network diagnostics or monitoring infrastructure.

## Acceptance

The developer accepted the story as complete on 2026-10-09. The gateway displays
independent Wi-Fi/RSSI and TCP status while preserving sensor diagnostics,
packet-error alerts, transport, and reconnection behavior. Production and
diagnostic firmware builds, portable diagnostics, protocol, and sensor-decoder
tests passed. Hardware checks were left to human review; Codex did not independently
verify hardware behavior. The illustrative environmental layout was adapted to
the existing infrastructure-only diagnostics as specified above.
