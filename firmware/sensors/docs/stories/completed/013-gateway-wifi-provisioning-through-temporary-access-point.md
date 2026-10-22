# Gateway Wi-Fi Provisioning Through Temporary Access Point

**Project:** DenClock
**Component:** Heltec ESP32 LoRa Gateway Firmware
**Type:** Firmware enhancement
**Status:** Accepted by the developer

## Background

DenClock is intended to support multiple independent household installations using identical gateway firmware.

The gateway currently obtains its Wi-Fi SSID and password through compile-time definitions in `config.h` or `config.local.h`. This is suitable for development but not for distributing turnkey DenClock systems.

The gateway already has persistent serial configuration for its LoRa frequency, sync word, and destination service hostname. That mechanism should remain unchanged.

This story adds a consumer-friendly Wi-Fi setup process using the ESP32's built-in access-point and station capabilities.

The gateway should require no companion mobile application, cloud service, or internet connection for provisioning.

## User Story

As a DenClock recipient, I want to power on the gateway, connect to its temporary Wi-Fi network using my phone, and enter my household Wi-Fi credentials through a browser, so the gateway can join my network without requiring technical knowledge or a USB connection.

## 1. Normal Startup Behavior

On power-up:

1. Initialize the existing gateway firmware and load the saved configuration.
2. Check whether valid household Wi-Fi credentials have been saved.
3. If credentials exist, attempt to join the household Wi-Fi network.
4. If no credentials exist, start Wi-Fi provisioning mode.
5. If repeated connection attempts fail for a reasonable bounded period, make provisioning mode available for recovery.

A gateway that successfully joins its configured household Wi-Fi network should operate normally without displaying a setup page or leaving its temporary access point enabled.

Preserve the existing LoRa reception, TCP connection, and packet-forwarding architecture.

Provisioning must not require a Windows service to be running.

## 2. Temporary Wi-Fi Access Point

When provisioning is required, the ESP32 starts its own Wi-Fi access point.

Use a recognizable SSID with a device-specific suffix, for example:

`DenClock-Setup-A42F`

Derive the suffix from an appropriate stable device identifier, such as the ESP32 MAC address.

The SSID must distinguish multiple gateways undergoing provisioning in the same location.

Protect the setup access point with WPA2-Personal and a device-specific setup password.

Generate and persist a device-specific random setup password. Per developer refinement, use eight characters containing only lowercase letters and digits (the WPA2 minimum length). Replace any earlier long setup password with this format without changing household credentials. Do not use a universal password shared by all gateways.

Display the setup SSID and password on the Heltec OLED while provisioning is active.

Use the ESP32's local DHCP server to assign addresses to connected phones or computers.

The initial setup interface should be reachable at:

`http://192.168.4.1`

No internet access is required.

## 3. Browser-Based Configuration

Run a small HTTP server on the ESP32 while provisioning is active.

Serve a mobile-friendly page containing:

- DenClock Gateway identification
- Available nearby Wi-Fi networks, including SSID and signal strength
- A way to select a discovered network
- A way to enter an SSID manually, including hidden networks
- Household Wi-Fi password field
- Connect button
- Connection progress and result messages

Do not require JavaScript frameworks, external fonts, CDNs, or internet resources.

The page should work with ordinary current phone and desktop browsers.

Never echo the household Wi-Fi password back to the browser.

Do not include credentials in serial diagnostic messages, URLs, or HTTP GET parameters.

Validate submitted values, including length and memory bounds, before attempting a connection.

Handle SSIDs as Wi-Fi identifiers rather than assuming every SSID is a simple DNS-style name.

## 4. Captive Portal

Implement a lightweight captive-portal experience.

While the temporary access point is active, respond to local DNS requests in a way that directs ordinary captive-portal discovery attempts toward the setup page.

Support common operating-system captive-portal detection behavior where practical.

Automatic display of the setup page is not guaranteed across all phones or operating systems.

**The manual address `http://192.168.4.1` must always remain a supported fallback.**

Do not depend on internet connectivity to make the portal work.

## 5. Testing Household Wi-Fi Credentials

Do not overwrite previously working credentials merely because a user submits new values.

When the user selects Connect:

1. Validate the submitted SSID and password.
2. Keep the temporary access point and configuration page available.
3. Attempt to connect to the selected household Wi-Fi network in station mode.
4. Report connection progress to the browser.
5. If the connection succeeds and the gateway receives a usable IP address, save the new credentials to NVS.
6. Confirm success and display the assigned household-network IP address.
7. Stop the temporary access point after the success response can be delivered.

If the connection fails:

- Report a useful, non-sensitive error.
- Keep provisioning available.
- Allow the user to correct the credentials and try again.
- Preserve any previously saved working credentials.
- Do not enter an endless blocking connection loop.

The ESP32 supports simultaneous AP and station operation, but station connection can cause access-point channel changes. Design the process so that a phone temporarily losing its setup connection can reconnect and continue.

Avoid blocking the LoRa receive loop for prolonged intervals.

## 6. Persistent Wi-Fi Configuration

Store household Wi-Fi credentials in ESP32 NVS using `Preferences` or another existing platform-supported persistent mechanism.

Use a separate namespace from the gateway's LoRa/hostname configuration.

Requirements:

- Saved credentials survive power cycles and ordinary firmware updates.
- Credential changes are committed only after successful connection verification.
- Missing or invalid credentials cause provisioning mode to be offered.
- Existing LoRa frequency, sync word, service hostname, and their NVS data remain unchanged.
- Factory reset of Wi-Fi credentials must not erase radio configuration.

Remove dependence on embedded household-specific Wi-Fi credentials for normal production operation.

The same production firmware image must work in different households without recompilation.

Do not retain real household passwords as default values in tracked source files.

No migration or compatibility framework for development-only compile-time Wi-Fi settings is required.

## 7. Reentering Provisioning Mode

Provide a simple physical recovery mechanism.

Use an appropriate existing Heltec button, if supported safely by the board, to request provisioning during startup.

The intended user experience is:

1. Disconnect gateway power.
2. Hold the designated button.
3. Reconnect power.
4. Release the button after the gateway indicates provisioning mode.

Verify the selected button and GPIO behavior against the Heltec board and ESP32 boot-strapping requirements. Do not implement a button combination that prevents the ESP32 from booting.

If the board's existing buttons cannot reliably support this interaction, document and implement a comparably simple recovery mechanism using available hardware.

Entering provisioning mode manually must not immediately erase saved credentials.

A new successful Wi-Fi configuration replaces the old credentials.

Provide a deliberate means of clearing stored Wi-Fi credentials when needed, without erasing the independent LoRa gateway configuration.

## 8. OLED Diagnostics

Extend existing OLED behavior to show provisioning information.

While provisioning is active, display:

- A clear SETUP indication
- Temporary setup SSID
- Temporary setup password
- Browser address `192.168.4.1`

The OLED is only 128 × 64 pixels, so use multiple screens, timed rotation, or another simple readable arrangement where necessary.

Do not permanently compromise existing gateway diagnostics for normal operation.

When provisioning completes, restore the current gateway diagnostic display.

Do not display the household Wi-Fi password on the OLED.

## 9. Preserve Normal Gateway Operation

When the ESP32 has joined household Wi-Fi, continue using the existing gateway connection logic.

The gateway resolves the configured LoRa service hostname and initiates TCP connections on the existing service port.

Do not change:

- LoRa frequency or sync-word configuration
- Existing serial configuration commands
- Bounded serial-console processing
- Gateway ID
- HELLO message
- TCP framing or packet formats
- LoRa packet reception and forwarding
- Existing service hostname resolution
- RSSI/SNR diagnostics
- RX and ERR counters
- Remote sensor firmware
- Windows services
- MQTT processing
- DenClock display client

The gateway must remain a LoRa-to-TCP transport.

Radio reception and diagnostics should remain active during provisioning where practical. Packets cannot be forwarded to the Windows service until a valid network connection exists.

Do not queue an unbounded backlog of packets awaiting connection.

## 10. Resource and Reliability Requirements

The gateway is an embedded device with limited memory.

Use bounded request handling, input sizes, and network operations.

Do not introduce unnecessary external services, application frameworks, or permanent background web interfaces.

The provisioning HTTP and DNS servers should run only when provisioning is active.

Normal operation must not continuously broadcast a setup network.

Maintain reasonable responsiveness of the main gateway loop during Wi-Fi setup and retries.

Handle repeated connection failures, unexpected disconnects, invalid requests, and browser refreshes without requiring firmware reinstallation.

## 11. Automated Testing

Add tests covering:

1. No saved credentials causes provisioning mode.
2. Valid saved credentials cause normal station connection attempts.
3. Failed saved-credential attempts permit recovery.
4. Successful new credentials are persisted.
5. Failed new credentials do not replace saved credentials.
6. Manual provisioning entry does not erase credentials.
7. Wi-Fi credential clearing leaves LoRa configuration intact.
8. Unique setup SSID generation.
9. Setup password generation and persistence.
10. Input validation and bounded HTTP request handling.
11. Provisioning state transitions.
12. Timeout and retry behavior.
13. Disabling the setup AP after successful provisioning.
14. Existing serial-console and radio-forwarding behavior.
15. Gateway reconnection and hostname resolution after Wi-Fi provisioning.

Use host-side fakes where practical for NVS, Wi-Fi state, and web requests.

Run all existing portable tests.

Build both the production gateway and diagnostic PlatformIO environments.

## 12. Hardware Verification

The developer will verify on the existing Heltec gateway, using a phone and the current DenClock development network:

1. Erase only saved Wi-Fi credentials.
2. Boot the gateway and observe the setup SSID and password on the OLED.
3. Join the temporary network from a phone.
4. Open the browser configuration page.
5. Scan for household Wi-Fi networks.
6. Enter an incorrect password and verify graceful failure.
7. Enter the correct password and verify successful connection.
8. Confirm that the temporary access point shuts down.
9. Power-cycle the gateway and verify automatic reconnection.
10. Reenter provisioning mode using the recovery mechanism.
11. Change to another Wi-Fi network, if available.
12. Verify existing LoRa reception, packet forwarding, OLED diagnostics, and TCP reconnection.
13. Verify the existing persistent LoRa settings are preserved.

No Surface Go hardware is required for these tests.

## 13. Documentation

Update gateway documentation with simple end-user setup instructions.

Include:

- How to identify the correct DenClock setup SSID
- Where to find the setup password
- How to connect from a phone
- How to open the browser page manually
- How to enter household Wi-Fi credentials
- What successful provisioning looks like
- How to recover from an incorrect password
- How to change networks later
- How to clear Wi-Fi credentials without erasing LoRa settings

Keep engineering-specific details separate from the short end-user setup procedure.

Document any limitations of captive-portal behavior on common phones.

## 14. Development Workflow and Deliverables

Inspect the repository's existing conventions before making changes.

Determine the appropriate story directory and next available story number. Do not assume a number or location from this document.

Implement only what is necessary to satisfy the story.

Reuse existing firmware and test infrastructure where practical.

Report:

- Files changed
- Implementation overview
- Automated test results
- Production and diagnostic PlatformIO build results
- Hardware verification still required
- Any departures from the requirements

Do not modify unrelated software components.

**Do not mark this story completed or accepted until developer review.**


## Acceptance

The developer explicitly accepted this story, including the refined eight-character
setup password using lowercase letters and digits. Temporary WPA2 setup, browser
provisioning, verified credential persistence and Wi-Fi-only clearing are implemented.
Recovery uses a two-second PRG hold after boot, the permitted alternative because
GPIO0 held across reset selects the bootloader. All eleven portable suites and
both PlatformIO environments passed; provisioning sanitizer checks also passed.
The shorter-password refinement passed provisioning/runtime tests and both builds.
Hardware verification remains documented in the setup guide; no separate hardware
results were supplied with acceptance.
