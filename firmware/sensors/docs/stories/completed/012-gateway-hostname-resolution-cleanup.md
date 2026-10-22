# Gateway Hostname Resolution Cleanup

Status: Accepted by the developer.

## Background and user story

The gateway is a client of the Windows LoRa service and has no requirement to
advertise its own hostname. Remove its fixed `denclock-gateway` mDNS responder
so multiple installations avoid unnecessary name conflicts, and provide clear
connection diagnostics for troubleshooting.

## Requirements

- Remove responder initialization without introducing another advertisement mechanism.
- Retain IPv4, ordinary DNS and `.local` hostname support, preferring existing
  platform resolution facilities where adequate.
- Resolve the configured destination on every new TCP connection attempt;
  introduce no permanent application IP cache and preserve the five-second retry behavior.
- Distinguish resolution failure, successful resolution followed by TCP failure,
  and TCP connection established with HELLO sent. Report the resolved IP and
  avoid repetitive logging beyond useful connection-attempt diagnostics.
- Preserve persistent configuration format/commands, NVS, LoRa parameters,
  bounded serial-console scheduling, gateway ID/HELLO, port/framing, raw radio
  forwarding, OLED, Wi-Fi provisioning/credentials, Windows services and client.
- Implement only necessary changes under existing repository conventions.

## Verification and review

Update portable tests for IPv4, ordinary DNS, `.local` through the selected
resolver interface, resolution failure/retry, resolved-address TCP failure,
connection/HELLO, re-resolution after disconnect, bounded serial processing and
packet forwarding. Run portable tests and build production and diagnostic
PlatformIO environments. Report files, checks, results and outstanding hardware
verification. Document `.local` behavior requiring hardware verification.

Do not mark this story complete or accepted, move it to completed, or stage a
completed-story handoff until explicit developer acceptance.

## Acceptance

The developer explicitly accepted this story. The gateway resolves destinations
through the platform resolver without starting an mDNS responder and reports
resolved addresses and connection outcomes. All ten portable test suites and
both gateway PlatformIO environments passed. Hardware verification of `.local`
multicast behavior, address-change recovery and absence of gateway advertisements
remains documented in the firmware README. Platform resolver timeouts replace
the former one-second ESPmDNS timeout; the five-second retry gate is preserved.
