# Suppress Gateway Serial Diagnostics During Configuration

**Project:** DenClock
**Component:** Heltec ESP32 LoRa Gateway Firmware
**Type:** Firmware enhancement
**Status:** Accepted by the developer

## Background

The DenClock LoRa gateway provides an interactive configuration console over its USB serial connection at 115200 baud.

During normal operation, the gateway continuously reports packet reception, forwarding activity, connection status, and errors through the same serial interface.

When the developer enters configuration mode using the `config` command, these unsolicited diagnostic messages continue appearing, making configuration commands difficult to enter and read.

The existing gateway configuration story explicitly permitted this behavior. Experience using the physical gateway has demonstrated that it is undesirable.

## User Story

As the DenClock developer, I want unsolicited serial diagnostic output suppressed while the gateway is in configuration mode, so I can interact with the configuration console without interference from normal gateway activity.

The gateway must continue performing all normal radio and network functions during configuration.

## 1. Required Behavior

### Normal operating mode

Preserve existing serial diagnostic behavior.

Packet reception, forwarding results, connection events, and errors should continue to be reported as before.

### Entering configuration mode

When the `config` command successfully enters configuration mode:

- Suppress unsolicited serial diagnostic output.
- Continue displaying configuration prompts, command echoes, responses, help, and errors.
- Do not interrupt gateway operation.
- Do not change or discard active configuration settings.

### While configuration mode is active

Only configuration-console output should appear on the serial connection.

The gateway must continue:

- Receiving LoRa packets.
- Forwarding valid radio packets to the LoRa service over TCP.
- Maintaining existing Wi-Fi and TCP connections.
- Performing normal connection retries.
- Updating OLED diagnostics and counters.
- Tracking reception and forwarding errors.

Diagnostic events must not interfere with an incomplete command line or the `> ` prompt.

### Leaving configuration mode

Restore normal serial diagnostic output when configuration mode ends through:

- The `exit` command.
- The existing five-minute inactivity timeout.

Do not require a reboot to restore diagnostic output.

The existing `save`, `defaults`, and `reboot` command behavior must remain unchanged.

## 2. Implementation Constraints

- Use the existing configuration-console state to determine whether diagnostic output should be suppressed.
- Prefer a small, centralized mechanism for distinguishing unsolicited diagnostics from configuration-console output.
- Avoid scattering duplicated configuration-mode checks throughout the firmware where practical.
- Do not introduce additional tasks, threads, services, timers, or diagnostic buffers.
- Do not accumulate suppressed messages for later replay.
- Do not change gateway packet handling or network protocols.
- Do not change the sensor firmware or Windows LoRa service.
- Do not introduce additional persistent settings.

If existing logging statements need to be routed through a small diagnostic-output abstraction, keep it simple and specific to this requirement.

The intent is to **discard only unsolicited serial output while configuration mode is active**, not defer processing or reporting internally.

## 3. Error Handling and Initial Provisioning

During initial gateway configuration, Wi-Fi or TCP connectivity may be unavailable or incorrectly configured. Repeated connection failures, hostname-resolution errors, and packet-forwarding failures are expected under these circumstances.

While configuration mode is active:

- Suppress all unsolicited serial diagnostics, including warnings, connection failures, packet errors, and retry messages.
- Preserve normal error detection, counters, state transitions, retry mechanisms, and recovery behavior.
- Continue displaying configuration command responses, validation errors, and save failures.
- Do not buffer suppressed diagnostics for later replay.
- Do not allow diagnostic output to interrupt command entry.

The OLED should continue presenting its existing connection status and diagnostic counters.

On leaving configuration mode, restore normal serial diagnostic output. Subsequent failures should appear normally; previously suppressed messages should not be replayed.

If the gateway encounters an unrecoverable failure, preserve its existing failure-handling behavior. Do not introduce additional recovery logic as part of this story.

The distinction is between suppressing serial output and suppressing error processing. Only serial output is affected.

## 4. Testing

Provide automated tests where practical to verify:

1. Normal diagnostic output is enabled outside configuration mode.
2. Entering configuration mode suppresses unsolicited diagnostics.
3. Configuration prompts, command echo, responses, and errors remain visible.
4. The `exit` command restores diagnostic output.
5. Configuration timeout restores diagnostic output.
6. Suppressed messages are not buffered or replayed.
7. Serial configuration continues to use bounded, nonblocking processing.
8. Gateway packet forwarding and connection management are unaffected.

Run the existing gateway tests and PlatformIO builds.

## 5. Hardware Acceptance Tests

The developer will verify on the physical Heltec gateway:

- Normal packet diagnostic output appears before entering configuration mode.
- Entering `config` produces a clean, usable command prompt.
- Incoming LoRa traffic does not disrupt command entry.
- `show`, `help`, and configuration commands operate normally.
- Gateway packet forwarding continues while the console is active.
- OLED reception counters continue updating.
- Exiting configuration mode restores normal packet diagnostics.
- Configuration timeout restores normal output.

A successful firmware build alone does not establish hardware acceptance.

## 6. Documentation

Update the existing gateway serial-configuration documentation to describe the new output behavior.

Explicitly supersede the earlier requirement allowing serial diagnostics during configuration mode.

Preserve the original completed story as a historical record.

Avoid duplicating documentation or modifying unrelated architectural descriptions.

## 7. Scope and Workflow

Before implementation, inspect the current gateway console, diagnostic output, main loop, and relevant tests.

Implement the smallest change that meets the requirements.

Preserve existing architectural conventions and avoid unrelated refactoring.

Report:

- Files changed.
- Implementation approach and rationale.
- Build and test results.
- Hardware verification still required.
- Any deviations from the story.

Do not modify unrelated components.

**Do not mark this story completed or accepted until developer review.**

## Acceptance

Accepted by the developer following physical-gateway verification.

Unsolicited serial diagnostics are discarded through a centralized gate tied to
the existing console state. Console output remains visible; exit and inactivity
timeout restore subsequent diagnostics without replay. Radio/network processing,
retries, error handling and OLED diagnostics continue.

All seven gateway host tests and both production and diagnostic PlatformIO
builds passed. Hardware acceptance is provided by the developer. No intentional
departures from the requested scope.
