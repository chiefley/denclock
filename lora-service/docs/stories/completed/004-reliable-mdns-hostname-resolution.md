# Reliable mDNS Hostname Resolution for the Windows LoRa Service

Implement this story in the local DenClock repository, following `AGENTS.md` and `docs/DEVELOPMENT.md`. Before implementation, save this story as a numbered Markdown file at the lowest existing story scope containing the work. Choose the next available number across active and completed stories.

## Background

The Heltec sensor and LoRa/Wi-Fi gateway configuration, including browser-based household Wi-Fi provisioning, have been tested on hardware.

The gateway already supports a persistent destination hostname and resolves it before each TCP connection attempt. `.local` names use `WiFi.hostByName` through the pinned ESP32 framework’s lwIP resolver.

The remaining requirement is to make the Windows LoRa Service reachable reliably through an explicitly configured mDNS hostname.

## Deployment constraints

- Independent installations already use different LoRa frequencies.
- Each installation has its own Windows server hardware running one LoRa Service, one MQTT broker and one DenClock service.
- Independent installations may share a LAN.
- Each gateway explicitly targets its installation’s unique service hostname.
- Production software must run natively on Windows.
- Preserve TCP port 5210 and the existing gateway hostname configuration.
- No installation identifier, MQTT namespace change, radio packet change or Gateway Protocol change is required.
- No automatic DNS-SD browsing or selection of the first available service is required.

## Inspect before implementing

Read the current implementation and relevant contracts, particularly:

- `firmware/sensors/lora-gateway/include/service_resolver.h`
- `firmware/sensors/lora-gateway/include/gateway_config.h`
- `firmware/sensors/lora-gateway/include/gateway_console.h`
- `firmware/sensors/lora-gateway/src/main.cpp`
- `firmware/sensors/lora-gateway/platformio.ini`
- `firmware/sensors/docs/contract.md`
- `lora-service/Program.cs`
- `lora-service/GatewayWorker.cs`
- `lora-service/appsettings.json`
- `lora-service/README.md`
- `lora-service/docs/contract.md`
- `docs/ARCHITECTURE.md`

These paths were reviewed on GitHub at commit `3ad9753a5e6225626f4b7e113c663526549badcf`. Inspect the current local workspace rather than assuming it matches that revision. Preserve unrelated changes.

## Requirements

### 1. Establish Windows hostname ownership

Choose and implement the smallest reliable Windows-native mechanism that makes the intended `.local` hostname resolvable by the existing gateway resolver.

Distinguish hostname resolution from DNS-SD service registration. Advertising a service instance alone is insufficient unless the gateway’s actual hostname query resolves correctly.

First establish whether Windows can reliably answer for its existing unique computer hostname without additional application infrastructure. If that meets the requirement, prefer documenting and validating that deployment mechanism. Otherwise implement the necessary advertisement within the LoRa Service lifecycle.

Codex must demonstrate that the Windows host responds to an ordinary mDNS A-record query for its hostname

Do not rename the Windows computer or make broad machine-wide networking changes automatically. Explain any required administrator setup and provide explicit commands where appropriate.

Document the chosen mechanism, its ownership and its limitations. Do not add a second responder for a name already owned by another component.

### 2. Explicit destination selection

Support an installation-specific hostname such as:

- `lora-den.local`
- `lora-cabin.local`

These are examples, not mandatory names. If the chosen mechanism uses the Windows computer name, document that naming relationship clearly.

The gateway must continue using its explicitly configured destination. It must never fall back to another discovered DenClock installation.

Retain ordinary DNS names and IPv4 literals as supported destination options.

### 3. Lifecycle and network recovery

The chosen mechanism must account for:

- Windows startup and reboot.
- LoRa Service startup, stop and restart.
- LAN address changes.
- Network disconnect and reconnect.
- Relevant network-interface changes.

If advertisement belongs to the LoRa Service, register and withdraw it with the service lifecycle and avoid stale advertised addresses. Advertise a reachable LAN address rather than loopback or an unrelated virtual adapter.

Preserve the gateway’s existing reconnect behavior and fresh resolution on each connection attempt. Avoid adding an application-owned permanent IP cache.

### 4. Failure and conflict behavior

Provide useful diagnostics for hostname advertisement or registration failures when the application owns that operation.

Document how duplicate names are detected and resolved under the chosen mechanism. A conflict must not cause the gateway to silently select an unrelated installation. If the platform changes an advertised name, make the effective name and required gateway configuration clear.

Keep hostname resolution failure, TCP connection failure and successful HELLO transmission distinguishable in gateway diagnostics.

Hostname resolution is not authentication; do not introduce an authentication redesign in this story.

### 5. Preserve existing behavior

Preserve:

- Household Wi-Fi provisioning and saved credentials.
- Gateway radio and hostname settings.
- LoRa reception and raw-packet forwarding.
- Existing HELLO and RX_PACKET wire formats.
- MQTT publication topics and payloads.
- DenClock subscriptions and presentation behavior.
- Console and Windows Service execution of the LoRa Service.

No gateway firmware changes are anticipated. The existing gateway already supports persistent hostname configuration, `.local` resolution, and reconnection. Implement the required Windows-side hostname advertisement or configuration without modifying gateway firmware. If testing demonstrates a specific gateway defect, document the evidence before proposing any firmware changes.

## Verification

Run appropriate automated checks for changed behavior. If adding application-owned advertisement code, include meaningful tests for its configuration and lifecycle where practical.

Provide a concrete hardware acceptance procedure covering:

1. Configure the gateway with the intended `.local` hostname, save and reboot.
2. Verify resolution to the Windows host’s LAN address, connection on port 5210, HELLO and normal sensor forwarding through MQTT to DenClock.
3. Stop and restart the LoRa Service; verify gateway recovery.
4. Reboot Windows; verify recovery without changing gateway configuration.
5. Change the Windows host’s LAN address in a controlled test; verify subsequent gateway reconnection resolves the new address.
6. Disconnect and reconnect the network; verify recovery.
7. Use an unavailable hostname; verify clear failure diagnostics and continued retry.
8. Exercise a duplicate-name condition safely; verify the documented conflict behavior.
9. With two installations on one LAN and distinct hostnames, verify each gateway connects to its configured server. Use the already established frequency partitioning.
10. Verify that an explicit IPv4 destination still works.

Document required Windows firewall rules and any LAN multicast assumptions. Do not assume successful Windows-to-Windows name resolution proves the Heltec gateway path.

Clearly separate automated results, tests actually performed on hardware, and hardware acceptance steps still pending. Do not claim hardware verification that was not performed.

## Documentation and handoff

Update affected enduring contracts, architecture and deployment instructions to describe:

- The chosen hostname ownership mechanism.
- How to choose a unique name for each installation.
- How to configure the gateway destination.
- Windows setup and firewall requirements.
- Recovery, diagnostics and conflict handling.
- Local-link mDNS limitations and the ordinary DNS/IP alternative.

Report the implementation, checks performed, remaining limitations and exact human acceptance steps.

Leave the story active pending explicit human acceptance. Do not move it to `completed/`, declare hardware acceptance, or commit automatically.


## Acceptance

Fully accepted by the developer on 2026-10-23 after confirming that the corrected
gateway works with `denclock.local`. Windows supplies the existing hostname
response; no LoRa Service responder is added. The developer authorized the
firmware departure from the original scope: `.local` lookups use the dedicated
ESP32 mDNS client without advertising a gateway hostname or services. Ordinary
DNS and IPv4 remain supported.

The production firmware build and portable runtime tests passed. The direct
Windows multicast A-query probe passed. Detailed recovery/conflict checks were
not individually reported; full acceptance does not imply Codex performed them.
See the [deployment guide](../../mdns-deployment.md) for operational checks and
verification limits, including the existing unrelated semantic-test mismatch.
