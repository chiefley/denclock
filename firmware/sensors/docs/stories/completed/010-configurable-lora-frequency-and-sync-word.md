# Story 010: Configurable LoRa Frequency and Sync Word

**Project:** DenClock
**Component:** Heltec ESP32 LoRa Remote Sensor Firmware
**Type:** Firmware enhancement
**Status:** Accepted by the developer

## Background

DenClock will support multiple independent installations, each containing two LoRa sensors, a gateway, Windows services, and a display client.

During assembly and testing, several installations may operate simultaneously within radio range of each other. Future amateur radio applications will also use LoRa independently of DenClock.

To permit radio separation between installations, the sensor firmware must support configurable LoRa carrier frequency and sync word.

Story 009 introduced persistent sensor configuration through the USB serial console. This story extends that mechanism rather than introducing a new configuration system.

## User Story

As the DenClock developer, I want to configure each sensor's LoRa frequency and sync word through its existing USB serial console, so I can operate independent DenClock installations on different radio channels using the same firmware image.

## 1. Configurable Parameters

Add two parameters to the existing `SensorConfig` structure.

| Parameter      | Command                | Default    |
| -------------- | ---------------------- | ---------- |
| LoRa frequency | `set frequency 915500` | 915000 kHz |
| LoRa sync word | `set sync D3C7`        | 0xD3C7     |

Frequency is entered as an integer in kHz, eliminating floating-point parsing and providing 1 kHz configuration resolution.

Display frequency in MHz with three decimal places.

The sync word is entered as four hexadecimal digits representing the two SX1262 sync-word register bytes.

The existing defaults must preserve current radio behavior:

- Frequency: 915.000 MHz
- Sync-word registers: D3 C7
- Bandwidth: 125 kHz
- Spreading factor: 9
- Coding rate: 4/7
- CRC enabled
- Existing preamble and TCXO configuration

All other sensor configuration parameters remain unchanged.

## 2. Serial Console Extensions

Extend the existing command parser to recognize:

```text
set frequency 915500
set sync D3C7
```

Accept hexadecimal sync words case-insensitively.

Update `?` and `help` with descriptions, ranges, units, and examples.

Extend `show` to display:

```text
LoRa Frequency: 915.500 MHz
LoRa Sync:      0xD3C7
```

Preserve the existing saved/unsaved configuration indication.

The new parameters must follow the existing console semantics:

- `set` modifies the working configuration in RAM.
- `save` persists the complete configuration in NVS.
- `defaults` restores development defaults in RAM.
- `exit` discards unsaved modifications.
- `reboot` loads the saved configuration.
- The five-minute inactivity timeout discards unsaved modifications.

Radio settings take effect when the radio is next initialized, not immediately when entered into the console.

## 3. Parameter Validation

### Frequency

Accept integer frequency values from 902125 through 927875 kHz, inclusive, covering carrier centers within the US 902–928 MHz band with nominal room for the configured 125 kHz bandwidth.

Reject malformed values, fractions, negative values, overflow, and out-of-range values.

The allowed configuration range does not by itself establish FCC compliance. No automatic channel allocation or regulatory-compliance mechanism is required.

### Sync word

Accept exactly four hexadecimal digits, optionally preceded by `0x`.

Store the value as a 16-bit integer.

Preserve the existing relationship between the displayed SX1262 register value and RadioLib's `setSyncWord(syncWord, controlBits)` parameters.

For example:

```cpp
radio.setSyncWord(0xDC, 0x37);
```

must continue to produce register bytes `D3 C7`.

Use a single, tested conversion function. Do not confuse the register representation with RadioLib's two arguments.

Custom sync words are experimental: the SX1262 does not guarantee equivalent reception performance or perfect isolation for arbitrary values. Do not silently substitute another value.

If radio initialization rejects a configured value, retain the existing radio-error handling behavior.

## 4. Persistent Configuration

Extend `SensorConfig` with the two new parameters.

Increment its version to 2.

Increment the configuration version to 2. Existing version-1 NVS configurations must be rejected, and the sensor must initialize with version-2 factory defaults. Do not migrate or preserve old settings. The new defaults are not written to NVS until the operator explicitly executes `save`.

**No migration or backward-compatibility code is required.**

The developer will reconfigure existing sensors as necessary.

Preserve the existing behavior of performing NVS writes only when the user explicitly executes `save`.

## 5. Radio Initialization

Modify the existing shared radio initialization function in:

`firmware/sensors/common/radio_config.h`

to accept frequency and sync-word parameters.

Retain default arguments or equivalent behavior so existing gateway initialization remains unchanged.

Pass the configured values from the sensor's `main.cpp` to the radio initialization function.

Keep all other radio parameters unchanged.

The gateway firmware must not acquire new configuration behavior as part of this story.

Update serial startup diagnostics to display the actual configured frequency and sync word rather than hardcoded values.

## 6. Existing Behavior to Preserve

Do not change:

- Sensor packet format
- Node numbering
- Reporting thresholds
- Sampling intervals or jitter
- Heartbeat processing
- Battery measurements
- SHT40 measurements
- Sequence-number handling
- Deep-sleep behavior
- OLED startup diagnostics
- Existing serial-console behavior
- Gateway TCP transport
- LoRa Windows service
- MQTT processing
- DenClock client

The configuration-entry timing and deep-sleep optimizations established in Story 009 must remain intact.

## 7. Testing

Extend the existing portable tests to verify:

1. Factory-default frequency and sync word.
2. Valid frequency and hexadecimal sync-word commands.
3. Minimum and maximum frequency values.
4. Rejection of invalid values and malformed hexadecimal input.
5. Case-insensitive command recognition.
6. Correct RadioLib parameter conversion for `0xD3C7`.
7. Configuration saved/modified comparisons.
8. Version-1 configuration rejection and default fallback.
9. Version-2 persistence through the existing storage mechanism.
10. Preservation of all previously implemented console behavior.

Build the sensor firmware using the existing PlatformIO environment.

Run the existing portable sensor tests.

### Hardware verification

The developer will verify on Heltec hardware:

1. Default configuration still communicates with the existing gateway.
2. Changing frequency and saving persists through reset.
3. Changing sync word and saving persists through reset.
4. Sensor transmission succeeds when the gateway has matching settings.
5. A deliberately mismatched frequency or sync word prevents ordinary packet reception.
6. Normal sampling, transmission, and deep-sleep operation remain unchanged.

Hardware verification requiring gateway parameter changes may be completed after the separate gateway configuration story is implemented.

## 8. Documentation

Update:

- Sensor README
- Sensor configuration documentation
- Relevant radio configuration comments

Document the commands, units, valid ranges, defaults, persistence behavior, and requirement that the sensor and gateway radio settings match.

Clarify that sync words are radio synchronization parameters, not authentication credentials.

## 9. Deliverables

Implement changes in the existing sensor firmware and shared radio configuration code.

Add or update automated tests.

Build and run available tests.

Report:

- Files changed
- Build results
- Test results
- Hardware verification still required
- Any departures from the story

Do not modify unrelated application components.

**Do not mark this story completed or accepted until developer review.**

## Acceptance

The developer explicitly accepted this story. Configurable frequency and sync
word extend the existing serial console and version-2 NVS configuration while
preserving gateway defaults. All seven portable test suites and the sensor and
both gateway PlatformIO builds passed. Hardware verification is developer-owned;
no separate hardware results were recorded with acceptance. No intentional
departures from the story.
