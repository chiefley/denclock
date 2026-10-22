# Story 011: Persistent Serial Configuration for LoRa Gateway

**Project:** DenClock
**Component:** Heltec ESP32 LoRa Gateway Firmware
**Type:** Firmware enhancement
**Status:** Accepted by the developer

## Background

DenClock will support multiple independent installations, each with two LoRa sensors, a gateway, Windows services, and a display client.

Multiple installations must be able to operate simultaneously during assembly and testing. Radio separation will be accomplished primarily through configurable LoRa frequencies, with configurable sync words also supported.

The remote sensor firmware already implements persistent USB serial configuration, including frequency and sync word. The shared radio initialization code was updated as part of that work.

The gateway currently uses compile-time configuration for its LoRa service destination and fixed radio defaults. This story introduces persistent gateway configuration for three parameters:

- LoRa carrier frequency
- LoRa sync word
- LoRa service hostname

The intention is to use one gateway firmware image for all DenClock installations, with installation-specific parameters configured through the USB serial console.

## User Story

As the DenClock developer, I want to configure each gateway's LoRa frequency, sync word, and destination service hostname through its USB serial console, so I can provision independent DenClock installations without rebuilding firmware for each gateway.

## 1. Configurable Parameters

Add persistent configuration for the following parameters.

| Parameter | Command | Default |
|---|---|---|
| LoRa frequency | `set frequency 915500` | 915000 kHz |
| LoRa sync word | `set sync D3C7` | 0xD3C7 |
| Service hostname | `set host denclock-d3c7.local` | Existing configured service address |

Frequency is represented internally as integer kHz and displayed in MHz with three decimal places.

Sync word is represented as the 16-bit SX1262 register value, consistent with the updated sensor firmware.

The hostname identifies the Windows computer running the LoRa service. It may be a DNS hostname, an mDNS `.local` hostname, or an IPv4 address.

Preserve the existing TCP port setting of 5210 and the current gateway ID of 1.

Do not introduce additional configurable parameters.

## 2. Serial Console

Use the existing USB serial connection at 115200 baud.

Provide a bounded, line-oriented command processor using the same conventions as the sensor configuration console wherever practical.

Supported commands:

```text
?                   Display help
help                Display help
show                Display current configuration

set frequency N     Configure frequency in kHz
set sync HHHH       Configure hexadecimal sync word
set host NAME       Configure LoRa service hostname

save                Persist configuration to NVS
defaults            Restore defaults in RAM
reboot              Restart gateway
exit                Leave configuration mode
```

Commands are case-insensitive except for hostname values, which should retain their entered representation.

Accept CR, LF, or CRLF line endings.

Echo typed characters and support Backspace and Delete.

Display a `> ` prompt when accepting configuration commands.

Reject malformed commands, extra arguments, and oversized input without modifying configuration.

### Entering configuration mode

Because the gateway is continuously powered, its serial configuration console should be accessible without resetting the ESP32.

Use a deliberate `config` command to enter configuration mode.

The gateway should continue normal radio reception and TCP operation while configuration is being edited. New settings are staged in RAM and do not affect active connections or radio settings until a subsequent reboot.

Serial diagnostic output may continue while configuration mode is active.

### Leaving configuration mode

`exit` discards unsaved modifications and returns to normal operation.

After five minutes without a valid command, automatically exit configuration mode and discard unsaved modifications.

`save` persists settings but does not exit configuration mode.

`reboot` restarts the gateway without automatically saving pending changes.

The console must not introduce blocking waits into the normal gateway receive loop.

## 3. Configuration Display

The `show` command should display all three parameters and the saved/unsaved status.

Example:

```text
DenClock Gateway Configuration

LoRa Frequency: 915.500 MHz
LoRa Sync:      0xD3C7
Service Host:   denclock-d3c7.local
Service Port:   5210

Configuration: Saved
```

The service port is displayed for reference but is not configurable in this story.

Update the help text with permitted ranges, units, and representative examples.

## 4. Validation

### Frequency

Use the same validation as the updated sensor firmware.

Accept integer values from 902125 through 927875 kHz, inclusive.

Reject fractions, negative values, malformed input, overflows, and out-of-range values.

The frequency range does not itself establish FCC regulatory compliance.

### Sync word

Use the same representation and validation rules as the updated sensor firmware.

Accept exactly four hexadecimal digits, optionally preceded by `0x`.

Examples:

```text
set sync D3C7
set sync 0xD3C7
```

Preserve the established RadioLib conversion from SX1262 register bytes to `syncWord` and `controlBits`.

Reuse the existing shared conversion function rather than creating an independent implementation.

### Hostname

Accept valid DNS hostnames, `.local` mDNS names, and IPv4 address literals.

Reject empty values, embedded whitespace, unsupported characters, malformed IPv4 literals, and excessively long names.

Use a fixed maximum length consistent with normal DNS hostname limits.

The configured destination must be stored safely, with no buffer overflow or truncation.

## 5. Persistent Storage

Use ESP32 Arduino `Preferences` and NVS, following the established sensor configuration approach.

Requirements:

1. Load validated configuration during startup.
2. Use development defaults if no valid stored configuration exists.
3. Modify working configuration only when executing `set` or `defaults`.
4. Write NVS only when `save` is executed.
5. Reject invalid stored configurations and revert to defaults.
6. Track saved versus modified configuration.
7. Retain settings through power cycles and normal firmware uploads that preserve NVS.

Use a versioned configuration structure.

No migration or backward-compatibility mechanism is required.

An existing gateway configuration may be invalidated during development without preserving its previous values.

Use a separate gateway NVS namespace, not the sensor namespace.

## 6. Radio Initialization

Use the existing shared radio initialization code in:

`firmware/sensors/common/radio_config.h`

Pass the configured frequency and sync word to `initializeRadio()`.

Preserve the gateway's existing transmit power and other radio settings.

Do not modify the shared radio parameter encoding unless a demonstrable defect requires correction.

On startup, report the actual configured frequency and sync word rather than hardcoded values.

A failed radio initialization must retain the existing error reporting behavior.

## 7. TCP Connection Configuration

Replace the gateway's compile-time service destination with the effective configured hostname.

The existing gateway-initiated TCP connection architecture must remain unchanged.

Requirements:

- Resolve the configured hostname to the appropriate IP address.
- Support ordinary DNS hostnames and IPv4 literals.
- Support `.local` hostname resolution through mDNS when the corresponding service is available.
- Preserve the existing TCP connection and retry behavior.
- Recover from temporary hostname-resolution failures without requiring a restart.
- Do not assume a fixed IP address.
- Do not cache a resolved IP address permanently across reconnect attempts.

The existing gateway protocol remains unchanged.

In particular, retain the one-byte gateway ID and current HELLO message format.

No LoRa service modifications are included in this story.

The Windows-side mDNS advertisement and gateway identity validation will be implemented separately. Until then, a configured `.local` hostname may remain unresolved.

## 8. Wi-Fi Configuration

Preserve the existing gateway Wi-Fi behavior.

The current compile-time SSID and password configuration may remain in place temporarily.

Do not add Wi-Fi credential commands to the serial console.

Do not implement a temporary access point, captive portal, web server, or browser-based Wi-Fi provisioning in this story.

Those features belong to a subsequent story intended for end-user configuration.

## 9. Existing Behavior to Preserve

Do not change:

- LoRa packet format
- Gateway TCP framing and protocol
- Gateway HELLO message
- Gateway ID
- Sensor packet forwarding
- OLED diagnostics
- RSSI/SNR handling
- RX and ERR counters
- Existing error handling
- TCP reconnection behavior
- Wi-Fi reconnection behavior
- LoRa Windows service
- MQTT processing
- DenClock display client
- Remote sensor firmware

The gateway must remain an application-agnostic LoRa-to-TCP transport.

Do not introduce application-level sensor interpretation, sensor registration, network identifiers, or new packet fields.

## 10. Testing

Add automated tests for:

1. Factory-default configuration.
2. Valid frequency commands.
3. Frequency boundary values.
4. Invalid frequency rejection.
5. Valid hexadecimal sync-word commands.
6. Malformed sync-word rejection.
7. Correct conversion to RadioLib parameters.
8. Valid hostname and IPv4 configuration.
9. Invalid hostname rejection.
10. Saved versus modified status.
11. NVS persistence and version validation.
12. Unsaved configuration discarded by `exit`.
13. Unsaved configuration discarded by inactivity timeout.
14. Reboot without implicit save.
15. Serial line editing, echo, CR/LF handling, and bounded input.
16. Preservation of normal gateway operation while configuration is being edited.
17. Connection retry behavior following hostname-resolution failure.

Run existing gateway tests and build both the production gateway and diagnostic environments using PlatformIO.

### Hardware verification

The developer will verify:

1. Default gateway configuration communicates with existing sensors and LoRa service.
2. Serial configuration is accessible during operation.
3. Modified settings persist after `save` and reboot.
4. Unsaved modifications are discarded.
5. A sensor and gateway configured with matching radio parameters communicate successfully.
6. Mismatched frequency or sync word prevents ordinary reception.
7. Valid hostname resolution permits TCP connection.
8. Unresolved hostname produces appropriate diagnostics and retries.
9. OLED diagnostics and packet forwarding remain operational.

Testing of `.local` resolution against the final Windows service advertisement may be deferred until that service feature is implemented.

## 11. Documentation

Update the gateway documentation to explain:

- Entering and leaving serial configuration
- Supported commands
- Frequency and sync-word representation
- Hostname configuration
- NVS persistence
- Configuration defaults
- Saved versus unsaved settings
- Radio parameter matching between sensors and gateway
- Hostname-resolution and TCP reconnection behavior

Document that sync words are radio synchronization parameters, not authentication credentials.

## 12. Development Workflow and Deliverables

Inspect the repository's existing conventions before making changes.

Determine the appropriate story directory and next available story number. Do not assume a number or location from this document.

Implement the enhancement within the existing gateway firmware architecture.

Reuse existing configuration and radio utilities where practical without introducing unnecessary abstractions.

Provide:

- Source changes
- Automated tests
- Updated documentation
- PlatformIO build results
- Test results
- Remaining hardware verification steps
- Any deviations from requirements and their justification

Do not modify unrelated components.

**Do not mark the story completed or accepted until developer review.**

## Acceptance

The developer explicitly accepted this story. The production gateway loads a
versioned NVS configuration of frequency, sync word and service host, edited
through a `config` serial console that stages changes in RAM without
interrupting reception or forwarding; saved settings apply after reboot. Each
TCP attempt re-resolves the host. All ten portable test suites and both gateway
PlatformIO environments passed. Hardware verification is developer-owned; no
separate hardware results were recorded with acceptance.

Intentional departures: frequency/sync parsing is duplicated from the sensor
rather than shared, because sensor firmware was out of scope; `show` adds a
`(reboot to apply)` note; the `diagnostic` environment keeps radio defaults;
host-test fakes gained `IPAddress` and namespace recording.
