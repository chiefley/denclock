# Windows LoRa hostname deployment

Use the Windows host's existing unique computer hostname with `.local` appended,
provided the direct multicast A-query check below passes. The recorded NUC name
is `DenClock`, so its destination is `denclock.local`. For separate installations,
choose distinct Windows computer names, for example `lora-den` and `lora-cabin`,
with destinations `lora-den.local` and `lora-cabin.local`. These are computer names,
not LoRa Service aliases. Changing `appsettings.json` does not create a hostname.

The deployment owns hostname availability independently of the LoRa Service.
The application neither registers DNS-SD instances nor starts another responder.
A DNS-SD PTR/SRV advertisement alone is not proof that an A query works. Do not
install another responder for a name that already resolves. Before deployment,
identify the current responder on the NUC in elevated PowerShell:

```powershell
hostname
Get-Service Dnscache
Get-NetUDPEndpoint -LocalPort 5353 | Select-Object LocalAddress,OwningProcess
# Inspect each returned PID (there may be multiple users of this shared port):
Get-Process -Id <PID>
tasklist /svc /fi "PID eq <PID>"
Get-Service | Where-Object { $_.DisplayName -match 'Bonjour|mDNS' }
```

Record whether Windows DNS Client or an installed responder owns the hostname.
Do not remove/replace an existing responder automatically. The current wire test
proves the Windows NUC answers, but does not identify the responding process.
If DNS Client is disabled by policy, obtain administrator review; this story does
not change registry, enterprise policies, VPN configuration, or install Bonjour.
If the existing mechanism fails the acceptance checks, retain the explicit DNS/IP
alternative and investigate before choosing application-owned advertisement.

## Setup and firewall

Check the computer name and LAN address on each Windows server:

```powershell
hostname
Get-NetIPAddress -AddressFamily IPv4
Get-NetConnectionProfile
Get-NetFirewallRule -PolicyStore ActiveStore | Where-Object { $_.DisplayName -match 'mDNS' } |
    Select-Object DisplayName,Enabled,Direction,Action,Profile
```

Use a unique computer name on the shared LAN. If a rename is desired, an
administrator must explicitly schedule it and its reboot. Example only:
`Rename-Computer -NewName 'lora-den' -Restart` in elevated PowerShell.
No rename is needed for the currently responding `denclock.local` installation.

Use a trusted household LAN profile. An administrator may explicitly set the
chosen interface to Private with
`Set-NetConnectionProfile -InterfaceIndex <LAN-index> -NetworkCategory Private`.
Do not change VPN/virtual/public interfaces indiscriminately. Review existing
rules before adding any. If equivalent rules are absent, these elevated commands
allow local-subnet traffic on a trusted Private network:

```powershell
New-NetFirewallRule -Name 'DenClock-LoRa-TCP' -DisplayName 'DenClock LoRa TCP 5210' -Direction Inbound -Action Allow -Protocol TCP -LocalPort 5210 -Profile Private -RemoteAddress LocalSubnet
New-NetFirewallRule -Name 'DenClock-mDNS-In' -DisplayName 'DenClock mDNS UDP 5353' -Direction Inbound -Action Allow -Protocol UDP -LocalPort 5353 -Profile Private -RemoteAddress LocalSubnet
```

Keep default listener `0.0.0.0:5210` so an address change does not leave it bound
to the old IP. Restrictive outbound policies must also allow mDNS UDP 5353 to
`224.0.0.251` and replies, on both server and probe machine. No firewall or profile
changes were made during this story. Do not expose MQTT beyond its existing
loopback listener for this work.

mDNS is local-link multicast, not routed DNS. Gateway and server need multicast
connectivity on the same LAN/VLAN; AP client isolation, multicast filtering, VPNs,
and firewalls can block it. Prefer a single reachable LAN adapter. On multihomed
hosts, verify answers on the gateway's link contain reachable addresses, not only
VPN, virtual or loopback addresses. Do not disable adapters automatically.

## Direct A-record check

From a second Windows machine on the gateway's LAN, run Windows PowerShell 5.1
or later from the repository root, substituting its LAN interface and the NUC's
current LAN IPv4 address:

```powershell
.\lora-service\tools\Test-MdnsHostname.ps1 -Hostname denclock.local -InterfaceAddress 192.168.50.200 -ExpectedAddress 192.168.50.90
```

Use the organization's approved script execution/signing procedure. An unsigned
WSL-share script may be blocked by execution policy; a process-only
`powershell.exe -NoProfile -ExecutionPolicy Bypass -File <script> ...` was used
with explicit authorization during development. Do not change machine policy.

The probe binds shared UDP 5353, joins `224.0.0.251` on the selected interface,
and sends an ordinary ID-zero A/IN query. It parses compressed DNS records and
prints the actual response address/source. It requires the expected address and
rejects multiple differing A answers. It bypasses DNS Client lookup, hosts files,
LLMNR, NetBIOS, and ordinary DNS. It makes no network configuration changes.
A timeout is a failed check, not proof that Windows lacks mDNS support. A pass
is a point-in-time check, not a guarantee against later conflicts. The optional
`-LegacyUnicast` mode uses an ephemeral source port; that mode failed on the
current path while normal multicast succeeded. The gateway formerly used this legacy query mode; its failure was therefore a
compatibility gap in the earlier verification. The corrected firmware uses the
dedicated ESP32 mDNS client for normal multicast queries.
`Resolve-DnsName`, ping by name, or Windows-to-Windows lookup alone is insufficient.

## Gateway configuration

At 115200 baud, enter each command on its own line:

```text
config
set host denclock.local
save
show
reboot
```

Substitute the installation's exact hostname. Saved changes take effect only
on reboot. Leave radio settings and household Wi-Fi credentials unchanged.
Port remains 5210. The gateway never browses/selects other installations; it
resolves exactly its configured destination on every connection attempt.
Ordinary DNS names and IPv4 literals remain supported by `set host NAME`.
A managed DNS name with updated A records or a reserved/static IPv4 address is
the alternative across VLANs or where multicast is unavailable. Neither is an
automatic fallback; changing destination requires explicit configuration.

## Recovery, ownership and conflicts

Windows or the pre-existing system responder must restore hostname resolution
on boot and track LAN address/interface changes. This is an acceptance condition,
not implemented by the LoRa application. No application IP cache is added.
Stopping the LoRa Service should leave host-name resolution working but cause
TCP failure. Restart should restore connections and a new HELLO. Configure the
installed LoRa Windows Service for automatic startup using its existing service
installation procedure; check `Get-Service`/Services after reboot.

The gateway retries failed connections at the existing five-second attempt gate;
lookup/platform caching and synchronous connection timeouts can extend recovery.
After address changes, allow old DNS TTLs to expire and confirm a subsequent
reconnect logs the new IP. Disconnect/reconnect and adapter changes must be tested
on the actual deployment. Stop old listeners and remove test adapters afterward.

Names must be unique across installations on a link. mDNS conflict probing is
specified by [RFC 6762 sections 8–9](https://www.rfc-editor.org/rfc/rfc6762.html#section-8),
but the installed responder's defense/rename behavior must be observed rather
than assumed. Capture `udp.port == 5353` traffic and repeat the A probe during a
controlled duplicate-name test. Multiple IP answers, a changed effective name,
or disappearance of the original name fails deployment acceptance. Resolve the
conflict by assigning a unique computer name to the test/affected installation,
rebooting as required, verifying that exact name, and explicitly saving/rebooting
its gateway. Do not silently follow a renamed DNS-SD instance or select a peer.

An mDNS name is not authentication. The existing gateway cannot detect an
unrelated host answering the same configured name, and can connect to it. The
probe can also miss a conflict if only one responder answers during its window.
Unique naming and the controlled conflict test are deployment requirements;
this story adds no identity checks or automatic conflict-safe failover.

Gateway diagnostics distinguish `hostname resolution failed`, the resolved IP,
`TCP connection attempt failed`, and `Gateway 1 connected; HELLO sent`. Correlate
with service `Gateway 1 connected` and MQTT reports. A HELLO send alone does not
prove MQTT publication; verify the full chain below.

## Hardware acceptance procedure

Record Windows build, computer name, responder process/service, LAN interface,
firewall profile, address, gateway firmware and timestamps. For each step, retain
gateway serial logs, service logs, A-query output and MQTT observations. Use the
[existing MQTT subscriber command](../README.md#verified-nuc-mqtt-deployment).

1. Confirm unique name/responder ownership, pass the A probe from the gateway's
   LAN, configure/save/reboot the gateway, and confirm `show` retains the host.
2. Confirm gateway logs the Windows LAN IP, connects on 5210 and sends HELLO;
   service receives HELLO; fresh sensor values/sequence reach MQTT and DenClock.
3. Stop the installed LoRa Service through Services (or `Stop-Service <actual-name>`).
   Confirm A queries still work and gateway reports TCP failure. Start it again;
   confirm fresh HELLO and resumed forwarding without destination changes.
4. Reboot Windows. Confirm automatic service startup, name resolution and the
   full sensor-to-display path recover without editing the gateway.
5. In a maintenance window, change the server's LAN IP using the deployment's
   DHCP reservation or approved adapter settings. Update the probe's expected
   address. Close the old TCP session, allow cached TTL expiry, and confirm a
   subsequent gateway attempt resolves/connects to the new address. Restore the
   intended allocation; do not change gateway hostname.
6. Disconnect/reconnect the physical LAN or Wi-Fi; confirm A-query and full-path
   recovery. Exercise an applicable LAN-interface change and confirm reachable
   answers, without advertising a virtual/loopback-only address.
7. Save/reboot with a deliberately unused `.local` name. Confirm resolution
   failure and repeated retries, with no connection to another installation.
   Restore/save/reboot the intended name.
8. In an isolated test LAN with test hosts/gateways and MQTT brokers, create a
   duplicate computer-name condition. Observe probing/defense and effective A
   names with capture/probe. Record actual responder behavior. Do not use a live
   unrelated installation. Resolve the duplicate as above and reconfirm both
   intended endpoints; do not accept silent connection to the wrong server.
9. With two uniquely named installations sharing a LAN and their established
   distinct radio frequencies, verify each gateway's resolved IP and each
   broker's fresh reports correspond to its configured Windows server only.
10. Save/reboot with explicit server IPv4; confirm HELLO and normal forwarding.
    Restore/save/reboot the intended hostname and reconfirm operation.

## Story acceptance

The developer fully accepted
[story 004](stories/completed/004-reliable-mdns-hostname-resolution.md) on
2026-10-23 after confirming that `denclock.local` works with the corrected gateway.
The verification limits below retain the distinction between reported results
and checks that were not individually reported. The procedure remains useful for
new installations and network changes.

## Results actually observed

On 2026-10-23, Windows PowerShell on XPS14 sent a normal multicast A/IN query
using Wi-Fi `192.168.50.200`. The Windows NUC answered:

```text
Query: denclock.local A IN via 192.168.50.200 -> 224.0.0.251:5353 (ID 0)
Answer: denclock.local A 192.168.50.90 from 192.168.50.90:5353
PASS: denclock.local answered with expected LAN address 192.168.50.90.
```

A separate TCP connect to `192.168.50.90:5210` succeeded. This demonstrates an
ordinary multicast hostname A answer from the Windows host; it does not prove
which NUC process owns it or the Heltec path. Initial legacy-unicast queries for
`denclock.local` and the development host `xps14.local` timed out. The probe host
had Public Wi-Fi and NordLynx profiles. No system settings were changed.

The initial probe did not verify the Heltec path. After the firmware correction
below, the developer confirmed that `denclock.local` works on the gateway.
Responder-process ownership, detailed MQTT observations, restart/reboot, address
changes, network recovery, unavailable-name and duplicate-name behavior, two
installations, and packet-capture checks have not been separately reported.
The ten steps above remain the acceptance procedure; the working hostname report
does not imply that each step was performed.

## Local verification

- `dotnet build lora-service/LoRa.Service.csproj --no-restore`: passed, zero
  warnings/errors.
- Existing executable test suite: 254 protocol/session/TCP checks passed, then
  failed at `SemanticTests.cs:30` (`valid identities`). The unchanged suite expects
  node 1 `porch`; the unchanged deployment mapping now names it `den`. No mapping
  or test changes were made for this hostname story. Later semantic/MQTT checks
  did not execute. The first sandboxed attempt could not open loopback sockets;
  the reported checks are from the authorized unsandboxed run.
- Live PowerShell probe: normal multicast positive case passed. Wrong expected
  address (`192.0.2.1`) failed as intended while displaying the actual NUC answer;
  an unavailable `denclock-unavailable-20261023.local` query timed out and failed
  as intended. These are network diagnostic checks, not Heltec acceptance.
- `git diff --check`: passed. Runtime service code, configuration, wire protocols and MQTT behavior were
  unchanged in that investigation. Gateway firmware is corrected below.

## Gateway mDNS client and verification

The developer subsequently confirmed that the gateway connects with an IPv4
literal but logs hostname-resolution failure with `denclock.local`. `.local`
lookups now use `mdns_init` and `mdns_query_a` (two-second timeout), rather than
lwIP legacy one-shot resolution. No gateway hostname or services are registered.
Ordinary DNS and IPv4 retain their existing paths. Initialization failures retry;
the library handles Wi-Fi/IP events and every TCP reconnect queries again.

Flash the updated production gateway firmware using the README upload command.
At 115200 baud, run `config`, `set host denclock.local`, `save`, `show`, and
`reboot`, each on its own line. Confirm the resolved NUC LAN IP, HELLO, service
receipt and fresh sensor values through MQTT to DenClock. Repeat after service
restart and Wi-Fi disconnect/reconnect; capture UDP 5353 to confirm hostname A
queries and no gateway hostname/service advertisements. Repeat the unavailable
name and IPv4 checks, then restore the intended hostname. The full ten-step
acceptance procedure above remains available for the unreported checks. The
developer confirmed that the corrected firmware works with `denclock.local` on
hardware. Codex has not independently observed the hardware or packet capture.

Automated checks for the correction: production PlatformIO build passed for
`heltec_wifi_lora_32_V3`; the portable gateway runtime test passed with warnings
as errors. It covers initialization failure/retry, case-insensitive `.local`
selection and trailing-dot removal, query failure/zero address, retry timing,
changed address on reconnect, DNS/IP preservation, TCP diagnostics, HELLO and
forwarding through provisioning recovery. The mDNS fake intentionally exposes no
hostname or service-registration APIs. These checks do not prove wire behavior.
