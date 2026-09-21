# Story: Remote Sensor Deep-Sleep Operation

## Objective

Reduce remote-sensor power consumption by operating the sensor as a periodic deep-sleep device.

The remote sensor shall:

1. Wake from deep sleep.
2. Power and initialize the temperature/humidity sensor.
3. Initialize the LoRa radio.
4. Measure temperature, relative humidity, and battery voltage.
5. Transmit one `SENSOR_REPORT`.
6. Shut down the sensor and LoRa radio.
7. Place the ESP32 into deep sleep.
8. Wake approximately 5 seconds later and repeat.

The 5-second interval is intended for development and testing. A substantially longer reporting interval will be selected later for normal battery operation.

---

## Governing Documents

Implementation shall conform to the existing repository documentation, particularly:

- `LORA-PROTOCOL.md`
- existing remote-sensor documentation and source code

Do not modify the LoRa wire protocol for this story.

---

# 1. Existing State

The production remote sensor currently:

- reads the SHT40;
- measures battery voltage;
- constructs a valid `SENSOR_REPORT`;
- transmits it successfully over LoRa;
- interoperates with the production gateway and LoRa Service.

The existing end-to-end receive path is working and shall not be redesigned as part of this maintenance story.

The problem addressed by this story is excessive remote-sensor power consumption.

---

# 2. Operating Cycle

The production remote sensor shall change from continuously running operation to a wake-measure-transmit-sleep cycle.

Each cycle shall perform:

```text
ESP32 wakes
    |
initialize required hardware
    |
power/initialize SHT40
    |
initialize LoRa radio
    |
measure temperature/humidity
    |
measure battery voltage
    |
construct SENSOR_REPORT
    |
transmit SENSOR_REPORT
    |
ensure transmission is complete
    |
shut down SHT40
    |
shut down LoRa radio
    |
configure wake timer
    |
ESP32 deep sleep
```

Only one `SENSOR_REPORT` shall normally be transmitted per wake cycle.

---

# 3. Development Sleep Interval

For this story, configure the timer wake interval to approximately:

```text
5 seconds
```

The interval shall be represented by a clearly named constant or configuration value so that it can easily be changed later.

Do not design a generalized configuration system solely for this setting.

The eventual production reporting interval is outside the scope of this story.

---

# 4. ESP32 Deep Sleep

The ESP32 shall use deep sleep between reports rather than remaining awake in a delay loop.

Before entering deep sleep, the firmware shall configure a timer wake source for the next measurement cycle.

On timer wake, normal program initialization may execute again.

The implementation should treat each timer wake as another reporting cycle rather than requiring the processor to remain continuously active.

---

# 5. Sequence Number Across Deep Sleep

Deep sleep is normal remote-sensor operation and shall not be represented as a sensor restart in the LoRa protocol.

The existing `SENSOR_REPORT` sequence-number semantics remain authoritative:

- the first report after a true power-up/reset uses sequence `0`;
- subsequent reports use `1, 2, ... 65535`;
- rollover after `65535` goes to `1`, not `0`.

Therefore, the sequence number shall be preserved across normal timer-driven deep-sleep cycles using ESP32 RTC-retained state or an equivalent mechanism that survives deep sleep without requiring nonvolatile flash writes every reporting cycle.

Example normal operation:

```text
Power-up:
    sequence 0

Deep-sleep wake:
    sequence 1

Deep-sleep wake:
    sequence 2

Deep-sleep wake:
    sequence 3
```

A true loss of retained state caused by power removal or reset shall again result in the first transmitted report using sequence `0`.

Do not write the sequence number to flash/NVS on every report merely to preserve it across deep sleep.

---

# 6. SHT40 Power Management

The SHT40 shall not be left unnecessarily powered during the deep-sleep interval.

If the existing hardware provides firmware control of sensor power, use that control to remove power from the sensor before ESP32 deep sleep and restore it on wake.

If the existing hardware does **not** provide firmware-controlled sensor power, do not invent a hardware capability in software. In that case:

- place the SHT40 into the lowest-power state supported by the existing hardware/device behavior;
- ensure no unnecessary I²C activity occurs while preparing for sleep; and
- document any residual sensor power consumption or hardware limitation discovered during implementation.

No PCB modification is required by this story unless the existing hardware makes the stated power-down requirement impossible and the limitation must be reported for review.

---

# 7. LoRa Radio Power Management

After completion of the `SENSOR_REPORT` transmission, the SX1262 shall be placed into the lowest appropriate power state before ESP32 deep sleep.

The implementation shall ensure that transmission has completed before shutting down the radio.

The radio shall be reinitialized as required after each ESP32 wake.

Do not leave the radio in receive mode or another unnecessarily high-power state during the sleep interval.

---

# 8. ESP32 Peripheral State

Before deep sleep, disable or place into an appropriate low-power state any peripherals used by the remote sensor that would otherwise materially increase sleep current.

Do not add broad low-power optimization unrelated to the actual remote-sensor hardware.

This story is intended to establish the major power-saving behavior:

```text
sensor inactive
LoRa radio inactive
ESP32 deep sleep
```

Further microamp-level optimization may be performed later if measurements show it is necessary.

---

# 9. Preserve Existing Protocol Behavior

The transmitted `SENSOR_REPORT` shall remain wire-compatible with the existing gateway and LoRa Service.

Do not change:

- Message Class
- Node ID
- fixed header
- TLV definitions
- temperature representation
- humidity representation
- battery-voltage representation
- byte order
- radio configuration
- sync word

The gateway and LoRa Service shall require no changes for this story.

---

# 10. Diagnostics

Retain enough serial diagnostic output to make development testing practical while the ESP32 is awake.

Useful diagnostics include:

- wake/reset reason;
- sequence number;
- measured values;
- transmission success/failure;
- indication that the device is entering deep sleep.

Do not keep the ESP32 awake merely to preserve serial output.

Diagnostics must not defeat the power-saving behavior being implemented.

---

# 11. Explicitly Out of Scope

Do not implement as part of this story:

- MQTT changes
- Gateway changes
- LoRa Service changes
- DenClock changes
- Dynamic reporting intervals
- Remote configuration of the reporting interval
- Command reception while sleeping
- Over-the-air firmware update behavior
- Flash persistence of every sequence number
- Battery-management redesign
- Detailed battery-life prediction
- Additional sensors
- General-purpose power-management framework

---

# 12. Acceptance Criteria

The story is complete when the real remote-sensor hardware demonstrates the following behavior:

1. Power/reset the remote sensor.
2. The sensor initializes normally.
3. The first transmitted `SENSOR_REPORT` uses sequence number `0`.
4. The gateway and LoRa Service successfully receive and decode that report.
5. The sensor shuts down or places the SHT40 into the lowest state permitted by the existing hardware.
6. The SX1262 is placed into an appropriate low-power state.
7. The ESP32 enters deep sleep.
8. Approximately 5 seconds later, the ESP32 wakes automatically.
9. A new measurement is taken.
10. Exactly one new `SENSOR_REPORT` is normally transmitted for that wake cycle.
11. The second report uses sequence number `1`, not `0`.
12. Subsequent deep-sleep cycles produce sequence numbers `2`, `3`, and so forth.
13. The existing gateway and LoRa Service decode the reports without modification.
14. Removing power and restoring it causes the next first report to use sequence number `0` again.
15. The remote sensor spends the large majority of time between reports in ESP32 deep sleep rather than an active delay loop.

Where practical, current measurement may be used to confirm the expected reduction in power consumption, but precise sleep-current characterization is not required for acceptance of this story.

---

# 13. Definition of Done

This story is done when:

- periodic wake-measure-transmit-sleep operation works on the real hardware;
- the development interval is approximately 5 seconds;
- the ESP32 uses deep sleep;
- the LoRa radio is inactive during the sleep period;
- the SHT40 is powered down or placed into the lowest state possible with the existing hardware;
- sequence numbering remains correct across deep-sleep wakes;
- a true power cycle still produces sequence `0`;
- the existing Gateway and LoRa Service continue to receive and decode the reports unchanged;
- relevant automated tests and firmware builds pass;
- any discovered hardware limitation preventing complete sensor power removal is documented;
- changes are ready for review without unrelated redesign.

The resulting behavior should prove this statement:

> The remote sensor consumes significant power only while waking, measuring, and transmitting, and spends the interval between reports in deep sleep.

## Repository mapping and verification for review

- Implementation: `firmware/sensors/remote-sensor/src/main.cpp`.
- Authoritative wire reference: `docs/LoRa Protocol.md` (unchanged).
- Five seconds is sleep duration; initialization, measurement, and TX add to it.
- Existing wiring supplies SHT40 VIN from switchable Vext. GPIO holds preserve
  power-off and radio control states; permanent 3.3 V wiring or always-on I2C
  pull-ups would prevent complete sensor power removal and must be checked.
- Final PlatformIO remote-sensor build passes. ELF inspection places the two-byte
  sequence variable in `.rtc.data`.
- Existing portable sensor-report tests pass, covering wire encoding and sequence
  rollover. Documentation links and diff whitespace checks pass.
- Gateway, LoRa Service, shared wire codec/radio settings, and bringup firmware
  are unchanged. No hardware was flashed or measured during implementation.

## Human acceptance

The developer confirmed that the story is verified and accepted as complete.
The accepted increment provides timer-driven deep sleep with retained sequence
numbering and peripheral shutdown, preserving the existing receive path.
Precise sleep-current characterization was not required by this story; no
numerical current measurement is recorded here.
