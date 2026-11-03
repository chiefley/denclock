# Restore On-Screen Keyboard in DenClock Configuration

## Background and objective

Ben's Clock uses Debian 13, XFCE, Xorg, Chromium 156, and Onboard on a
Surface Go 3. Configuration fields receive touch focus but do not invoke
Onboard, although native applications do and Onboard auto-show is enabled.
Restore automatic keyboard invocation while preserving Story 015's fullscreen
app window and dedicated Chromium profile.

## Investigation and implementation

- Inspect the existing launcher and window-control implementation and guidance.
- Investigate Chromium's accessibility exposure used by Onboard; evaluate
  `--force-renderer-accessibility=complete` as the minimal configuration fix.
- Prefer standard accessibility configuration. Do not introduce custom keyboards,
  JavaScript keyboard launching, services, daemons, polling, or HTTP endpoints.
- Preserve XFCE/Xorg, fullscreen switching, profile, startup, and relaunch behavior.
- Keep changes minimal, isolated, and reversible. If the simple fix fails,
  restore the prior working launcher and report findings and alternatives before
  implementing anything more elaborate; further scope requires developer approval.

## Deployment and documentation

Deploy the launcher through established SSH access to `ben@bensclock`, preserving
an exact rollback copy. Restart only what testing requires. Document the change,
any established cause, dependencies, diagnostics, and rollback in existing Linux
deployment guidance without duplicating instructions.

## Acceptance criteria

1. Touching an editable configuration field automatically shows Onboard.
2. Onboard can enter and edit the selected field.
3. Keyboard visibility behaves appropriately when editable focus ends.
4. Configuration works fullscreen and windowed; fullscreen switching still works.
5. DenClock starts correctly after reboot.
6. No new services or custom keyboard-control mechanisms are needed.

Automated checks do not establish touchscreen correctness. Leave this story
active until the developer verifies the actual Surface Go and explicitly accepts
it, including preserved DenClock behavior and documentation.

## Acceptance

Accepted by the developer on November 3, 2026 following Surface Go verification.
The Chromium accessibility flag restores automatic Onboard invocation while
preserving the existing profile and fullscreen/windowed operation. The revised
launcher was deployed with a rollback copy; shell syntax and diff checks passed,
AT-SPI exposed DenClock web content, and remote window-state checks passed.
Physical touchscreen and reboot acceptance are provided by the developer.
No new runtime dependencies or intentional departures were introduced.
