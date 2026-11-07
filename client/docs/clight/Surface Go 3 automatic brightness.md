# Surface Go 3 automatic brightness

This records the completed Debian 13 / XFCE installation on the Surface Go 3
known in the source conversation as `bensclock`. Brightness is controlled by
Clight and the hardware ALS, independently of the DenClock browser application.
The Windows NUC documentation continues to describe that separate deployment.
The configuration, launcher, and desktop entry were retrieved through read-only
SSH access to the tablet. No live files were changed.

## Evidence and limits

Source: [Brightness Configuration Modes](chatgpt-conversation://6aecab94-7d78-83ea-b31d-4b7af1a18b19),
retrieved for this documentation. The available record contains the final five
exchanges, including the user's post-reboot output and “i verified it is working.”
The user subsequently recovered the exact working configuration from `bensclock`
and verified it on Debian 13, Surface Go 3, including automatic operation after
reboot. [clight.conf.example](clight.conf.example) now preserves those settings.
The startup script and desktop entry were subsequently recovered directly from
`bensclock` and are preserved as examples alongside the configuration. The build
transcript remains unavailable.

| Item | Record |
| --- | --- |
| Platform | Surface Go 3, Debian 13, XFCE (user-supplied context) |
| Source-built components | libmodule 5.0.1, Clightd 5.7, Clight 4.10 (user-supplied versions) |
| Sensor selection | IIO device named `als`; numeric device identifier changes across boots |
| Verified process | One `clight` process, executable `/usr/bin/clight` |
| Verified sensor | `/dev/iio:device1` in the final reboot log |
| Verified output | `intel_backlight` updates and repeated successful sensor captures |
| Polling | Verified configuration: 10 seconds for all AC/battery states |
| Transitions | Verified configuration: 2000 ms |
| Curves | Verified identical AC/battery arrays: 0.01, 0.02, 0.03, 0.05, 0.08, 0.20, 0.27, 0.35, 0.44, 0.53, 0.60 |
| Auxiliary modules | Gamma, dimmer, DPMS, screen, and keyboard disabled |

The working configuration and automatic operation after reboot are user-verified.
A fresh-device installation has not been reproduced as part of this documentation
update. Verbose logging and restore-on-exit are enabled in the recovered config.

## Installation and recovery procedure

1. Establish Debian 13 with a working local XFCE login and accessible built-in
   ALS/backlight. Kernel version, Surface-specific drivers, package dependencies,
   and XFCE power-manager settings from the successful installation are unknown.
2. Recover the build transcript before rebuilding. Preserve the source-built
   versions above. Exact source commits, dependencies, build order, CMake flags,
   install prefixes, linker setup, service activation, and installed Polkit policy
   are unknown. There is deliberately no speculative `apt`, build, or service
   command sequence here. Consult the versioned upstream source and record the
   actual commands during a future reproduction:
   [libmodule 5.0.1](https://github.com/FedeDP/libmodule/tree/5.0.1),
   [Clightd 5.7](https://github.com/FedeDP/Clightd/tree/5.7), and
   [Clight 4.10](https://github.com/FedeDP/Clight/tree/4.10).
3. Preserve the recovered brightness calibration in
   [clight.conf.example](clight.conf.example). Both power sources use the same
   verified eleven-point array. Do not substitute a webcam curve or infer a
   replacement calibration from the two observed mapping pairs below.
4. Review the configuration asset: it uses 10-second polling for all
   day/night/event states on both power sources, 2000-ms transitions, verbose
   logging, restore-on-exit, and disabled gamma, dimmer, DPMS, screen, and keyboard
   modules. `devname = "iio:device1";` is a placeholder, not a stable hardware
   identifier; the dynamic ALS discovery launcher replaces it at startup.
5. After reviewing the config, install it as `~/.config/clight.conf`.
   Keep a backup of any existing configuration. Install the recovered
   [launcher](start-clight.sh.example) as `~/.local/bin/start-clight.sh`
   and make that file executable. It selects the first sysfs IIO device whose
   `name` equals `als`, replaces a matching numeric `devname` assignment using
   `sed -i`, and executes `/usr/bin/clight`. It exits if no ALS is found. It does
   not detect multiple matching devices or verify that the replacement occurred;
   retain the assignment format in the config example. Concurrent launches/config
   edits are not supported. The script is preserved exactly as recovered.
6. Install the recovered [desktop entry](clight.desktop.example) as
   `~/.config/autostart/clight.desktop`. Its `Exec` points to
   `/home/ben/.local/bin/start-clight.sh`; adjust that absolute path for a different
   account. Desktop `Exec` is not a shell command: do not substitute `~` or `$HOME`.
   Check XFCE's existing startup entries and saved session so only one launcher
   starts.
7. Test from a terminal inside the local XFCE graphical session, then use the
   next planned reboot/login to verify automatic startup with the checks below.
   Verify visible brightness response in dim and bright light and on both AC and
   battery. This documentation task does not perform those tablet operations.

Until steps 1–2's missing build evidence is recovered, this is a recovery/runbook
outline, not a complete from-scratch reproducible build recipe.

## Session authorization: local XFCE versus SSH

The user corrected an earlier diagnosis: the successful manual Bash launch was
in a terminal directly on the tablet. A later SSH-launched process discovered
the sensor but received `Operation not permitted`. That SSH process had an
unset `DISPLAY` and belonged to a separate login-session scope. Its failure did
not establish that XFCE autostart would fail; the subsequent reboot succeeded.

Launch Clight in the authorized local graphical session. SSH is useful for
reading process/config/log state after login, but an SSH launch does not test
the same session's Polkit authorization. Do not work around this with root
execution, permission changes, or blanket Polkit grants. Exact installed policy
and rules were not captured and must be recovered for a full reproduction.

## Debugging and post-login checks

These diagnostic commands were present in the source conversation:

```sh
pgrep -a -x clight
grep devname ~/.config/clight.conf
tail -40 /run/user/$(id -u)/clight/clight.log

echo "Session: ${XDG_SESSION_ID:-unset}"
echo "Display: ${DISPLAY:-unset}"
cat /proc/$$/cgroup
```

Additional read-only inspection commands for the new runbook:

```sh
for device in /sys/bus/iio/devices/iio:device*; do
    [ -r "$device/name" ] || continue
    printf '%s: ' "$device"
    cat "$device/name"
done
cat /sys/class/backlight/intel_backlight/brightness
cat /sys/class/backlight/intel_backlight/max_brightness
```

Expect one process, a `devname` matching the current `als` device, successful
captures, and hardware backlight changes. A process alone is insufficient proof.
For missing ALS, inspect sysfs names before changing configuration. For permission
errors, first distinguish the originating session. For no autostart, inspect the
desktop entry's account path, launcher executable permission, and XFCE startup
configuration. Do not start an extra Clight instance merely to inspect an existing
one. The recovered configuration enables verbose logging.

## Verified reboot record

The final supplied output showed one `/usr/bin/clight` process and
`devname = "iio:device1";`. Sanitized excerpts (personal coordinates omitted):

```text
[09:50:15] Captured [5/5] from '/dev/iio:device1'. Ambient brightness: 0.488.
[09:50:15] Ambient brightness: 0.488 -> Screen backlight: 0.175.
[09:50:15] Set timeout of 10s 0ns on fd 31.
[09:50:25] Ambient brightness: 0.482 -> Screen backlight: 0.172.
[09:50:25] Backlight 'intel_backlight' level updated: 0.17.
```

Captures continued at 09:50:36 and 09:50:46. The user then confirmed operation.
The conversation identifies this as the reboot/autostart test and reports
dynamic discovery working when the device number changed. This documentation
does not independently observe the reboot or prove every AC/battery setting.

The same log contained `Failed to validate display state request`, location
activity, and `Failed to validate location request` while brightness worked.
Do not reinterpret these as sensor failures. Their precise cause is unknown;
the recovered configuration explicitly disables the five auxiliary modules listed
above, but does not define a `daytime.disabled` setting.

## Configuration syntax references

The configuration's setting names and units were checked against Clight 4.10's
[backlight config](https://github.com/FedeDP/Clight/blob/4.10/Extra/config/modules.conf.d/backlight.conf),
[sensor config](https://github.com/FedeDP/Clight/blob/4.10/Extra/config/modules.conf.d/sensor.conf),
and [config loader](https://github.com/FedeDP/Clight/blob/4.10/src/conf/config.c).
These upstream references document syntax/meaning; the recovered tablet
configuration supplies the verified values.
In particular, the loader provides no `daytime.disabled` setting; none is invented
here.
