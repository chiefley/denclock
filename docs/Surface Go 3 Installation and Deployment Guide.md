# DenClock — Master Surface Go 3 Provisioning Guide

Reference: `bensclock`. First reproduction: `iansclock`.
Research/inspection date: November 7, 2026 (America/New_York).
GitHub baseline: [`b9909482e586ead4d434ff5f9934561a3ef08a0c`](https://github.com/chiefley/denclock/commit/b9909482e586ead4d434ff5f9934561a3ef08a0c).

This is the authoritative Surface Go installation sequence. It replaces the
earlier version of this same document. Detailed subsystem runbooks remain
authoritative for their individual procedures. The installed reference works;
the November 9 Ian trial has now reproduced the backend, desktop and brightness
setup on a fresh tablet, including reboot recovery. Full physical, sensor, remote
access and power acceptance remains pending; see the trial record below.

**Readiness:** ready for a supervised `iansclock` installation trial, with the
hold points in section 13 resolved as encountered. It is not an unattended or
fully validated provisioning recipe. Time synchronization must be established
before clock acceptance. The Ian trial retains a guarded first-install adaptation in
[its configuration script](installations/iansclock-configure.sh); this is not a
generic multi-device or unattended provisioning tool.

## How to use the evidence

- **Observed (O):** read directly from `bensclock` through ordinary OpenSSH over
  Tailscale on November 7, or compared with current repository files. Observation
  of configuration is not proof that a fresh installation or reboot will work.
- **Documented (D):** repository implementation, accepted story, original user
  confirmation/terminal output, or cited vendor instructions. Identify which kind
  when it matters; assistant suggestions alone are not execution evidence.
- **Inferred (I):** a conclusion or adaptation still requiring validation.

Commands described as **adapted** are manual equivalents of documented operations,
parameterized for the new tablet; they were not executed during this research.
Vendor-supported steps without recovered command history are marked as such.
All modifying commands below are instructions for the **new installation** only.
`bensclock` was inspected read-only; it was not installed, reconfigured, restarted,
rebooted, or used for destructive tests. Secrets and household coordinates are
excluded from this guide.

## 0. Prepare the installation record and choose device values

- [ ] Start the observation log in section 14 before erasing or installing anything.
- [ ] Record the source commit, ISO filename/checksum, firmware version, package
  versions, and each deviation. Read the latest GitHub files, not a stale mirror.
- [ ] Choose the following values; do not copy Ben's identity or credentials.

| Value | Common baseline | `iansclock` choice / reference distinction |
|---|---|---|
| Hardware/OS | Surface Go 3 x86-64; Debian 13, XFCE/X11, LightDM | Record actual RAM/storage and firmware; no assumption all units are 64 GB |
| Hostname | Unique per tablet | `iansclock`; reference is `bensclock` |
| Desktop/admin user | Real login account with sudo | Confirmed `ian` in the November 9 trial; use the chosen account on other tablets |
| Backend service user | `denclock`, non-login system account | Same role on each tablet; numeric UID/GID are incidental |
| Application paths | `/opt/denclock/server`, `/opt/denclock/lora-service` | Common |
| Persistent presentation | `/var/lib/denclock/server/presentation.json` | Household-specific contents |
| Logs | `/var/log/denclock/` | Common path, separate from published binaries |
| Browser | Local `http://localhost:5000`; dedicated Chromium profile | Never point Ian's browser at Ben's server |
| Network/admin | Household Wi-Fi, ordinary SSH over Tailscale | New DHCP/Tailscale addresses, SSH host keys, RustDesk ID/password |
| Time/location | Correct system timezone and synchronized UTC | Choose Ian's timezone and forecast location independently |
| Sensors/gateway | Numeric identities, household-specific radio/network settings | Choose labels, IDs, stale limits, frequency/sync word; never assume Ben's assignments |
| Charge/power policy | Continuous AC display; deliberate battery fallback | Confirm firmware Battery Limit and battery sleep policy |

On the **development computer**, set these variables for a shell session. Replace
`ian` if another login was chosen; replace the host with the verified Tailscale
address if hostname resolution is unavailable:

```sh
cd ~/src/denclock
CLOCK_USER=ian
CLOCK_HOST=iansclock
CLOCK_REMOTE="${CLOCK_USER}@${CLOCK_HOST}"
```

Do not assume these variables exist in another terminal or on the tablet. Every
remote modifying operation must target `iansclock`; check `hostname` first.

## 1. Hardware, firmware, installation media, and disk

**D:** the original conversion conversation confirms the standard Debian Live
environment worked with touchscreen, Wi-Fi, native display, USB hub/keyboard,
and Onboard. No linux-surface kernel was required. **O:** the reference now runs
the ordinary Debian `6.12.113+deb13-amd64` kernel at 1920 × 1280.

- [ ] Back up any wanted Windows files and record firmware settings before erasing.
  Keep the Surface charger connected. Have a USB-C hub, physical keyboard, and
  installer USB available: the original Live session could not initially be typed
  into using touch alone.
- [ ] Download a **Debian 13 amd64 Live Xfce** image from
  [Debian Live images](https://www.debian.org/CD/live/) and verify it using
  [Debian's image-verification instructions](https://www.debian.org/CD/verify).
  Record the exact image/checksum; the original image checksum was not preserved.
- [ ] Write the image using Rufus on Windows, as documented in the original setup.
  Copying the `.iso` as a file does not create the installer. The original Rufus
  mode/partition options were not recovered; record those actually used.
- [ ] Enter Surface UEFI with Volume Up + Power. Enable USB boot as needed; boot
  USB with Volume Down + Power. The exact Secure Boot setting on the reference
  was not observed. Do not assume disabling Secure Boot is required. If boot fails,
  record the error and use the applicable Surface/Debian documentation.
- [ ] In Live Xfce, test touch, Wi-Fi, native resolution, keyboard/hub, and storage.
  If useful, the original temporary keyboard test was:

```sh
sudo apt update
sudo apt install onboard
onboard
```

- [ ] Launch Install Debian (Calamares). Choose language, timezone, keyboard,
  hostname `iansclock`, and the intended login account. Select the **internal**
  disk by identity and size, not by a copied `/dev` name. Erase disk removes Windows
  and all existing data; inspect the final partition summary before proceeding.
- [ ] Finish, remove installer media, boot from internal storage, and log in.

**Expected:** installed Debian desktop with functioning touch and Wi-Fi.
**O reference layout:** 58.2 GiB eMMC at `/dev/mmcblk1`, 300 MiB FAT EFI,
49.7 GiB ext4 root, 8.3 GiB swap. **D:** the user chose default partitioning after
selecting the approximately 58.24 GB internal disk. These sizes/device names are
observations, not a mandatory partition recipe. Disk encryption and its effect on
unattended boot were not recovered; decide and record before installation.

**Hold point:** firmware update method, Secure Boot state, ISO identity, and any
different storage/driver requirements must be recorded during this reproduction.
Do not introduce new drivers merely because this is a Surface.

## 2. Update Debian, identify the machine, and establish time

**D user-confirmed:** the original install was updated with this sequence:

```sh
hostname
cat /etc/os-release
uname -r
sudo apt update
sudo apt full-upgrade -y
sudo reboot
```

- [ ] Reconnect household Wi-Fi using Xfce's network menu; confirm it reconnects
  without prompting after reboot. Enter Wi-Fi credentials locally, not in this log.
- [ ] Confirm the intended hostname, Debian 13, native display, and touch again.
- [ ] Confirm internet/DNS access and the selected system timezone.
- [ ] **Establish and verify network time before accepting the clock.**

### 2.1 Install and verify OS network time

Run on the **new tablet**, after confirming its hostname. Inspect existing
services first; do not run competing time daemons:

```sh
timedatectl status
systemctl is-active systemd-timesyncd chrony ntpsec
```

Use the Debian 13 implementation verified on `bensclock`:

```sh
sudo apt update
sudo apt install systemd-timesyncd
sudo systemctl enable --now systemd-timesyncd
```

Verify both enablement and synchronization, allowing time for initial contact:

```sh
systemctl is-enabled systemd-timesyncd
systemctl is-active systemd-timesyncd
systemctl status systemd-timesyncd --no-pager
timedatectl status
timedatectl timesync-status
journalctl -u systemd-timesyncd -b --no-pager -n 50
```

Expected: `enabled`, `active`, `System clock synchronized: yes`, and
`NTP service: active`. Service activity alone does not prove synchronization.
For failure, inspect the journal and `timesync-status` for server/contact errors;
check internet connectivity, DNS, and outbound NTP (UDP 123). Inspect
`/etc/systemd/timesyncd.conf` and its drop-ins if server selection is unexpected.
Do not hard-code the reference's observed server address. Tailscale provides
administrative connectivity, not time synchronization.

### 2.2 Choose the deployment timezone

Every installation must explicitly select an IANA timezone for its location.
List available choices with `timedatectl list-timezones`. For the verified Chicago
example (choose the appropriate value for Ian's actual location):

```sh
sudo timedatectl set-timezone America/Chicago
timedatectl status
```

The system clock represents UTC; the OS timezone rules convert it to civil time,
including daylight saving changes. Keep the RTC in UTC (`RTC in local TZ: no`).
If a new device reports a local-time RTC, configure UTC and recheck:

```sh
sudo timedatectl set-local-rtc 0
```

Note: Denclock services has to be restarted after a timezone change.

Clock timezone and weather forecast location are independent. Forecast settings
do not set the OS timezone. Configure the timezone before starting the DenClock
server; if changing it on an already running new installation, restart
`denclock-server.service` and verify `/api/time` and the display. .NET caches the
local timezone; a successful OS change alone does not establish the running
server's timezone. If `timedatectl` reports a D-Bus activation timeout, inspect
`timedatectl status` again before assuming failure or repeating the change.

### 2.3 Time architecture and temporary network loss

Source review: [server endpoint](../server/Program.cs),
[browser clock](../client/clock.js), [timing configuration](../client/config.js),
and [server contract](../server/docs/contract.md).

`GET /api/time` samples `DateTimeOffset.UtcNow` and derives the current offset
from `TimeZoneInfo.Local`. It returns UTC, Unix milliseconds, timezone ID, and
UTC offset with `Cache-Control: no-store`. The browser polls every 60 seconds,
adds half the request round-trip duration, and advances the sample using
`performance.now()`. It formats the server's civil time independently of the
browser's timezone. Requests time out after five seconds; without a sample it
shows “Waiting for time”; after three minutes without a successful API sample
it shows “Sync delayed” while continuing to advance.

No application code changes or second NTP client are needed. OS synchronization
is the accuracy foundation; browser/server synchronization only distributes it.
The endpoint does not report OS synchronization health. If internet NTP is lost
but the local API still responds, the browser can remain apparently synchronized
while the OS clock drifts. “Sync delayed” is not an NTP-health indicator.

During temporary internet loss, the system clock continues running without a
fresh network correction. Accuracy can drift; after connectivity returns,
verify a fresh successful time-server contact. The browser receives any corrected
system time on a later successful poll. Startup is not gated on NTP success,
so an offline boot is not proof of accurate time. Do not add startup blocking
or application NTP logic without a demonstrated requirement.

The API supplies the offset at each sampled instant. DST updates therefore
reach the browser on the next successful poll (normally within one minute),
not necessarily exactly at the transition. If API access is unavailable across
a transition, the browser retains the old offset until it reconnects.

### 2.4 November 7 reference evidence and remaining tests

**D, supplied user verification:** the initial `bensclock` audit found
`System clock synchronized: no`, `NTP service: n/a`, and no active time daemon.
The user then installed/enabled `systemd-timesyncd` using section 2.1's commands
and set `America/Chicago`. Subsequent inspection showed:

```text
Time zone: America/Chicago (CST, -0600)
System clock synchronized: yes
NTP service: active
RTC in local TZ: no
systemd-timesyncd.service: enabled; active (running)
Status: Contacted time server [address omitted]:123
```

An initial timezone-command D-Bus activation timeout occurred; subsequent
inspection confirmed the change succeeded and synchronization remained operational.
These are supplied execution results, not a new live inspection by this task.
No evidence establishes post-install reboot synchronization, network-loss recovery,
an independent reference comparison, or a DST transition test on `bensclock`.

**Required on `iansclock`; all pending:**

- [ ] Record the chosen timezone and enabled/active service, synchronization status,
  contacted server and timestamp. Compare OS UTC (`date -u`), `/api/time`, and
  visible civil time with an independent trusted time reference; record the source,
  observed difference and acceptance judgment, not merely agreement between two
  DenClock displays sharing the same source.
- [ ] After the planned reboot in section 11, repeat section 2.1's checks. Require
  `System clock synchronized: yes` and evidence of successful contact during the
  current boot; record how long synchronization took and recheck the display.
- [ ] With local access available, temporarily disconnect the new tablet's network.
  Record outage duration, continuing clock display, and any drift. Restore access,
  confirm new successful NTP contact from diagnostics/journal, and compare OS/API/
  display against the independent reference again. API success alone is insufficient.
- [ ] For a timezone with DST, verify the correct offset and display across a real
  transition, or a controlled isolated test environment. Record before/after UTC,
  API offset and civil display, including the polling delay. Do not change the
  working reference's clock to simulate this. Until performed, record DST behavior
  as source-reviewed rather than hardware-tested.

Command/behavior references: Debian 13
[`timedatectl`](https://manpages.debian.org/trixie/systemd/timedatectl.1.en.html)
and
[`systemd-timesyncd`](https://manpages.debian.org/trixie/systemd-timesyncd/systemd-timesyncd.service.8.en.html).
These references support operational guidance, not claims that acceptance tests
were executed.

## 3. SSH, hostname resolution, and Tailscale

### 3.1 Local administration first

**D user-confirmed, O running:** OpenSSH is the administration mechanism.

On the **new tablet**:

```sh
sudo apt install openssh-server
sudo systemctl enable --now ssh
systemctl is-active ssh
hostname -I
sudo ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub
```

From the **development computer**, connect to the discovered LAN address using
the chosen user. Compare the host-key fingerprint locally before accepting it.
Then verify `hostname` and `uname -r` in the SSH session. Never transplant Ben's
SSH host keys, `known_hosts` entries, or passwords to Ian's device.

Use DHCP with working local DNS or a deliberate reservation as needed. The
reference LAN address is incidental, not a static-address requirement. For the
gateway's `iansclock.local`, install/verify Avahi if absent:

```sh
sudo apt install avahi-daemon
sudo systemctl enable --now avahi-daemon
systemctl is-active avahi-daemon
```

The Avahi installation command is an **adapted** standard package step; the
conversation directly verifies the reference daemon active and gateway hostname
resolution succeeding. Household multicast behavior still needs testing.

### 3.2 Private administration across households

**D user-confirmed:** Tailscale was installed on the tablet using its official
script, authenticated into the administrator's existing tailnet, and ordinary
OpenSSH from WSL worked through the Windows host's Tailscale connection.
**O:** tailscaled is enabled/active, backend Running, online, with no reported
health warning. This does not establish phone-hotspot or reboot recovery.

[Official Linux instructions](https://tailscale.com/docs/install/linux):

```sh
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up
tailscale ip -4
```

If `curl` is absent, install it with APT first; this prerequisite is an **adapted**
step, not a recovered original invocation. Authorize the one-time login URL using
the intended account on the admin computer. Do not save the URL, tokens, or keys
in the repository. Record Ian's new address privately and update `CLOCK_HOST`.

- [ ] Test ordinary SSH to Ian's Tailscale address from the actual WSL shell.
  A separate WSL Tailscale installation was proposed but unnecessary on the
  reference administration computer; do not install a second instance by default.
- [ ] Inspect the new device's key-expiry/access policy in the Tailscale admin UI.
  **O:** the reference key had an expiry date; disabling expiry was merely proposed,
  not observed. Decide an unattended reauthentication plan instead of assuming it
  is already configured.
- [ ] Test SSH from a different internet connection and after the final reboot.
  Leave router port forwarding out of the installation.

**Troubleshooting:** verify target/user, tailnet membership, host-key identity,
hostname resolution, and `systemctl is-active ssh tailscaled`. Ordinary OpenSSH
is used here; Tailscale SSH was not required. Never disable host-key verification
to bypass an address-change warning.

## 4. RustDesk independent recovery access

**O:** RustDesk 1.5.0 is installed, its root system service is enabled/active with
`ExecStart=/usr/bin/rustdesk --service`, and TCP 21118 is listening. Selected root
and desktop-user configuration both show `direct-server = 'Y'` (direct IP enabled).
**D user-confirmed:** public RustDesk infrastructure was used for existing remote
administration. A private RustDesk server and GitHub auto-updater were discussions,
not installed prerequisites.

The original RustDesk download/install command and unattended-password setup
transcript were not recovered. Use the official
[Linux package instructions](https://rustdesk.com/docs/en/client/linux/) and
[releases](https://github.com/rustdesk/rustdesk/releases) for an amd64 `.deb`.
Choose/record the actual version and artifact checksum; 1.5.0 is the observed
reference, not a claim that it is the latest upstream release.

- [ ] Download the chosen official package. Following vendor-supported syntax,
  install that local `.deb` with APT. Do not guess a release URL/filename or add
  Ubuntu PPAs to Debian. The package-manager invocation is vendor-documented,
  not a recovered bensclock execution transcript.

Use the actual verified download path in this vendor-supported manual command;
the path below is a placeholder to replace, not a recovered filename:

```sh
RUSTDESK_DEB='/absolute/path/to/the-verified-amd64-package.deb'
test -f "$RUSTDESK_DEB" && sudo apt install "$RUSTDESK_DEB"
```

- [ ] Open RustDesk locally in Xfce, unlock Security settings, and configure
  unattended access with a unique permanent password stored in the administrator's
  password manager. Do not copy Ben's RustDesk state/ID or include credentials here.
- [ ] Enable **direct IP access** using the Security setting documented in
  [RustDesk advanced settings](https://rustdesk.com/docs/en/self-host/client-configuration/advanced-settings/#direct-server).
  The documented default direct port is 21118. A configuration flag/listener does
  not prove an authenticated direct-IP session succeeds.
- [ ] Verify `systemctl is-active rustdesk` and `systemctl is-enabled rustdesk`.
  If package installation did not enable/start the service, use the vendor's
  service setup and record the correction; no missing startup command is invented.
- [ ] From the admin computer, connect to Ian's **Tailscale IP** in RustDesk,
  authenticate, and test display, keyboard, pointer/touch support, and clipboard
  only as needed. Test public-ID access separately if retained as a fallback.
- [ ] Test after reboot and on a phone hotspot/different network without local
  approval. These functional tests were not performed during the read-only audit.

**Unknowns:** original package source/checksum, exact unattended-access options,
and direct-IP/reboot/hotspot acceptance. Password values were deliberately not
inspected. Do not declare unattended readiness solely because the service runs.

## 5. Local XFCE/X11 login, keyboard, display, and battery

### 5.1 LightDM automatic login

**D user-confirmed, O exact file:** `/etc/lightdm/lightdm.conf.d/50-autologin.conf`.
Confirm `cat /etc/X11/default-display-manager` returns `/usr/sbin/lightdm`.
On the **new tablet**, create the directory and edit the file:

```sh
sudo mkdir -p /etc/lightdm/lightdm.conf.d
sudo nano /etc/lightdm/lightdm.conf.d/50-autologin.conf
```

```ini
[Seat:*]
autologin-user=ian
autologin-user-timeout=0
```

Replace `ian` with the actual chosen desktop account. This is the observed
reference configuration with the user changed. Automatic login makes the desktop
accessible to someone physically present; sudo/SSH passwords remain separate.
**Expected:** active local X11/XFCE session without typing after reboot.

### 5.2 Onboard and accessibility

**D user-confirmed:** install `onboard` with APT and enable it in Xfce Settings →
Session and Startup → Application Autostart. Avoid creating duplicate entries.
**O:** the reference runs the packaged Onboard autostart entry, with a user override
`~/.config/autostart/onboard-autostart.desktop` containing
`X-XFCE-Autostart-Override=true`; auto-show reads `true`.

```sh
sudo apt install onboard
```

Before Onboard first starts, enable desktop accessibility and auto-show as the
actual desktop user on their session bus (not with sudo):

```sh
gsettings set org.gnome.desktop.interface toolkit-accessibility true
gsettings set org.onboard.auto-show enabled true
```

This prerequisite was established during the Ian trial: enabling auto-show alone
caused the GNOME accessibility dialog and failed automatic keyboard appearance.
If accessibility was enabled after session startup, restart Onboard for a quick
retest and perform a planned logout/login or reboot for persistence acceptance.
The namespace is shared with GNOME; a GNOME desktop installation is unnecessary.
Launch/test it locally and verify actual text entry. Use the existing
[Onboard troubleshooting and accepted fix](clock-window-control.md#onboard-keyboard-accessibility).
The current repository launcher includes `--force-renderer-accessibility=complete`;
install that launcher in section 8 rather than recreating the old command line.

### 5.3 Screen lock and power policy

**D:** the original setup removed light-locker and disabled AC display sleep.
**O correction:** `dpkg-query` reports `light-locker` state `rc` (removed with
configuration remnants), and no process was found. A version printed by
`dpkg-query -W` alone does **not** mean it remains installed.

On a new tablet, check first:

```sh
pgrep -a -f 'xfce4-screensaver|xscreensaver|light-locker'
```

If light-locker is present, the recovered removal commands are:

```sh
sudo apt remove light-locker
pkill light-locker
```

`pkill` finding no process after removal is not a failed installation. In Xfce
Power Manager, set AC display blanking/sleep and system suspend to Never and
disable automatic screen locking. Decide battery suspend behavior deliberately.
**O reference explicit values:** `dpms-enabled=false`,
`lock-screen-suspend-hibernate=false`, `brightness-switch=0`,
`lid-action-on-ac=0`, `inactivity-on-battery=14`; other battery brightness/DPMS
values exist in the saved XML. Do not translate these numeric enums into a new
policy without checking the GUI. The older guide's 15/5-minute battery table was
a recommendation, not an exact observed configuration.

**O runtime:** X11 saver timeout is 0 and DPMS is disabled. The current launcher
sets `xset s off`, `xset s noblank`, and `xset -dpms`; Xfce also has DPMS disabled.
No explicit logind drop-ins were found in the inspected system directories.
Battery timers may consequently behave differently from the old recommendation;
validate AC and battery separately. Do not introduce another automatic brightness
controller alongside Clight.

### 5.4 Firmware charge limiting

**D:** original user correction identified a direct **Battery Limit** switch in
UEFI; there was no separate Kiosk Mode submenu visible on that device. Enable it
on the new device if using the same always-plugged-in policy and record the screen.
[Microsoft documents a 50% Battery Limit](https://learn.microsoft.com/en-us/surface/battery-limit).
**O:** reference battery was 49%, Not charging. That is consistent with 50% mode
but does not directly read/verify the firmware switch. A later conversation said
80%; the two claims conflict, so do not document 80% as the observed limit.
No writable Linux charge-threshold sysfs values were exposed in this inspection.

**Expected:** AC unattended operation with a verified charging policy. **Unknown:**
actual UEFI switch state, behavior on a full AC outage, discharge/suspend/recovery
policy, and any firmware-version differences. Do not change the working reference
to resolve these questions during documentation work.

## 6. Runtime, MQTT, and service-account prerequisites

### 6.1 .NET 10 runtime on the tablet

**D user-confirmed:** initial failure was missing `wget`; corrected installation
succeeded. **O:** both ASP.NET Core and base runtime are 10.0.14. Use supported
10.0 patch updates; the observed patch is not an instruction to downgrade.
See [Microsoft's Debian 13 instructions](https://learn.microsoft.com/en-us/dotnet/core/install/linux-debian?tabs=dotnet10).

```sh
sudo apt install wget
wget https://packages.microsoft.com/config/debian/13/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb
sudo apt update
sudo apt install aspnetcore-runtime-10.0
dotnet --list-runtimes
```

Expected: both `Microsoft.AspNetCore.App 10.0.x` and `Microsoft.NETCore.App 10.0.x`.
The tablet runs framework-dependent binaries; it does not need the SDK or a source
checkout. The development computer needs .NET 10 **SDK**, OpenSSH client, and rsync.
The original development SDK installation transcript is unavailable; verify
`dotnet --list-sdks` there and follow the vendor's matching platform instructions.

### 6.2 Local Mosquitto

**D user-confirmed pub/sub test; O active/enabled, local-only and persistent:**

```sh
sudo apt install mosquitto mosquitto-clients rsync
sudo systemctl enable --now mosquitto
systemctl is-active mosquitto
```

`rsync` is included here as a documented deployment prerequisite. Test MQTT in
two SSH terminals on the new tablet:

```sh
mosquitto_sub -h localhost -t 'denclock/test' -v
```

```sh
mosquitto_pub -h localhost -t 'denclock/test' -m 'Provisioning test'
```

The message text is an adaptation of the original verified test. Expected: the
subscriber receives it; stop the subscriber afterward. Reference broker listeners
are `127.0.0.1:1883` and `[::1]:1883`, with `persistence true` and
`persistence_location /var/lib/mosquitto/` in `/etc/mosquitto/mosquitto.conf`.
Confirm the new broker's effective configuration; do not expose MQTT for the gateway.
The gateway sends TCP to LoRa.Service on 5210, not MQTT and not USB serial.

### 6.3 Non-login backend account and persistent paths

**D recovered setup; O resulting account, paths, and unit users:**

```sh
sudo useradd --system --no-create-home --shell /usr/sbin/nologin denclock
sudo mkdir -p /var/lib/denclock/server /var/log/denclock
sudo chown -R denclock:denclock /var/lib/denclock /var/log/denclock
sudo chmod -R 755 /var/lib/denclock /var/log/denclock
```

If the account already exists, inspect it instead of creating another identity.
The desktop user owns deployed binaries; `denclock` needs read access there and
write access to the persistent data/log directories. Do not put permanent settings
or logs under a directory mirrored with `rsync --delete`.

## 7. Publish and install the applications — choose the correct target

**D current code:** both projects target net10.0, published framework-dependent
for linux-x64. Server publishing includes the browser's runtime assets under
`wwwroot`; client docs/tests and development files are excluded. No separate
Node installation or frontend build is needed on the tablet.

**Critical:** `deploy-to-bensclock.sh` hardcodes `REMOTE=ben@bensclock` and ownership
`ben`; `deploy-and-restart-bensclock.sh` also hardcodes its refresh target. They are
not generic provisioning scripts. Setting `CLOCK_REMOTE` or an environment
`REMOTE` does **not** retarget them. Do not alias `bensclock` to Ian's machine,
reuse `.bensclock-password`, or run these scripts for `iansclock`.

For Ian's first trial, use the following **manual adaptation** of the existing
publish/transfer operations. This is not a tested new deployment tool. It uses
ordinary SSH prompts and never reads Ben's private password file.

On the **development computer**, in the latest reviewed repository:

```sh
git status --short
git fetch origin
git log -1 --oneline origin/main
```

Review local changes before updating; when clean and on the intended branch,
`git merge --ff-only origin/main` is the documented research refresh operation.
If starting a new development checkout, clone the actual repository using your
authorized GitHub connection; no GitHub credential belongs on the tablet.

Publish both applications to a new empty staging directory. The explicit commands
are equivalent to the current script's two publishes:

```sh
PUBLISH_DIR=$(mktemp -d -t denclock-iansclock.XXXXXXXX)
dotnet publish server/DenClock.Server.csproj \
    --framework net10.0 --configuration Release --runtime linux-x64 \
    --self-contained false --output "$PUBLISH_DIR/server"
dotnet publish lora-service/LoRa.Service.csproj \
    --framework net10.0 --configuration Release --runtime linux-x64 \
    --self-contained false --output "$PUBLISH_DIR/lora-service"
```

Both must succeed before proceeding. Record `git rev-parse HEAD` and retain a
known-good publish artifact for recovery. Do not reuse old `publish/` output.
Recheck the chosen `CLOCK_USER`, `CLOCK_HOST`, and `CLOCK_REMOTE` from section 0:

```sh
ssh "$CLOCK_REMOTE" hostname
```

Require `iansclock`. Open an interactive SSH session to that target and run on
the **new tablet** (adapted from the current script, using its actual login user):

```sh
sudo mkdir -p /opt/denclock/server /opt/denclock/lora-service
sudo chown -R "$(id -un):$(id -gn)" /opt/denclock/server /opt/denclock/lora-service
test -w /opt/denclock/server
test -w /opt/denclock/lora-service
```

Back on the **development computer**:

```sh
rsync -rtp --delete --delay-updates -e ssh \
    "$PUBLISH_DIR/server/" "$CLOCK_REMOTE:/opt/denclock/server/"
rsync -rtp --delete --delay-updates -e ssh \
    "$PUBLISH_DIR/lora-service/" "$CLOCK_REMOTE:/opt/denclock/lora-service/"
```

These operations delete destination-only files inside those two application
directories. On an update, back up and stop the affected backend services first;
transfers are not atomic across both applications and there is no built-in rollback.
Inspect both results before starting/restarting anything. Use the
[deployment runbook](deploy-to-bensclock.md) for the implemented Ben-specific
workflow and its failure behavior; a generic multi-device tool remains future work.

### 7.1 System services

**O:** the following units match the recovered original service setup. On the
**new tablet**, use `sudo nano` to create the two named files with these contents.
These are common service definitions, not household credentials.

`/etc/systemd/system/denclock-server.service`:

```ini
[Unit]
Description=DenClock Web Server
Wants=network-online.target mosquitto.service
After=network-online.target mosquitto.service

[Service]
Type=simple
User=denclock
Group=denclock
WorkingDirectory=/opt/denclock/server
ExecStart=/usr/bin/dotnet /opt/denclock/server/DenClock.Server.dll
Environment=ASPNETCORE_ENVIRONMENT=Production
Environment=Presentation__ConfigurationDirectory=/var/lib/denclock/server
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

`/etc/systemd/system/denclock-lora.service`:

```ini
[Unit]
Description=DenClock LoRa Gateway Service
Wants=network-online.target mosquitto.service
After=network-online.target mosquitto.service

[Service]
Type=simple
User=denclock
Group=denclock
WorkingDirectory=/opt/denclock/lora-service
ExecStart=/usr/bin/dotnet /opt/denclock/lora-service/LoRa.Service.dll
Environment=Logging__FilePath=/var/log/denclock/lora-service-.log
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

```sh
sudo systemctl daemon-reload
sudo systemctl enable --now denclock-server denclock-lora
systemctl is-active denclock-server denclock-lora mosquitto
curl -fsS http://localhost:5000/api/time
curl -fsS http://localhost:5000/api/presentation
```

Expected: all three active, valid JSON responses, and browser page available.
For failure: `journalctl -u denclock-server -u denclock-lora -n 100 --no-pager`.

**D implementation/O sockets:** HTTP listens on `0.0.0.0:5000` because
`server/Program.cs` adds that URL explicitly, despite appsettings.json mentioning
localhost. LoRa listens on `0.0.0.0:5210`; MQTT is loopback-only. The browser window
helper in section 8 is loopback-only 5001. Choose household/tailnet firewall access
deliberately; no existing firewall policy was established by this inspection.

## 8. Install current desktop integration separately

**O:** launcher, Python helper, helper unit, and deployed `window-control.js`
match current GitHub by SHA-256. **D accepted stories 015/016:** fullscreen
restore and Onboard behavior were physically accepted, including reboot startup.
This task observed files/state but did not retest touch, window changes, or reboot.

Use [clock fullscreen control](clock-window-control.md) for detailed behavior and
recovery. The current launcher uses a fullscreen **app window**, not locked
`--kiosk`, so the upper-right rectangle can restore/minimize it for maintenance.
It restarts Chromium five seconds after exit and imports graphical environment
for the user helper. Closing the browser alone does not leave it stopped.

On the **new tablet**, install the documented desktop dependencies:

```sh
sudo apt install chromium python3 wmctrl x11-utils xdotool
```

The combined package list consolidates existing subsystem instructions. From the
**development computer**, this is the existing copy procedure adapted to Ian:

```sh
ssh "$CLOCK_REMOTE" 'mkdir -p ~/.local/lib/denclock ~/.local/bin ~/.config/systemd/user ~/.config/autostart'
scp tools/kiosk/window-control.py "$CLOCK_REMOTE:.local/lib/denclock/"
scp tools/kiosk/denclock-window-control.service "$CLOCK_REMOTE:.config/systemd/user/"
scp tools/kiosk/denclock-kiosk.sh "$CLOCK_REMOTE:.local/bin/denclock-kiosk.sh"
```

On the **tablet**, as its desktop user, without sudo:

```sh
chmod +x ~/.local/bin/denclock-kiosk.sh
systemctl --user daemon-reload
systemctl --user enable denclock-window-control.service
```

Create `~/.config/autostart/denclock-kiosk.desktop` (adapted account path):

```ini
[Desktop Entry]
Type=Application
Name=DenClock Kiosk
Exec=/home/ian/.local/bin/denclock-kiosk.sh
Terminal=false
X-GNOME-Autostart-enabled=true
```

Replace the absolute home path if needed. Desktop `Exec` does not expand `~` or
`$HOME`. Keep one launcher only. The user helper may not have a usable DISPLAY
when started from SSH; the local XFCE launcher imports it and starts the helper.
Apply the browser startup flags at the planned local-login/reboot test, rather
than launching another copy from SSH.

Expected: one DenClock window, accessible Onboard fields, functional restore/
fullscreen button. `--password-store=basic` in the dedicated profile avoids the
original keyring prompt; keep saved credentials out of that profile. If Onboard
fails only in web fields, check the accessibility flag before changing the desktop.

**Update gap:** normal app deployment does not transfer any of these three desktop
files or autostart entries. A Ctrl+Shift+R refresh does not apply changed Chromium
startup flags. Update desktop integration explicitly using its runbook.

## 9. Clight automatic hardware brightness

Follow the [verified Clight runbook](../client/docs/clight/Surface%20Go%203%20automatic%20brightness.md),
including its recovered dependencies, source builds, install commands, and evidence
labels. Do not duplicate or replace the calibrated configuration here.

- [ ] Build/install libmodule 5.0.1 → Clightd 5.7 → Clight 4.10.
- [ ] Install the exact `clight.conf.example` as the desktop user's
  `~/.config/clight.conf`; use its identical eleven-point AC/BATT arrays, 10-second
  polling, 2000-ms transitions, restore-on-exit, verbose logging, and disabled
  gamma/dimmer/DPMS/screen/keyboard modules.
- [ ] Install `start-clight.sh.example` as `~/.local/bin/start-clight.sh`, executable.
  Install `clight.desktop.example` as `~/.config/autostart/clight.desktop`, changing
  `/home/ben/` to the chosen account's actual home path.
- [ ] Preserve discovery by IIO device **name `als`**; `iio:device1` is not stable.
  The exact recovered launcher selects the first match and does not check sed's
  replacement success. Preserve this documented limitation for the trial.
- [ ] Test locally within XFCE and then after reboot. Do not run a second Clight
  instance from SSH, run it as root, or grant blanket Polkit permissions.

**O:** one `/usr/bin/clight` process, correct ALS, exact saved configuration,
and Capture policy `any=no`, `inactive=no`, `active=yes`. No custom Clight Polkit
rule filename was found in the inspected local rule directories. Successful
sensor capture under the local session is historically user-verified.
November 7 source/cache inspection additionally confirmed all six supplied
Clightd options disabled and `/usr`, `lib`, Release for each build:

| Component | Observed checked-out source revision |
|---|---|
| libmodule | `3f60063e98631ce3fd25f70428b67ef15025597f` |
| Clightd | `4a3dcde8109c9d789748fe1273593a74319c01c3` |
| Clight | `c304f4a774f5e5533b290fe31f48da3759e95300` |

A checked-out revision/cache is current configuration evidence, not a full install
transcript or proof of binary provenance. The runbook distinguishes the original
November 5 execution evidence from these later observations.

Expected: actual `intel_backlight` changes after illumination changes on both AC
and battery. For checks/logs and SSH authorization failures, use the runbook.

## 10. Household configuration and gateway/sensor integration

Use the local gear/settings UI, not copied Ben household files. See
[presentation configuration](../server/docs/presentation-configuration.md),
[client behavior](../client/README.md), and the
[gateway/sensor guide](../firmware/sensors/README.md).

- [ ] Set temperature unit, forecast city/state or ZIP, and verify the resolved
  location/forecast before saving. This requires external HTTPS/DNS. Location
  lookup/forecast requests run in the browser; there is no separate weather daemon
  or API key provisioned by the inspected code.
- [ ] Set the OS timezone independently: `/api/time` uses the server timezone;
  forecast labels use the forecast location's timezone. Validate both.
- [ ] Enter actual numeric gateway/node pairs, unique logical names, display labels,
  primary positions, and appropriate stale limits. Discovery is not implemented.
  **O reference:** gateway 1/nodes 1 and 2, upperLeft/upperRight, 1000-second stale
  limits, F units, a configured forecast location. These are Ben-specific choices.
- [ ] Confirm `/var/lib/denclock/server/presentation.json` is writable by `denclock`.
  On first start it is initialized from the bundled file, which contains example/
  installation-specific values; do not accept these as Ian's settings by default.
  Later deployments do not replace an existing persistent file. Save through the UI;
  external edits are not watched and invalid saved files fail service startup.
- [ ] Connect the gateway to Ian's Wi-Fi through its temporary setup AP as documented
  in the firmware guide. Credentials stay outside Git. Point its service hostname
  to `iansclock.local`; verify TCP 5210/mDNS rather than relying on Ben's hostname.
- [ ] Choose the installation's frequency/sync word and match all sensors. Ben's
  conversation records 915.500 MHz and D3C7, not a universal household assignment.
  Use the firmware guide's actual gateway and sensor console-entry procedures:
  the sensor requires `?` within ten seconds of reset. Do not rely on the earlier
  conversational suggestion that arbitrary first commands enter sensor config.
- [ ] Confirm gateway connection, current reports in LoRa.Service/MQTT, correctly
  assigned readings, battery icons, and stale-to-fresh recovery on the display.

Expected: independent Ian household configuration and sensor traffic. Unknowns:
Ian's intended location, labels, IDs, radio channel, Wi-Fi and timezone until chosen.
No need to attach the gateway's USB serial port permanently to the Surface;
runtime transport is Wi-Fi/TCP. Serial is for configuring firmware devices.

## 11. Functional acceptance — complete before unattended delivery

These tests are for the **new tablet**, with the installer present. They were not
executed on the working reference during this audit.

- [ ] Correct device identity and boot disk; touchscreen/native display work.
- [ ] Complete every time acceptance check in section 2.4: correct explicit timezone,
  enabled/active time service, synchronized status, independent reference comparison,
  reboot synchronization, temporary network-loss/recovery, and DST where applicable.
- [ ] Wi-Fi, SSH over Tailscale, and RustDesk direct-IP/unattended connection work
  from the development computer and an external/hotspot network.
- [ ] Three backend units active/enabled: Mosquitto, server, LoRa. Local APIs return
  JSON; visible clock matches the verified time; settings save/reload works.
- [ ] Forecast location/units correct; hourly/daily/radar views function and recover
  after network failure. No saved default household location accepted accidentally.
- [ ] Sensor labels/IDs correct; fresh readings and battery states visible; stale
  readings become red and recover on real new reports. Retained old reports do not
  become fresh merely after restart.
- [ ] One Chromium app window; fullscreen restore/return works; minimize permits
  desktop maintenance. Onboard opens for editable settings and hides appropriately.
- [ ] One Clight process, correct dynamically selected `als`, successful captures,
  visible dim/bright response on AC and battery, comfortable dark-room minimum.
- [ ] Remain on AC through an observation period longer than configured idle timers:
  no screen blank, lock prompt, sleep, keyring dialog, or remote-access loss.
- [ ] Confirm Battery Limit in UEFI and charging behavior; conduct a controlled
  charger disconnect/reconnect test and record battery sleep/wake decisions.
- [ ] Save settings, record current service state, and perform a planned reboot:

```sh
sudo reboot
```

- [ ] Without touching the tablet: LightDM autologin, Wi-Fi/Tailscale/RustDesk,
  backend units, browser, helper, Onboard, and Clight all recover. Settings persist.
  Confirm actual ALS captures/backlight changes, not just a running process.
- [ ] Repeat remote access and API checks after reboot; verify time synchronization
  again. Test planned application update/recovery with backups on Ian's device.
- [ ] Record cold-power/extended-outage behavior separately. A built-in battery means
  pulling AC is not equivalent to a complete power cycle. A reboot alone does not
  prove recovery after battery exhaustion or automatic power-on when AC returns.

Read-only checklist commands:

```sh
hostname
timedatectl status
systemctl is-active denclock-server denclock-lora mosquitto ssh tailscaled rustdesk
systemctl is-enabled denclock-server denclock-lora mosquitto ssh tailscaled rustdesk
curl -fsS http://localhost:5000/api/time
curl -fsS http://localhost:5000/api/presentation
pgrep -a -x clight
grep devname ~/.config/clight.conf
```

From a terminal in the local graphical session, also check
`systemctl --user is-active denclock-window-control.service` and `xset q`.
Clight and Onboard tests/diagnostics are in their linked subsystem runbooks.

## 12. Maintenance, backup, and recovery

### Keep three update layers distinct

| Layer | Current mechanism | Operational limit |
|---|---|---|
| Application/server/browser assets | Publish + SSH/rsync, explicit service restart and browser refresh | Existing scripts target Ben only; transfers are not atomic and no rollback is implemented |
| Desktop integration | Explicit launcher/helper/unit copy, user-manager reload, planned browser/session restart | Not covered by app deploy; refresh alone cannot apply browser flags |
| OS/runtime/remote admin | Deliberate package/firmware maintenance | No DenClock auto-updater exists; keep RustDesk/SSH independent for recovery |

For Ben, follow [existing deployment instructions](deploy-to-bensclock.md).
For Ian, repeat the reviewed manual sequence in section 7 using his verified
target, until a separately accepted generic deployment tool exists. Stop backend
services before replacing files, restart them only after both transfers succeed,
and verify active state and APIs before browser refresh. The helper/launcher have
their own [update/recovery instructions](clock-window-control.md).

### Backup before an update

- Preserve the exact known-good publish artifact and source commit for **both**
  backend services together. Record deployed DLL hashes; a current GitHub commit
  does not prove the running binaries came from it.
- Back up `/var/lib/denclock/server/presentation.json`, both system units, user
  autostart/launcher/helper files, and `~/.config/clight.conf` to private storage.
  Include needed broker persistence only with an appropriate consistent snapshot/
  stopped-broker backup procedure. No exact backup command was recovered/tested.
- Treat network and remote-admin recovery material as private. Do not dump
  NetworkManager secrets, Tailscale state, RustDesk passwords/keys, or browser
  credentials into repository records or generic diagnostic logs.
- Preserve original ownership/modes when restoring. Test the restore on the trial
  tablet; a backup file existing is not proof of recoverability.

### Diagnosis and broken-deployment recovery

1. Connect using independent SSH or RustDesk. Confirm `hostname` first.
2. Read `systemctl status` and `journalctl -u denclock-server -u denclock-lora`.
   Inspect disk space and permissions, `/var/lib/denclock/server`, runtime versions,
   local MQTT, and API responses. Redact household/private values before sharing logs.
3. If a deployment stopped partway, keep affected backends stopped until both
   known-good artifacts are restored/transferred. Re-running a corrected publish/
   transfer is the existing recovery path; there is no version-switch symlink or
   automatic rollback service in this implementation.
4. If settings are invalid, restore the private known-good persistent file while
   the server is stopped. Do not delete it to silently adopt bundled defaults.
5. Restart the chosen backend services on the trial tablet, verify APIs/settings,
   then refresh Chromium. If flags changed, use the desktop runbook's launcher
   restart procedure. Never kill random browser processes as a general repair.
6. If remote access itself fails, local keyboard/UEFI/media remain the recovery
   path. Record how the recipient can identify Wi-Fi/power problems without
   exposing admin credentials.

**Version provenance O limit:** deployed runtime configs target net10.0; installed
runtime is 10.0.14. The desktop files match the GitHub baseline. Deployed DLL hashes
were collected but no embedded source-commit manifest was established, so the
complete deployed application version cannot be asserted from GitHub HEAD alone.

## 13. Unresolved questions and documentation conflicts

| Priority | Question/conflict | Evidence and required resolution |
|---|---|---|
| P0 | Working internet time | November 7 user verification confirms timesyncd enabled/active, synchronized, and America/Chicago on Ben. Ian setup, reboot/recovery, independent comparison and DST acceptance remain pending (section 2.4). |
| P0 | Safe Ian deployment target | Existing scripts hardcode Ben and Ben ownership. Use the documented manual adaptation with target checks; generic deployment automation is future work. |
| P1 | RustDesk exact install and unattended setup | Package/service/direct-IP flag observed; original install/password setup not recovered. Record official artifact and prove direct-IP, external-network and reboot access. |
| P1 | Firmware/charge policy | Original guide/Microsoft say 50%, later user statement says 80%, current capacity 49%. Inspect new device's firmware setting; do not infer a precise configured threshold from capacity alone. |
| P1 | Actual power-outage recovery | Battery timers/suspend and startup after full exhaustion not tested. Reboot acceptance does not answer these. |
| P1 | Installer/Secure Boot/firmware details | Original ISO checksum/Rufus options, Secure Boot and firmware-update history not recovered; record during Ian trial. Standard Debian kernel and hardware response are established. |
| P1 | Package/driver inventory reproducibility | Relevant versions observed; not a full clean-install dependency proof. Record extra dependencies and original driver/firmware requirements if found on Ian. |
| P1 | Deployed application identity and rollback | No source-commit deployment manifest or automatic rollback. Keep publish artifacts and verify restored data/config compatibility. |
| P2 | Old locked kiosk instructions | Replaced by current `tools/kiosk/` app-window launcher and accepted stories 015/016. Do not recreate `--kiosk`. |
| P2 | Old brightness implementation record | `Automatic Brightness Control.md` is historical and lists already-finished startup/reboot/calibration work. Use current Clight runbook. |
| P2 | Old battery timers and light-locker package query | Recommended timer table was not actual state; `rc` means removed, not installed. Inspect effective GUI/runtime settings. |
| P2 | Source guide status of sensor integration | Early guide said pending; later conversation verifies gateway connection, and current persistent mappings contain two sensors. This audit did not force transmissions or accept Ian's data path. |
| P2 | Network boundaries/key expiry | Direct-IP listener is not limited to Tailscale; firewall policy not audited. Tailnet key has expiry. Decide household access/reauthentication policy and test it. |

## 14. `iansclock` installation observation log

Keep this section as the first reproduction record. Do not mark success before the
step has actually been performed. Store credentials and precise household details
outside Git; use references such as “password manager entry configured.”

| Date/time | Step/device | Expected | Observed/error | Correction and exact command/version | Evidence O/D/I | Retest/result |
|---|---|---|---|---|---|---|
| Nov 9 | Hardware/firmware/ISO | Compatible Surface Go 3; verified installer | Surface Go 3 and Secure Boot disabled supplied by user; exact ISO/firmware unrecorded | Existing Debian retained | D | Physical/firmware acceptance pending |
| Nov 9 | Debian/partitions/drivers | Native touch, Wi-Fi and display | Debian 13.7, LVM root, Wi-Fi, 1920×1280, ALS/backlight present | Updated kernel 6.12.116; no new Surface drivers | O/D | SSH/display mode verified; touch pending |
| Nov 9 | Identity/time/network | Ian identity, correct synchronized time | iansclock/ian, Eastern; initially unsynchronized | Installed timesyncd; synchronized after reboot in ~32 seconds; HTTPS Date within 0.81 s | O | Passed OS/API checks; display/outage/DST pending |
| Nov 9 | SSH/Tailscale/RustDesk | Independent remote access | SSH/Tailscale recovered after reboot; RustDesk service installed | Tailscale 1.104.5; RustDesk 1.5.1 verified package hash in audit | O/D | SSH passed; RustDesk authentication/external network pending |
| Nov 9 | Services/deployment/settings | Correct target and persistent settings | All backend units active/enabled; settings roundtrip and reboot persistence passed | Source/build hashes in installation manifest; neutral sensor list | O | MQTT and weather fetch passed; real sensors pending |
| Nov 9 | Desktop/Onboard/Clight | Touch editing, fullscreen and ALS | One fullscreen clock, helper, Onboard and Clight after reboot; captures/backlight updates | Existing assets and documented source builds; ALS rediscovered device2 | O | Process/API/ALS passed; touch and visible response pending |
| Nov 9 | Power/reboot/hotspot/recovery | Unattended recovery | Reboot recovery passed; DPMS disabled, saver timeout 0 | AC policies applied; no cold-outage test | O | Battery/UEFI/dwell/hotspot/restore pending |

Final sign-off fields: installer, date, hardware identity reference, installed source
commit/artifact hashes, accepted deviations, remaining limitations, backup location
reference, and reviewer acceptance. Do not place serial numbers or secrets in public
records unnecessarily.

## 15. Future automation opportunities — prioritized, not implemented

1. Explicit multi-device deployment target/account selection with an identity check,
   private credential handling, and rejection of accidental cross-device targets.
2. A common package/account/path/system-unit provisioning manifest, with validation
   of time synchronization and a per-device input record. Keep destructive disk
   installation separate from application provisioning.
3. Managed desktop integration: launcher, helper, user unit, autostart, Onboard and
   Clight assets, with correct user/session handling and version/hash verification.
4. Release/artifact provenance, consistent backup, health checks and tested rollback
   for both services and persistent configuration. Then design approved updates;
   the proposed GitHub polling/updater services do not currently exist.
5. Redacted diagnostics and a repeatable acceptance report, including external
   connectivity, time, sensors, power, and reboot tests with explicit human checks.
6. Repeatable installer/media/firmware records after the first successful Ian trial.
   Do not automate unresolved defaults merely because they appeared on Ben's tablet.

## 16. Sources inspected and access limits

### Repository and implementation

GitHub `main` and the clean WSL checkout were compared and fetched; both were at
the baseline commit above. Inspected root README, STATUS, AGENTS and development
workflow; this pre-existing Surface guide; deployment and window-control runbooks;
current Clight runbook/assets; hardware and historical brightness notes; client
README/contract/decisions and forecast/background behavior; server Program,
PresentationSettings/configuration guidance, appsettings, bundled presentation,
project publishing rules; LoRa Program/appsettings and gateway protocol/setup
guidance; both deployment scripts and all custom desktop launcher/helper/unit files.

Relevant accepted stories inspected include root 011–016 (deployment, forecast,
wrapper, password file, fullscreen, keyboard), client 020–022 (background and Clight),
and linked sensor/gateway configuration/provisioning acceptance. Some older
architecture/client descriptions still center the original Windows NUC; they do
not override the explicitly requested Linux deployment or the current source.

### Underlying conversations

- [Convert Surface Go 3 to Linux](chatgpt-conversation://6ae4cca6-a804-83ea-90dc-fc30251b9c56):
  read recent turns through the conversation tool and loaded older browser messages
  covering Live input/installer, partition choice, updates, Onboard, SSH/host keys,
  LightDM, .NET correction, deployment, power/MQTT, service setup and kiosk/keyring.
  User confirmations and pasted outputs were separated from assistant proposals.
- [Brightness Configuration Modes](chatgpt-conversation://6aecab94-7d78-83ea-b31d-4b7af1a18b19):
  underlying November 5 build/install and later reboot exchanges were recovered in
  the accepted Clight work in this same research chat; the current runbook carries
  those commands/evidence. Earlier discussion of 80% charging conflicts with other
  evidence and is not silently adopted.
- [RustDesk private server setup](chatgpt-conversation://6aef13f4-9e08-83e9-9167-fb9bd7415cbd):
  recent tool turns and older browser messages cover public-server use, existing
  administration, Tailscale installation/authentication, WSL connectivity, and
  proposed hotspot/updater work. Proposals were not promoted into installed services.
- [Virtual Keyboard Troubleshooting](chatgpt-conversation://6ae9e69c-4194-83e9-9283-e9e6ce74d251):
  recent tool turns verify gateway `.local`/TCP connection and the Ben-specific radio
  selection. Keyboard repair is supported more directly by accepted story 016,
  current launcher code, and the current live configuration.
- [Surface Go DenClock Present](chatgpt-conversation://6ad7f722-ced4-83e9-aea4-50be0d1fff3b):
  recent accessible content concerns gateway provisioning; it is not an original
  tablet-install transcript. Firmware source/runbook is used for actual procedures.

The conversation tool exposed bounded recent turns with no older cursor for these
ChatGPT chats; browser loading recovered substantial older setup history. This is
not a claim that every historical conversation or attachment was exhaustively read.
The exact original RustDesk install transcript and some firmware/media decisions
remain unavailable. No secrets or unavailable content were reconstructed.

### Live reference inspection

Read-only SSH over Tailscale using an existing private login file and the already
trusted `bensclock` host key. Observed OS/kernel/firmware identity, storage, selected
package versions, sessions, network listeners/service enablement, LightDM/user
autostart, launchers/user unit, brightness files/Capture policy/source caches,
power XML/X11 status, runtime configs, sanitized settings metadata, API status,
tailnet readiness, selected RustDesk options, and desktop hashes. Privileged reads
were restricted to selected nonsecret settings, effective SSH policy, rule names,
and logind overrides. No full credentials, private keys, tailnet state, RustDesk
passwords, or Wi-Fi connection secrets were collected.

Reference version snapshot: Debian 13.7, firmware 16.106.143, kernel
6.12.113+deb13-amd64, XFCE session 4.20.2, LightDM 1.32.0, Chromium
156.0.8078.73, Onboard 1.4.1, .NET/ASP.NET 10.0.14, Mosquitto 2.0.21,
Tailscale 1.104.5, RustDesk 1.5.0. These describe the observation, not a latest-version
claim or a requirement to pin every package. Installed/reachable does not mean
this task verified installation, uptime history, touch behavior, or reboot recovery.

### Vendor references checked

[Debian Live media](https://www.debian.org/CD/live/),
[Microsoft Debian/.NET installation](https://learn.microsoft.com/en-us/dotnet/core/install/linux-debian?tabs=dotnet10),
[Microsoft Battery Limit](https://learn.microsoft.com/en-us/surface/battery-limit),
[Tailscale Linux installation](https://tailscale.com/docs/install/linux),
[RustDesk Linux](https://rustdesk.com/docs/en/client/linux/), and
[RustDesk direct IP settings](https://rustdesk.com/docs/en/self-host/client-configuration/advanced-settings/#direct-server)
were checked against official documentation. No external source was used as proof
that a proposed command had actually been executed on either tablet.

## IansClock initial installation — November 9, 2026

The supplied installation account confirms Debian 13.7 (reported installer version
13.7.0), hostname iansclock, login ian, working Wi-Fi and SSH. Secure Boot was
disabled on this tablet and USB boot selected through Surface UEFI. This records
Ian's successful choice, not a universal requirement to disable Secure Boot.
The exact ISO/checksum and complete original installation conversation were not
supplied to this session; do not reconstruct them as verified history.

### Required access prerequisites

Ensure an Internet APT repository is configured before package installation.
Ian's initially missing repository was configured for Debian 13 Trixie as:

```text
deb http://deb.debian.org/debian trixie main contrib non-free-firmware
```

Run `apt update` as root. Install `openssh-server` and `sudo`, then run
`usermod -aG sudo ian` as root. Start/enable SSH and sign out/in to acquire the
sudo group. Verify SSH from the development computer and verify `hostname`
returns `iansclock` before configuration. These prerequisites are already
completed on this trial. Security/update repository coverage must be checked
separately; a working base repository alone does not establish that coverage.

### Troubleshooting and installation-specific observations

The installer requested `iwlwifi-cc-a0-77.ucode`; Wi-Fi ultimately worked without
manually supplying that file. Do not add drivers solely on this warning when
Wi-Fi is operational. Record any future actual failure and required firmware.
For USB boot failures, check UEFI USB boot selection and Secure Boot; Ian disabled
Secure Boot successfully. Reinstalling Debian or repartitioning is unnecessary.

The live SSH inspection confirmed Debian 13.7, LightDM, ian's sudo membership,
working SSH, America/New_York, a UTC RTC, and no existing /opt/denclock. Network
time was not synchronized and no time daemon was active. Root uses LVM with
about 41 GiB free. These observations do not prove touchscreen or reboot recovery.

The user approved automatic desktop login and disabling screen locking, and
selected Eastern time for now (America/New_York). Forecast location remains
unconfirmed. Follow [installation status](installations/iansclock-status.json)
and [audit](installations/iansclock-installation.md) for execution and tests.

### Complete APT update coverage (Ian trial correction)

A base Trixie line alone allowed packages to install but omitted security and
stable updates. Inspect all sources before adding anything; avoid duplicate suites.
Ian retained his base source and added `denclock-updates.sources` with:

```text
Types: deb
URIs: https://deb.debian.org/debian
Suites: trixie-updates
Components: main contrib non-free-firmware
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg

Types: deb
URIs: https://security.debian.org/debian-security
Suites: trixie-security
Components: main contrib non-free-firmware
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg
```

Run `sudo apt-get update`, inspect `sudo apt-get -s full-upgrade`, and apply with
`sudo apt-get -y --no-remove full-upgrade` when no destructive changes are proposed.
The no-remove guard stops an unexpected removal. This follows Debian's
[official Trixie APT source guidance](https://www.debian.org/releases/trixie/release-notes/upgrading.en.html#adding-apt-internet-sources).
For a clean install, a single deb822 file covering base, updates and security is
also supported by Debian; do not add another base entry alongside an existing one.

### First-start health check correction

Enabling/starting a unit can return before HTTP binds. After verifying unit states,
use `curl -fsS --retry 10 --retry-connrefused --retry-delay 1 http://localhost:5000/api/time`.
A failed immediate connection alone does not establish application failure. Retain
real errors and inspect the journal if retries do not succeed.

### Ian trial user confirmation after reboot

The user confirmed automatic clock launch and visible weather, and independently
confirmed SSH over Tailscale. RustDesk functional acceptance remains unverified:
the response confirmed SSH rather than a RustDesk session. Visible time correctness
and remaining physical/power/sensor acceptance are still pending in the status record.

### Ian trial RustDesk authentication follow-up

RustDesk connected after local approval while password authentication initially
failed. Revisit Security settings and configure a permanent RustDesk password;
the Debian/SSH password is separate. After this guidance, the user confirmed
RustDesk over Tailscale works. Separate post-reboot unattended and external-network
acceptance remains pending in the installation record.

### Ian trial correction: initialize accessibility before Onboard

Section 5.2 also requires enabling `org.gnome.desktop.interface toolkit-accessibility`
before enabling `org.onboard.auto-show enabled`, as the actual desktop user on their
session bus. The original trial omitted that prerequisite and produced an Onboard
GNOME accessibility dialog with nonworking automatic keyboard appearance. See the
[fresh-XFCE prerequisite and verification](clock-window-control.md#fresh-xfce-installation-prerequisite-iansclock-trial).
The retained Ian configuration script now sets both in the required order.
The Chromium accessibility flag was already installed and its accessible web
content was observable; removing that flag or installing GNOME is not the repair.

Ian keyboard correction acceptance: after restarting Onboard the user confirmed
keyboard appearance; after a planned reboot the user confirmed the accessibility
dialog stays gone and the keyboard appears and accepts text in client settings.
The two-setting initialization in section 5.2 and retained script is therefore
validated on this fresh Surface Go 3 installation. Other pending acceptance checks
remain tracked separately.
