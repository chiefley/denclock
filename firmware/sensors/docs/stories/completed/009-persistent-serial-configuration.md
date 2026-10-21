# 009: Persistent Serial Configuration for LoRa Sensor Nodes

**Project:** DenClock
**Component:** Heltec ESP32 LoRa Sensor Firmware
**Type:** Firmware enhancement
**Status:** Accepted

## Background

DenClock uses Heltec WiFi LoRa 32 V3 boards with SX1262 radios and SHT40 temperature/humidity sensors.

The den sensor requires frequent temperature measurements because the fireplace is used to heat the house. The remote sensors can operate with longer reporting intervals to conserve battery power and minimize unnecessary LoRa transmissions.

Rather than maintaining separate firmware builds, all sensor nodes should run the same firmware image, with individual configuration parameters entered through the USB serial console and retained in ESP32 nonvolatile storage.

## User Story

As the DenClock operator, I want to configure each sensor's identity, reporting intervals, temperature threshold, and LoRa transmit power through a simple serial console, so I can deploy the same firmware to every sensor without recompilation.

## Functional Requirements

### 1. Configurable Parameters

Provide the following parameters:

| Parameter             | Command           | Factory Default | Units        |
| --------------------- | ----------------- | --------------- | ------------ |
| Node ID               | `set node N`      | 2               | Integer      |
| Sample interval       | `set sample N`    | 5               | Seconds      |
| Sample jitter         | `set jitter N`    | 1               | ± seconds    |
| Heartbeat interval    | `set heartbeat N` | 30              | Seconds      |
| Temperature threshold | `set threshold N` | 1               | Tenths of °C |
| LoRa transmit power   | `set power N`     | 10              | dBm          |

**Factory defaults must exactly preserve the current development firmware behavior.**

The threshold command uses integer tenths of a degree Celsius:

- `set threshold 1` means 0.1°C.
- `set threshold 2` means 0.2°C.
- `set threshold 5` means 0.5°C.

Continue evaluating temperature changes against the unrounded, high-resolution SHT40 temperature measurement.

The following radio parameters remain fixed in firmware: 915 MHz, 125 kHz bandwidth, SF9, coding rate 4/7, existing sync word, CRC, preamble, and TCXO settings.

### 2. Serial Console

Use the existing USB serial connection at 115200 baud.

Implement a line-oriented command processor that accepts CR, LF, or CRLF line endings. Commands are case-insensitive and tolerate extra whitespace.

Support these commands:

```text
?                 Display command reference
help              Same as ?
show              Display current configuration

set node N        Set node ID
set sample N      Set sample interval in seconds
set jitter N      Set sample jitter in seconds
set heartbeat N   Set heartbeat interval in seconds
set threshold N   Set temperature threshold in 0.1 C units
set power N       Set LoRa TX power in dBm

save              Save configuration to flash
defaults          Restore development defaults in RAM
reboot            Restart the sensor
exit              Resume normal sensor operation
```

Entering `?` must display the complete command reference, including parameter units, permitted ranges, and representative examples.

On entry to configuration mode, display:

```text
DenClock Sensor Configuration
Enter ? for help.
```

The `show` command should display all current parameters, including threshold in both forms:

```text
Node ID:       2
Sample:        5 seconds
Jitter:        +/- 1 second
Heartbeat:     30 seconds
Threshold:     1 (0.1 C)
TX Power:      10 dBm

Configuration: Saved
```

When there are unsaved changes, display `Configuration: Modified (unsaved)`.

Invalid commands should produce a useful error message and suggest `?` for help.

### 3. Configuration Storage

Use the ESP32 Arduino `Preferences` library and NVS flash storage.

Requirements:

1. Load saved configuration at startup.
2. If no saved configuration exists, use the factory development defaults.
3. `set` commands modify the working configuration in RAM only.
4. `save` commits the entire validated working configuration to NVS.
5. `defaults` restores factory defaults in RAM without writing flash.
6. `exit` discards unsaved changes and restores the last saved configuration.
7. `reboot` must not automatically save unsaved changes.
8. Normal measurement and reporting cycles must never write NVS.

Include a configuration version number and validate persisted values before use. Invalid or incompatible stored configuration must fall back safely to defaults.

Settings must survive power cycling, deep sleep, and ordinary firmware updates that do not erase NVS.

### 4. Entering Configuration Mode

The existing five-second OLED startup display and the ten-second serial configuration entry window must run concurrently, not sequentially. The entire configuration-entry period must be ten seconds from deliberate reset or power-up, including the five-second OLED display. Do not introduce an additional startup delay. Ordinary deep-sleep timer wake-ups must bypass both mechanisms.

Preserve that behavior.

After a deliberate power-up or manual reset:

1. Load the configuration from NVS.
2. Preserve the existing five-second OLED startup diagnostics displaying node ID and battery voltage.
3. Provide a ten-second serial configuration entry window.
4. If a recognized configuration command arrives during this window, enter configuration mode.
5. If no command arrives, continue normal sensor operation.

Integrate the OLED diagnostics with the entry window to avoid unnecessary consecutive delays.

Timer-driven deep-sleep wake-ups must bypass configuration entry entirely and proceed directly to normal sampling/reporting.

The serial terminal should be usable through the existing PlatformIO serial monitor.

### 5. Leaving Configuration Mode

Two mechanisms terminate configuration mode.

**Explicit exit**

The `exit` command:

- Discards unsaved modifications.
- Restores the last saved configuration or factory defaults.
- Leaves configuration mode immediately.
- Resumes normal sensor operation.

**Inactivity timeout**

After five minutes without a valid command:

- Display a timeout message.
- Discard unsaved changes.
- Restore the saved configuration.
- Resume normal sensor operation.

Reset the inactivity timer after each valid command.

The `save` command must not terminate configuration mode. This allows the operator to execute `show` and verify the saved configuration before exiting.

The `reboot` command restarts the ESP32 and subsequently loads the saved configuration.

**No LoRa transmissions are permitted while configuration mode is active.**

### 6. Validation

Validate every command before modifying the working configuration.

Requirements:

- Node ID must be 1–239, consistent with the existing protocol.
- Sample interval must be strictly greater than jitter.
- Jitter must be nonnegative.
- Heartbeat must be positive.
- Threshold must be a positive integer in tenths of degrees Celsius.
- Transmit power must be within the supported SX1262/RadioLib and Heltec board limits.
- Reject numeric overflows and unsupported values.
- Reject configurations that would cause overflow in seconds-to-microseconds conversions or jitter calculations.
- Reject incomplete commands and additional unexpected arguments.
- Invalid commands must leave the configuration unchanged.

Define sensible upper bounds and include permitted ranges in the `?` help.

Avoid unbounded serial input buffers.

### 7. Integration With Existing Firmware

Refactor the existing firmware so that the configurable values replace the corresponding compile-time constants.

Use the configured node ID in the existing packet encoder.

Pass the configured transmit power to radio initialization. Preserve the gateway's existing fixed radio configuration and behavior.

Use the configured sample interval, jitter, heartbeat, and threshold in the existing reporting policy.

Preserve:

- Randomized sampling using fresh hardware entropy.
- Deep-sleep power management.
- RTC-retained reporting state.
- Temperature-change reporting relative to the last transmitted high-resolution measurement.
- Heartbeat reporting.
- Existing battery and humidity reporting.
- Existing OLED diagnostics.
- Sensor packet format.
- Sequence-number behavior.
- Existing error recovery.

After a deliberate reset or configuration session, the first valid measurement should generate an initial report using the effective node ID and configuration.

No modifications to the gateway, LoRa Service, MQTT, or DenClock display applications should be necessary.

### 8. Intended Operating Configurations

The following are deployment examples, **not factory defaults**.

| Parameter       | Den Sensor                 | Remote Sensors   |
| --------------- | -------------------------- | ---------------- |
| Sample interval | 5 seconds                  | 15 seconds       |
| Sample jitter   | ±1 second                  | ±2 seconds       |
| Heartbeat       | 30 seconds                 | 300 seconds      |
| Threshold       | 1 (0.1°C)                  | 1 (0.1°C)        |
| TX power        | Reduced after link testing | 10 dBm initially |

The operator will assign a unique node ID and appropriate transmit power to each sensor.

Do not introduce predefined den/remote profiles. Individual parameter configuration is sufficient.

## Acceptance Criteria

- [ ] One firmware build supports every sensor node.
- [ ] Factory defaults match the existing development firmware exactly.
- [ ] Configuration is available through the USB serial console following deliberate reset or power-up.
- [ ] `?` displays all commands, units, ranges, and examples.
- [ ] `show` displays effective settings and unsaved status.
- [ ] `set` changes working values without writing flash.
- [ ] `save` persists settings in NVS and remains in configuration mode.
- [ ] `defaults` restores factory values in RAM without saving.
- [ ] `exit` discards unsaved changes and resumes sensor operation.
- [ ] Five-minute inactivity timeout exits automatically.
- [ ] `reboot` does not persist unsaved changes.
- [ ] Saved settings survive power cycling and deep sleep.
- [ ] Normal timer wake-ups do not enter configuration mode or add startup delays.
- [ ] No LoRa packets are transmitted during configuration.
- [ ] Invalid commands are rejected without modifying settings.
- [ ] Configured transmit power is applied to the SX1262.
- [ ] Configured node ID appears in transmitted packets.
- [ ] Temperature threshold continues to use unrounded measurements.
- [ ] Existing gateway communication and packet formats are unchanged.
- [ ] Existing automated tests pass.
- [ ] Firmware builds successfully under the existing PlatformIO environment.
- [ ] On deliberate reset, the five-second OLED display and ten-second serial configuration window overlap, with no additional startup delay.


## Testing

Add appropriate tests for configuration validation, serial command parsing, persistence behavior, and reporting-policy integration.

Verify manually on Heltec hardware:

1. Factory-default startup.
2. Entry into configuration mode using `?`.
3. Modification and display of each parameter.
4. NVS save and subsequent reboot.
5. Unsaved changes discarded on exit.
6. Unsaved changes discarded on timeout.
7. Restoration of defaults.
8. Invalid configuration rejection.
9. Correct node ID and transmit power during operation.
10. Normal sampling and heartbeat behavior with custom intervals.
11. Deep-sleep timer wake-ups without configuration delays.
12. Continued gateway reception without gateway firmware changes.

Verify that two sensors can be configured differently while running the same firmware image.

## Deliverables

Implement the enhancement directly within the existing sensor firmware project.

Update the sensor README and contract documentation to describe configuration entry, commands, parameter ranges, persistence, and timeout behavior.

Build using PlatformIO, run existing and new tests, and report modified files and verification results.

Do not modify unrelated components or mark the story complete until developer acceptance.

**Scope:** A self-contained sensor firmware enhancement. No architectural changes to the gateway, server, display, or LoRa packet protocol.

## Developer review refinement

Echo serial input so commands are visible while typing, and support Backspace
and Delete for correcting the current line.

Show a `> ` prompt during the startup command-entry window and after each
command or invalid input while accepting commands. Do not print another prompt
after exit, timeout, or reboot; CRLF must produce only one prompt.

## Acceptance

The developer accepted this story on 2026-10-21, including input echo, line
editing, and the command prompt refinements. Sensor identity, reporting policy,
and transmit power are configurable through the bounded serial console and
persist only on explicit save. Startup entry overlaps OLED diagnostics; timer
wakes bypass both. The sensor PlatformIO build and all six portable firmware
test suites passed. Hardware operation was reviewed and accepted by the developer;
Codex did not independently verify the hardware scenarios. No intentional
departures from the refined requirements were recorded.
