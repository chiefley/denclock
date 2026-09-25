# 006: Sensor Change Reporting, Heartbeat, and Randomized Sampling

Modify the LoRa sensor-node firmware to reduce unnecessary radio traffic while guaranteeing periodic proof-of-life and preventing periodic transmissions from becoming synchronized between nodes.

### Goals

The node should:

1. Sample the sensor at a regular nominal interval.
2. Add fresh random jitter to every sampling/sleep interval.
3. Transmit immediately when temperature has changed sufficiently from the last **transmitted** temperature.
4. Transmit periodically even when temperature is stable, as a heartbeat/proof-of-life.
5. Include humidity and battery voltage whenever a packet is transmitted, but humidity changes must **not** independently cause a transmission.

Preserve the existing LoRa packet format.

### Sampling cadence

After every sampling cycle, calculate a new sleep interval using:

```text
next_interval = nominal_sample_interval + fresh_random_jitter
```

Generate fresh jitter on **every cycle**, regardless of whether that cycle resulted in a LoRa transmission.

Use the ESP32 hardware random facility already available in the platform rather than implementing a deterministic pseudo-random generator unless the existing codebase provides an appropriate mechanism.

The jitter exists primarily to prevent independently operating sensor nodes from drifting into persistent LoRa packet collisions.

The randomized interval must remain safely positive.

### Temperature-driven reporting

Apply the temperature-change threshold to the sensor measurement at the highest practical internal resolution available from the SHT40 reading, **before any rounding or quantization performed for packet encoding or display**.

For example, a configured threshold of:

```text
TEMPERATURE_CHANGE_THRESHOLD = 0.1 °C
```

means a change of 0.1 °C in the unrounded sensor measurement relative to the unrounded temperature associated with the last transmitted report.

Conceptually:

```text
abs(current_raw_temperature - last_transmitted_raw_temperature)
    >= TEMPERATURE_CHANGE_THRESHOLD
```

Here, `raw_temperature` means the high-resolution converted SHT40 measurement used internally by the firmware. It does **not** mean the 16-bit SHT40 ADC code must necessarily be retained directly.

Do not perform change detection using the rounded or quantized temperature value prepared for LoRa packet transmission.

When a report is transmitted:

1. Retain the current high-resolution temperature internally as `last_transmitted_raw_temperature`.
2. Separately convert/quantize that same measurement as required by the existing LoRa packet format.
3. Transmit the packet normally.

The state needed for `last_transmitted_raw_temperature` must survive routine ESP32 deep sleep.

This ensures that packet-format resolution does not determine when a meaningful accumulated temperature change has occurred.


Humidity changes must **not** independently trigger a transmission.

Whenever a packet is transmitted for either a temperature change or heartbeat, include the current humidity value using the existing packet format.

Humidity therefore comes along with the current report but does not control reporting cadence.

### Battery voltage

Continue including current battery voltage in every transmitted sensor report.

Battery-voltage changes do not need to independently trigger a transmission in this story.

### Heartbeat

A node must transmit periodically even if temperature remains stable.

If the maximum heartbeat interval has elapsed since the last transmitted packet, transmit the current temperature, humidity, and battery voltage.

This guarantees that the gateway/Den Clock can distinguish a stable sensor from a dead or unreachable sensor.

After the heartbeat packet is successfully transmitted, reset the heartbeat timing state in the same manner as any other successful report.

### First report after startup

After power-up/reset, ensure that the node transmits an initial sensor report without requiring the temperature-change threshold or heartbeat timeout to occur first.

After that initial transmission, normal temperature-change/heartbeat policy applies.

### Configuration

Define clearly named configuration constants for at least:

```text
SAMPLE_INTERVAL
SAMPLE_JITTER
TEMPERATURE_CHANGE_THRESHOLD
HEARTBEAT_INTERVAL
```

Follow the existing project's naming/style conventions where appropriate.

Keep these values easy to modify so short development/test intervals can be used without changing the reporting logic.

Do not unnecessarily hard-code production values into the logic.

### Deep-sleep considerations

Preserve whatever state must survive ESP32 deep sleep so that the node knows:

- the last temperature actually transmitted;
- how long it has been since the last transmitted report, or equivalent heartbeat state.

Use the project's existing RTC/deep-sleep state mechanism if one exists.

A routine deep-sleep wake must continue to avoid the five-second OLED startup/configuration behavior implemented separately for deliberate reset/power-up.

### Collision avoidance

The random jitter is required even when change-based reporting greatly reduces normal traffic.

Two or more nodes may experience similar environmental changes or heartbeat timing, so reporting optimization must not reintroduce deterministic synchronization.

Do not implement acknowledgments, carrier sensing, gateway scheduling, time slots, or retransmission protocols for this story. Independent randomized sampling is sufficient for this network.

### Constraints

Preserve:

- Existing LoRa frequency/modulation configuration.
- Existing sensor-report packet format.
- Existing temperature, humidity, and battery measurement mechanisms.
- Existing node-ID behavior.
- Existing deep-sleep operation.
- Existing startup OLED behavior.

Prefer a small, coherent change rather than restructuring unrelated firmware.

### Acceptance criteria

1. The sensor wakes and samples at approximately the configured sampling interval.
2. Every sleep interval receives a fresh random jitter component.
3. Jitter is applied whether or not the current cycle transmitted a packet.
4. A temperature change equal to or greater than the configured threshold causes a report.
5. Temperature change is measured relative to the **last transmitted temperature**.
6. Humidity change alone does not cause a report.
7. Battery-voltage change alone does not cause a report.
8. Every transmitted report continues to contain current temperature, humidity, and battery voltage.
9. A heartbeat report is transmitted when the configured maximum reporting interval expires even if temperature has remained stable.
10. An initial report is transmitted after startup.
11. State required for change detection and heartbeat behavior survives routine deep sleep.
12. Multiple nodes running the same firmware do not use identical fixed periodic transmission timing.
13. Existing node-ID, OLED, packet protocol, and deep-sleep behavior continue to work.
14. Build the firmware and resolve any compilation errors introduced by the change.

### Additional Acceptance Criteria

- A configured 0.1 °C threshold refers to change in the high-resolution SHT40-derived temperature, not change in the rounded/quantized transmitted value.
- Several small temperature changes must accumulate relative to the last **transmitted high-resolution measurement** until the configured threshold is reached.
- Packet encoding and display rounding must not affect the change-detection decision.


### Test behavior

For development, retain short enough sampling and heartbeat intervals that the behavior can be observed conveniently.

In particular, verify with at least two nodes that their wake/transmit timing no longer remains phase-locked. Their visible sensor-board activity LEDs should wander relative to one another rather than repeatedly flashing together.

Also verify that stable temperature eventually suppresses ordinary transmissions while heartbeat packets continue to arrive.

## Acceptance

The developer verified and accepted this story. Implemented high-resolution
change reporting, RTC-retained successful-report state, initial reporting,
heartbeat checks, and fresh hardware-random sampling jitter. Firmware build,
portable reporting-policy tests, and packet regression tests passed. Heartbeats
are evaluated at sampling boundaries, with up to one sampling cycle of latency;
failures can delay reporting further. No other intentional departures.
