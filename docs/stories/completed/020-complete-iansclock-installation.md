# Story: Complete IansClock Installation Using the Documented DenClock Procedure

## Objective

Complete the installation and configuration of IansClock, a Surface Go 3 running Debian 13, using the installation procedure and scripts already developed in the DenClock GitHub repository.

This is an opportunity to validate that the documented procedure is sufficient to reproduce a working DenClock installation without having to reverse-engineer BensClock again.

## Current State

IansClock has completed its initial operating system installation.

- Hardware: Microsoft Surface Go 3
- Operating system: Debian 13.7.0
- Hostname: `iansclock`
- Username: `ian`
- IP address: `192.168.50.156`
- Wi-Fi: Operational
- SSH: Installed, running, and accessible
- Sudo: Installed; user `ian` added to the sudo group
- APT: Internet repository configured and `apt update` completed successfully
- Secure Boot: Disabled to permit USB boot

SSH access has been verified from another computer.

## Phase 1 — Update the Installation Documentation

Review the complete IansClock installation conversation and update the existing installation procedure to incorporate the experience.

Document these issues and their resolutions:

1. Booting the Surface Go 3 from the Debian installation USB using UEFI settings.
2. Disabling Secure Boot.
3. Installing Debian 13.7.0.
4. The installer requesting `iwlwifi-cc-a0-77.ucode`, despite Wi-Fi ultimately working without manually supplying the firmware.
5. The missing APT Internet repository configuration.
6. Configuring the Debian 13 Trixie package repository.
7. Installing `openssh-server` and `sudo`.
8. Adding `ian` to the sudo group.
9. Establishing SSH connectivity.

Distinguish between required steps, troubleshooting procedures, and observations specific to this installation.

Preserve the existing documentation and its history.

## Phase 2 — Review the Existing Installation Procedure

Examine the installation guide and associated configuration scripts already developed from the earlier BensClock analysis.

**Treat this existing documentation and these scripts as the authoritative installation baseline.**

Do not repeat the original investigation of BensClock.

Do not access or modify BensClock.

If the documentation is incomplete, ambiguous, or inconsistent, identify the specific deficiency. Request permission before consulting BensClock to resolve it.

Determine the remaining steps necessary to bring IansClock into the documented operational state.

## Phase 3 — Configure IansClock

Connect to IansClock using SSH:

`ian@192.168.50.156`

Inspect its current configuration and determine which documented installation steps have already been completed.

Execute the remaining applicable installation and configuration scripts.

Requirements:

- Prefer existing repository scripts rather than inventing replacement procedures.
- Check prerequisites before executing scripts.
- Avoid repeating completed operations unnecessarily.
- Make configuration steps idempotent wherever practical.
- Record commands executed and their results.
- Verify each significant configuration change.
- Stop and request approval before destructive, irreversible, or security-sensitive operations.
- Do not modify BensClock.
- Do not expose credentials or secrets in logs or repository files.

Work may perform ordinary package installation, updates, and configuration changes necessary to complete the documented installation, subject to the safeguards above.

## Phase 4 — Validation

Use the existing DenClock documentation and tests to verify that IansClock is functioning as intended.

Validate all applicable documented requirements, including:

- Startup and automatic application launch
- Display configuration
- Clock synchronization and time zone
- Weather data acquisition and presentation
- Network connectivity
- Recovery after reboot
- Any documented kiosk-mode or power-management requirements

Do not assume these features are configured merely because installation scripts completed successfully.

Where physical interaction or visual inspection is required, identify the necessary test for the user.

## Phase 5 — Documentation and Audit Trail

Update the repository with:

1. The revised installation procedure.
2. Any necessary script corrections.
3. A record of configuration changes made to IansClock.
4. Installation and validation test results.
5. Remaining issues and deviations from the documented procedure.

Maintain a machine-readable installation status record so subsequent sessions can resume without reconstructing previous work.

Avoid committing credentials, private keys, or other secrets.

## Acceptance Criteria

The story is complete when:

- IansClock operates as a DenClock display according to the documented requirements.
- The installation procedure accurately describes the steps required to reproduce the installation.
- All applicable validation tests pass, or remaining failures are explicitly recorded.
- Any discrepancies between the documented procedure and the actual installation are resolved or documented.
- BensClock has not been accessed or modified.
- The GitHub repository contains an auditable record of the installation and its outcome.

## Guiding Principle

**The purpose is not simply to get IansClock working. It is to demonstrate that DenClock can be reliably reproduced from its documented installation procedure, with AI executing the process and maintaining an audit trail.**

BensClock is the original reference implementation. The existing installation documentation is the specification for this exercise. IansClock is the reproducibility test.

## Acceptance

Accepted by the developer on November 9, 2026.

IansClock was provisioned from the documented Surface Go 3 baseline, with
synchronized Eastern time, weather, backend services, automatic desktop startup,
Clight brightness control, Tailscale SSH and RustDesk. The missing Onboard desktop
accessibility prerequisite was corrected in the procedure and retained script;
the developer verified keyboard appearance and text entry without the startup
dialog after reboot. Verification included 62 client tests, four helper tests,
live service/API/MQTT/weather checks, deployed-artifact hashes and reboot recovery.

The developer accepted the installation with remaining sensor, power/outage,
external-network, DST and backup/restore checks explicitly recorded in the
installation status. Those checks remain unverified rather than implicitly passed.
The supplied account records initial Debian setup; the exact ISO/checksum and
complete original installation transcript were unavailable. BensClock was neither
accessed nor modified. No new application behavior was introduced.
