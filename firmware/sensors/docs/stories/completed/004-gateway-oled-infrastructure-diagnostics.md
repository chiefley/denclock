# Gateway OLED Infrastructure Diagnostics

Enhance the existing USB-powered LoRa/Wi-Fi gateway with a continuously visible
Heltec 128×64 OLED infrastructure/debug display. Power consumption is not a concern.

## Requirements

- After each successfully received and validated sensor-node packet, show the most
  recent node ID, RSSI (dBm), SNR (dB), node battery voltage, cumulative accepted
  packet count since boot, and cumulative rejected/error packet count since boot.
- Do not show temperature, humidity, or other environmental measurements.
- Make node ID the visual anchor, preferably with a larger font. Inspect the
  existing OLED library and actual font dimensions; use the 128×64 pixels well.
  SNR need not have unnecessary precision. The conceptual layout is:

  ```text
        NODE 03
  RSSI -87   SNR +8
  BAT  3.91V
  RX 18427  ERR 003
  ```

- A rejected packet must produce a conspicuous, predominantly illuminated/inverse
  screen with a large `PACKET ERROR` message for approximately three seconds.
- Restore the normal diagnostic display automatically. Use timestamp/state logic,
  never a three-second blocking delay. Packets arriving during the indication
  must still be processed normally.
- Count every received-packet rejection observable by radio/protocol validation.
  Retain its reason internally using an enum or equivalent representation.
  Relevant reasons include radio/CRC failure, malformed packets, unsupported
  protocol, invalid node, and other validation failures. Do not invent checks
  absent from the protocol or library. Per-category counts need not be displayed.
- Preserve the wire protocol and existing gateway handling/forwarding except for
  changes required here. Keep the change small; do not restructure unrelated code
  or build a weather/user-interface display.

## Implementation scope

The production Wi-Fi/TCP build is the target. Reuse the existing sensor-report
decoder for the OLED's diagnostic acceptance decision without filtering the raw
TCP transport, which already delegates application rejection to the LoRa Service.
RX counts locally validated reports independently of TCP availability; ERR counts
radio failures and reports rejected by that decoder. Transport disconnection is
not a bad sensor packet. Preserve the decoder's exact static error reason and
RadioLib error code. There is no protocol-version field or known-node registry.
Missing battery TLVs are legal and should display N/A, never an older node's value.
The serial-only diagnostic environment remains unchanged.

## Acceptance criteria

1. Valid packets update node ID, RSSI, SNR, battery, RX, and ERR.
2. Environmental measurements are absent from the OLED.
3. Node ID is visually prominent.
4. RX increments for successfully accepted packets.
5. ERR increments for observable rejected packets.
6. Rejections show a conspicuous inverse/high-illumination screen for about three seconds.
7. Reception and processing continue during that interval.
8. Normal diagnostics return automatically, showing the latest accepted node.
9. Firmware retains the rejection reason internally.
10. Existing packet handling and forwarding continue functioning.
11. Build the firmware and resolve introduced compilation errors.

## Verification

Build production and diagnostic firmware. Run portable tests covering decoder,
transport, counters, retained failure reasons, latest-node updates during the
error interval, repeated errors, missing battery, and timer rollover.
On hardware, confirm screen fit/visibility, inject malformed packets and observable
radio failures, send valid packets during the error interval, and confirm unchanged
TCP forwarding. Human acceptance is required before story closure.

## Acceptance

The developer confirmed that all checks passed and accepted the story as complete
on 2026-09-24. The production gateway provides infrastructure-only OLED diagnostics
and a nonblocking error indication while preserving raw forwarding. Production and
diagnostic firmware builds and all three portable test suites passed, including
error-timer rollover and valid reports during alerts. Diagnostic rejection remains
observational as specified above; there were no further departures.
