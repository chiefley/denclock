# DenClock — Automatic Brightness Control

> Historical November 5 implementation record. For current calibrated settings,
> dynamic ALS discovery, and verified startup/reboot behavior, use the
> [Clight runbook](../client/docs/clight/Surface%20Go%203%20automatic%20brightness.md).
> For fresh-device installation, use the [master Surface Go provisioning guide](Surface%20Go%203%20Installation%20and%20Deployment%20Guide.md).
> The pending tasks and provisional polling below describe that earlier stage.

**Implementation Record — November 5, 2026**

**Status:** Functionally verified; nighttime calibration pending.

## 1. Design decision

Use **Clight** as an independent Linux automatic-brightness controller rather than implementing custom brightness control within DenClock.

Clight reads the Surface Go's built-in ambient light sensor and adjusts the LCD's physical backlight.

Benefits:
- No additional hardware
- No custom DenClock brightness-control code
- Configurable brightness response
- Smooth backlight transitions
- Works on AC power and battery
- Leaves the DenClock browser application independent of the hardware

## 2. Target environment

- Hardware: Microsoft Surface Go
- OS: Debian 13 (Trixie)
- Desktop: XFCE, X11
- Login manager: LightDM, automatic login as `ben`
- Display backlight: `intel_backlight`
- Ambient light sensor: `/sys/bus/iio/devices/iio:device1`
- Remote administration: SSH and RustDesk

## 3. Installed components

Built from source because packages were unavailable in the configured Debian repositories.

| Component | Version |
|---|---|
| libmodule | 5.0.1 |
| Clightd | 5.7 |
| Clight | 4.10 |

Source repositories:
- https://github.com/FedeDP/libmodule
- https://github.com/FedeDP/Clightd
- https://github.com/FedeDP/Clight

Build locations:
- `~/src/libmodule`
- `~/src/Clightd`
- `~/src/Clight`

Clightd was built with unnecessary optional integrations disabled.

## 4. Configuration

System configuration:
- `/etc/clight/clight.conf`
- `/etc/clight/modules.conf.d/`

User configuration:
- `/home/ben/.config/clight.conf`

The user configuration selects `iio:device1` explicitly and defines identical AC and battery brightness curves.

Initial brightness range was 5–60%. Subsequently reduced the low-light curve points to approximately 1–8% to improve dark-room behavior.

Other configuration decisions:
- Two-second brightness transitions
- Ten-second ambient sampling during testing
- Restore previous brightness on normal exit
- Gamma, dimmer, DPMS, screen compensation, and keyboard modules disabled

**Important:** Preserve the final calibrated `~/.config/clight.conf` as the authoritative configuration. The brightness curve remains subject to nighttime evaluation.

## 5. Authorization discovery

Clightd uses D-Bus and Polkit authorization.

Launching Clight through SSH failed with:

`capture_frames_brightness(): Operation not permitted`

The Clightd Capture action permits active local sessions but not remote sessions.

Launching Clight from the Surface Go's local XFCE desktop through RustDesk resolved the problem.

**Decision:** Run Clight within the local graphical session. Do not weaken Polkit permissions to accommodate SSH.

## 6. Verified operation

Clight successfully:
- Identified Clightd 5.7
- Selected the Surface Go ALS
- Captured five ambient light measurements
- Computed a brightness target
- Adjusted the hardware backlight smoothly
- Responded to changing room illumination
- Operated on battery power

An observed measurement:

`Ambient brightness: 0.525 -> Screen backlight: 0.223`

This resulted in approximately 22% hardware brightness.

## 7. Remaining tasks

- Evaluate brightness in a completely dark room
- Finalize the calibrated response curve
- Select production sampling interval (provisionally 30 seconds)
- Verify XFCE power management does not compete with Clight
- Enable and verify automatic startup through XFCE
- Test after reboot without manual intervention
- Save final configuration and installation procedure in the DenClock repository

## 8. Final architectural verdict

**Provisional decision: Keep Clight independent of DenClock.**

If nighttime testing and automatic startup succeed, there is no need to implement DenClock-specific brightness management.

The Surface Go installation procedure should include Clight, its dependencies, and a copy of the proven configuration file.

Final acceptance remains pending nighttime and reboot tests.
