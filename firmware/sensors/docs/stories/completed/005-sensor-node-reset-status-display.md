## Story: Sensor Node Reset Status Display

Enhance the battery-powered LoRa sensor-node firmware so the Heltec OLED provides a brief local identification and battery-status display after a deliberate reset or power-up.

The purpose is field diagnostics: someone standing at a sensor should be able to press Reset and immediately determine which node it is and its current battery voltage without connecting a computer or consulting the gateway.

### Startup display

On a true reset/power-up, turn on the OLED and display:

- Node ID
- Current battery voltage

Use the available 128×64 pixels efficiently. Make the node ID the visually dominant information.

Conceptually:

```text
       NODE 03

       3.91 V
```

The exact pixel layout is not prescribed. Inspect the OLED library and fonts already used by the project and choose appropriately sized fonts for good readability.

Do **not** display temperature, humidity, RSSI, SNR, or other application/sensor information.

### Display duration

Keep the startup information visible for approximately **5 seconds**.

After that interval, shut down the OLED so that it does not contribute significant current during normal battery-powered operation.

Use the strongest OLED power-down mechanism supported by the existing Heltec hardware and firmware architecture. If the board provides GPIO-controlled OLED power, disable that power after sending any appropriate display-off command.

Do not merely clear the screen while leaving an avoidable OLED power load active.

### Deep-sleep wakeups

The five-second startup display must **not** occur after ordinary periodic wakeups from ESP32 deep sleep.

Normal sensor operation should remain:

1. Wake from deep sleep.
2. Read the sensor and battery as currently implemented.
3. Transmit the normal LoRa packet.
4. Return to deep sleep.

The OLED should remain off throughout those routine measurement/transmission wakeups.

Use the ESP32 wake/reset reason or the mechanism most appropriate to the existing firmware to distinguish a deliberate reset/power-up from a deep-sleep timer wakeup.

### Reset behavior

Pressing the board's Reset button should cause the startup diagnostic display to appear again for approximately five seconds.

Power cycling the node should likewise cause the startup diagnostic display to appear.

If other reset causes are distinguishable, preserve sensible behavior and avoid unnecessary complexity. The important distinction for this story is:

**manual reset / power-up → show display**

**routine deep-sleep wakeup → do not show display**

### Battery measurement

Use the node's existing battery-voltage measurement mechanism rather than introducing a second measurement implementation.

The displayed voltage should represent the same battery-voltage information that the node reports through the existing packet infrastructure.

Format the voltage appropriately for the display, for example:

```text
3.91 V
```

Two decimal places are sufficient.

### Power constraints

This is a battery-operated device. Preserve the existing low-power design.

After the startup-display interval:

- OLED should be off/powered down as completely as practical.
- No new polling loop should keep the ESP32 awake.
- Existing deep-sleep behavior should remain intact.
- The startup display should add power consumption only when the node is deliberately reset or power-cycled.

A simple five-second blocking delay during the deliberate startup diagnostic display is acceptable **if** it does not interfere with required initialization or watchdog behavior. Unlike the continuously operating gateway, there is no requirement to receive packets concurrently during this diagnostic interval.

### Constraints

Preserve the existing LoRa packet format, sensor-reading behavior, reporting interval, node configuration, and deep-sleep behavior except where modification is necessary for this story.

Do not turn the node OLED into a sensor display. Its purpose is local infrastructure identification and battery diagnostics.

Prefer the smallest coherent change to the existing firmware.

### Acceptance criteria

1. Powering up a sensor node displays its node ID and battery voltage.
2. Pressing Reset displays the same information.
3. Node ID is visually prominent.
4. Battery voltage is displayed to approximately two decimal places.
5. No temperature, humidity, or other environmental measurement is displayed.
6. The startup display remains visible for approximately five seconds.
7. The OLED shuts down afterward using the lowest-power mechanism reasonably available on the Heltec hardware.
8. An ordinary timer wake from deep sleep does **not** illuminate the OLED or wait five seconds.
9. Existing sensor measurement, LoRa transmission, and deep-sleep operation continue normally.
10. The existing battery-voltage measurement implementation is reused.
11. The existing LoRa packet protocol is unchanged.
12. Build the firmware and resolve any compilation errors introduced by the change.

### Acceptance

Accepted by the developer on 2026-09-24. Implemented the five-second node and
battery display on non-deep-sleep resets, reusing the existing battery routine.
Shared Vext is cut after diagnostics and during sleep; OLED reset remains LOW
when Vext is restored for sensor operation. No intentional scope departures.
PlatformIO firmware build, existing sensor packet regression tests, and diff
checks passed. Codex did not independently verify hardware behavior or current
consumption.
