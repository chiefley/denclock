# Reliable System Time and Timezone Configuration

**Type:** Infrastructure / Documentation / Verification
**Priority:** High
**Reference system:** `bensclock`
**Next deployment:** `iansclock`

## Objective

Ensure every DenClock client maintains accurate time through an operating-system network time synchronization service, with an explicitly configured local timezone.

Document the verified implementation and incorporate it into the master Surface Go 3 provisioning procedure.

## Background

During the November 7, 2026 configuration audit, `bensclock` was found to have:

- `System clock synchronized: no`
- `NTP service: n/a`
- No active network time synchronization daemon

DenClock's browser synchronizes with its local DenClock server. However, the server obtains time from the underlying Linux system clock. Therefore, browser/server synchronization alone does not establish accurate time.

The discrepancy was observed when two DenClock displays in the den disagreed.

## Verified Implementation

On Debian 13, the following commands were used:

```bash
sudo apt update
sudo apt install systemd-timesyncd
sudo systemctl enable --now systemd-timesyncd
```

The timezone was subsequently changed to Chicago:

```bash
sudo timedatectl set-timezone America/Chicago
```

Verification showed:

```text
Time zone: America/Chicago (CST, -0600)
System clock synchronized: yes
NTP service: active
RTC in local TZ: no
```

The systemd service was:

```text
systemd-timesyncd.service
Loaded: enabled
Active: active (running)
Status: Contacted time server 38.80.8.230:123
```

An initial `timedatectl` D-Bus activation timeout occurred during timezone configuration, but subsequent inspection confirmed that the timezone change succeeded and time synchronization remained operational.

## Requirements

### 1. Document network time synchronization

Update the master Surface Go 3 provisioning guide to specify `systemd-timesyncd` installation, enablement, and verification.

Include:

- Installation commands
- Service status checks
- Synchronization verification
- NTP diagnostics
- Behavior when network time is temporarily unavailable
- Verification after reboot

Do not assume Tailscale provides time synchronization.

### 2. Document timezone configuration

Every DenClock installation must have an explicit IANA timezone appropriate to its deployment location.

For example:

```bash
sudo timedatectl set-timezone America/Chicago
```

Explain that the Linux system clock remains synchronized to UTC and that timezone conversion, including daylight saving time, is handled by the operating system.

The clock timezone and weather forecast location are independent configuration choices.

### 3. Review DenClock time architecture

Inspect the current server and client implementations.

Confirm how the browser obtains and maintains its time reference, including the `/api/time` endpoint.

Determine whether any application-code changes are necessary. Do not introduce a second NTP client or duplicate system-level synchronization logic without identifying a real requirement.

Document the findings.

### 4. Update provisioning acceptance criteria

The master installation guide must require:

- Correct timezone
- Active and enabled network time synchronization service
- `System clock synchronized: yes`
- Successful synchronization after reboot
- Correct behavior following temporary network loss and recovery
- Comparison against an independent time reference
- Correct clock display after daylight saving time transitions, where applicable

Distinguish tests already performed from tests awaiting the next installation.

### 5. Preserve evidence

Document the successful November 7 installation and verification on `bensclock`.

Do not claim that reboot persistence or network-loss recovery has been tested unless supported by actual evidence.

## Constraints

- Prefer operating-system time synchronization over application-specific NTP logic.
- Do not modify the working Surface Go during this documentation task.
- Do not alter unrelated DenClock functionality.
- Preserve the distinction between verified implementation and future acceptance tests.
- Do not include private network information unnecessarily.

## Acceptance Criteria

- [ ] Network time synchronization setup is fully documented.
- [ ] Explicit timezone configuration is documented.
- [ ] The master provisioning guide incorporates both requirements.
- [ ] Existing timekeeping architecture is reviewed against the source code.
- [ ] Reboot and network-recovery verification procedures are included.
- [ ] `bensclock` verification results are accurately recorded.
- [ ] Remaining tests for `iansclock` are explicitly identified.
- [ ] No unrelated application or system configuration is changed.

## Completion Report

Report modified files, implementation findings, documentation updates, validation performed, and any unresolved questions.

Follow the repository's established development-story conventions and workflow.

## Acceptance

Accepted by the developer on November 7, 2026. The master provisioning guide now
includes systemd-timesyncd installation and diagnostics, explicit timezone setup,
source-reviewed time architecture, and the supplied bensclock verification evidence.
No application changes were needed. Clock tests, documentation-link validation,
and whitespace checks passed. No live system changes were performed.
Reboot, network recovery, independent-reference comparison, and DST checks remain
explicitly pending for the next installation; acceptance does not claim those
hardware tests were executed.
