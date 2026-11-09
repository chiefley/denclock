# IansClock installation audit

Date: November 9, 2026. Target: iansclock only. Story 020 was accepted by the developer on November 9, 2026. No commits or BensClock access performed.

## Initial inspection

Commands (over SSH to the existing trusted host): `hostname`, `cat /etc/os-release`,
`date -u`, `timedatectl status`, `id`, `df -h /`,
`cat /etc/X11/default-display-manager`, selected `systemctl is-active` and
`dpkg-query` checks, `cat /etc/apt/sources.list`, `loginctl list-sessions`, `sudo -v`.
Result: Debian 13.7; kernel 6.12.107+deb13-amd64; LightDM active; ian in sudo;
53 GiB root / 41 GiB free; no /opt/denclock; no active time daemon or application
services. Chromium, Onboard and rsync require installation; light-locker installed.
Existing APT base line: trixie main contrib non-free-firmware.
Timezone America/New_York, UTC RTC, unsynchronized system clock.

User approved autologin and disabling locking; Eastern time retained. Forecast
location, sensor assignments, remote-access enrollment and physical tests pending.
Credentials are kept out of repository files and output records.

## Installation commands and results

- `sudo apt-get update` and `sudo apt-get install -y systemd-timesyncd chromium
  onboard python3 wmctrl x11-utils xdotool wget mosquitto mosquitto-clients rsync
  avahi-daemon`: successful (18 new packages).
- `sudo systemctl enable --now systemd-timesyncd mosquitto avahi-daemon`:
  successful. NTP synchronized after an approximately five-hour forward correction;
  prior clock timestamps are unreliable. `timedatectl timesync-status` encountered
  the documented D-Bus activation timeout; `timedatectl status` and service status
  independently confirmed synchronization and successful pool-server contact.
- `sudo apt-get -s remove light-locker`: only light-locker removal proposed;
  `sudo apt-get remove -y light-locker`: successful, user-approved locking change.
- Created `/etc/lightdm/lightdm.conf.d/50-autologin.conf` from guide section 5.1,
  substituting ian (user approved); reboot verification pending.
- Downloaded official Microsoft Debian 13 package-repository bootstrap with curl,
  `sudo dpkg -i /tmp/iansclock-microsoft-prod.deb`, `sudo apt-get update`, installed
  `aspnetcore-runtime-10.0` and the Clight runbook build dependencies. Both runtime
  families verified at 10.0.14. Third-party installation explicitly approved.
- Downloaded Tailscale official installer to /tmp and executed with sudo; successful,
  version 1.104.5. Enrollment and remote connectivity tests remain pending.
- Published both projects with the guide's Release/net10.0/linux-x64/framework-
  dependent arguments to fresh `artifacts/iansclock/{server,lora-service}`.
  Both successful. SHA-256 manifest: `iansclock-artifacts.json`.
- Transferred `artifacts/iansclock-install.tar.gz` using SCP with strict host-key
  checking. Extracted in `/home/ian/denclock-install` and ran `configure.sh`.
  Exact adaptation retained in `iansclock-configure.sh`; existing account/files
  guarded, no `rsync --delete` needed for a fresh empty application directory.
  Backend units copied exactly from guide; desktop assets from repository with
  ian home path. User helper enabled; server, LoRa and Mosquitto active.
  Immediate HTTP probe raced service readiness (connection refused); follow-up
  probes performed separately. Retained script corrected to retry connection refusal.
- Initialized a new persistent presentation file with Fahrenheit and empty gateway/
  sensor lists, avoiding bundled Ben-specific labels/IDs. Weather remains unconfirmed.
- Sysfs confirms built-in `als` at iio:device1 and intel_backlight; dynamic discovery
  preserved, no new drivers or permission grants installed.

Official package guidance checked against Microsoft Debian/.NET documentation,
Tailscale Linux installation and RustDesk Linux documentation. Debian security
repository coverage still requires correction/verification (base source only).

## Brightness, updates and remote-access installation

Executed the Clight runbook's release-tag clones and exact Release /usr + lib
CMake options, builds with `-j 2`, `sudo cmake --install` and `sudo ldconfig` for:

- libmodule 5.0.1, commit 3f60063e98631ce3fd25f70428b67ef15025597f;
- Clightd 5.7, commit 4a3dcde8109c9d789748fe1273593a74319c01c3 (six optional
  modules disabled exactly as documented);
- Clight 4.10, commit c304f4a774f5e5533b290fe31f48da3759e95300.

All succeeded. `pkg-config --modversion libmodule` returned 5.0.1; `clight
--version` returned 4.10-c304f4a. Configuration-stage/build logs retained on target
as `/tmp/iansclock-{libmodule,clightd,clight}-{config,build}.log`. These are temporary;
copy sanitized diagnostics into this record if needed before cleanup/reinstallation.
No custom Polkit grant or SSH-launched Clight controller used.

RustDesk official GitHub release API identified 1.5.1 and normal x86_64 .deb.
Downloaded `https://github.com/rustdesk/rustdesk/releases/download/1.5.1/rustdesk-1.5.1-x86_64.deb`;
SHA-256 verified against published asset digest:
`b5d682ad90cb7571ecbdbf26eef58d3c4514e726a74ef6d08ce3f0b114ef0bd9`.
`sudo apt-get install -y /tmp/rustdesk-1.5.1-x86_64.deb` succeeded; service active.
Package's initial attempt to stop a not-yet-installed service was harmless.
Unattended password/direct-IP setup and functional connection are not yet verified.

Added `/etc/apt/sources.list.d/denclock-updates.sources` with trixie-updates from
https://deb.debian.org/debian and trixie-security from
https://security.debian.org/debian-security; components main/contrib/non-free-firmware,
Signed-By Debian archive keyring. Preserved existing base source and third-party
sources. This corrects missing update coverage rather than replacing the OS.
`sudo apt-get update` succeeded. `sudo apt-get -s full-upgrade` proposed 255
upgrades, one new kernel package, zero removals; execution uses
`sudo apt-get -y --no-remove full-upgrade` and target-local log
`/tmp/iansclock-upgrade.log`. Result/reboot recorded below after verification.

Onboard auto-show enabled with `gsettings set org.onboard.auto-show enabled true`
on ian's existing session bus. No change to desktop capture authorization.
User confirmed Eastern time and default Granger Township weather for this trial.

## Automated checks

From WSL: `PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools/kiosk/tests`
passed 4 tests. `node --test client/tests/*.test.js` passed 62 tests, zero failures.
These checks do not establish physical touch, brightness, weather rendering,
lock/suspend dwell, reboot or outage acceptance.

## Live functional probes before reboot

- `tailscale status --json` filtered to BackendState=Running, SelfOnline=true,
  Health=[]; user confirmed authentication. WSL ordinary SSH to Ian's Tailscale
  address with `StrictHostKeyChecking=yes` and `HostKeyAlias` set to the existing
  trusted LAN identity returned hostname `iansclock`. No Ben connection was made.
- `timeout 12 mosquitto_sub -h localhost -t denclock/test -C 1`, followed by
  `mosquitto_pub -h localhost -t denclock/test -m 'Provisioning test'`: exact message
  received. No retained test report or fabricated sensor observation published.
- Python urllib fetched current presentation's location and requested Open-Meteo
  forecast on iansclock: 240 hourly entries, 10 daily entries. No coordinates logged.
  This establishes provider connectivity/data, not browser rendering or radar.
- `ss -lnt`: MQTT loopback-only 1883, application 5000 and gateway TCP 5210 on all
  IPv4 interfaces, matching guide. No direct RustDesk listener yet configured.
- Applied documented XFCE booleans DPMS=false and lock-screen-suspend-hibernate=false,
  AC inactivity=0 and lid-action-on-ac=0. Readback confirms values; Onboard auto-show
  reads true. Battery suspend policy remains unchosen and requires physical check.

## Update completion and planned reboot

`sudo apt-get -y --no-remove full-upgrade` exited 0. New kernel
6.12.116+deb13-amd64 installed, initramfs generated; no disk/space failures.
`sudo dpkg --audit` produced no findings. All seven units enabled: DenClock server,
LoRa, Mosquitto, SSH, Tailscale, RustDesk, systemd-timesyncd.
`pkill -x light-locker` stopped the old session process left after package removal.
Native display checked with `xrandr --current`: 1920×1280 at 60 Hz.
Settings API GET / PUT of the same revision/configuration returned HTTP 200;
readback matched exactly. This persists the user-approved default location without
introducing sensor mappings. `sudo reboot` issued; SSH disconnected as expected.
Post-reboot results remain pending until reconnected and checked.

## Post-reboot verification (November 9, approximately 10:16–10:18 Eastern)

Early reconnection attempts timed out while boot/network were still starting;
both LAN and Tailscale TCP/22 subsequently returned. Reconnected over Tailscale
with the original trusted host key. `uname -r`: 6.12.116+deb13-amd64.

- `systemctl is-active denclock-server denclock-lora mosquitto ssh tailscaled
  rustdesk systemd-timesyncd`: all seven active after reboot, with no manual start.
- `timedatectl status`: America/New_York, UTC RTC, synchronized=yes, NTP active.
  Current-boot journal records initial synchronization at 10:15:42, roughly
  32 seconds after timesyncd startup. A separate HTTPS Date header from
  `https://www.google.com/generate_204` differed by 0.81 seconds at request midpoint,
  RTT 0.21 seconds, one-second header resolution. Appropriate coarse clock check;
  not a precise time calibration or a visible-display comparison.
- `wmctrl -lx`: exactly one localhost.DenClock app window. `xprop -id` for that
  window confirms fullscreen/focused. `xprop -name DenClock` initially failed to
  match despite the window existing; ID-based inspection resolves that diagnostic.
- `systemctl --user is-active denclock-window-control`: active. GET /window with
  the allowed Origin returns `{"fullscreen":true}`. An origin-less probe returned
  HTTP 403 as designed. No access-policy bypass applied.
- One launcher and Chromium main process, expected app/fullscreen/accessibility
  flags and dedicated profile. One Onboard process and one `/usr/bin/clight`.
- Dynamic ALS moved from iio:device1 to iio:device2 at reboot. User config selected
  device2 automatically; Clight log shows repeated 5/5 captures, ten-second polling,
  and intel_backlight updates (example ambient 0.530 -> backlight 0.202).
- `xset q`: saver timeout 0, DPMS disabled. No light-locker process after reboot.
- Settings persisted: F, zero sensor mappings, approved Granger Township forecast.
  Root page HTTP 200 and time API valid with UTC offset -18000.
- Deployed DenClock.Server.dll and LoRa.Service.dll SHA-256 values exactly match
  the retained fresh-build manifest. `sudo dpkg --audit` is clean; current-boot
  error-priority journals for server and LoRa show no entries.

## Remaining acceptance and decisions

These are pending, not passed or silently waived:

1. User's visible clock/weather confirmation; independent visible time comparison.
2. Physical touchscreen, fullscreen restore/return/minimize, Onboard show/edit/hide,
   hourly/daily/radar rendering and actual network-loss/recovery tests.
3. RustDesk permanent password and Direct IP settings, authenticated session,
   keyboard/pointer/display checks, post-reboot and external/hotspot access. User
   asked to configure password privately; service installation alone is insufficient.
4. Fresh real sensor reports, numeric identities/labels/radio settings, battery icons,
   stale-to-fresh tests. No sensors mapped or synthetic reports presented as real.
5. Visible dim/bright response, comfortable dark-room level, AC/battery comparison.
6. Long AC idle dwell, battery suspend policy, UEFI Battery Limit observation,
   charger disconnect/reconnect, true battery-exhaustion/cold-start behavior.
7. DST transition in isolation/real transition; source behavior only reviewed so far.
8. Application backup/restore and interrupted-update recovery exercise. Build
   artifacts and hashes retained in WSL artifacts/iansclock; no rollback test claimed.
9. Exact installer ISO/checksum, firmware settings/version and original full
   installation conversation unavailable; supplied installation account recorded.
10. Human review/acceptance. Story 020 remains active; no staging/commit/push.

No BensClock access or modification occurred. All source/document edits are in
`/home/chief/src/denclock`; synced project reference files were not modified.

## User confirmation following reboot

The user confirmed that the clock opens automatically and weather is visible.
The user also confirmed SSH over Tailscale works. The response to the RustDesk
question confirmed SSH, so it does not establish RustDesk password/direct-IP
configuration or an authenticated RustDesk display/keyboard/pointer session.
Displayed time correctness was not explicitly confirmed in the response.
Physical touch/keyboard/brightness and other pending acceptance checks remain open.

## RustDesk user validation

Initial password authentication failed, but accepting the connection locally on
IansClock allowed a working remote session. This established connectivity while
leaving permanent-password authentication unresolved. After guidance to revisit
Security settings, set a private permanent password, and retry without local
approval, the user confirmed: "yes, rustdesk over tailscale is working."
Record this as user-confirmed RustDesk-over-Tailscale operation. No password was
requested or recorded. A separate post-reboot unattended connection and an
external-network/hotspot test remain pending; they were not explicitly reported.

## Onboard acceptance failure and correction

User confirms RustDesk over Tailscale works after reboot. User reports an Onboard
GNOME accessibility startup dialog and no virtual keyboard in client settings,
both locally and through RustDesk. This invalidates keyboard acceptance; earlier
process/startup observations never established keyboard function.

Read-only Ian inspection: Onboard running; auto-show=true; toolkit-accessibility=true
(after user clicked OK); AT-SPI bus/registry active; Chromium has the expected
accessibility flag and exposes a web document, buttons and an entry via AT-SPI.
Onboard's installed source checks toolkit-accessibility and produces the exact
reported dialog when false; it recommends a fresh login after enabling it.
Initial provisioning omitted explicit toolkit-accessibility initialization.
Corrected retained configuration script and runbooks to set that prerequisite
before auto-show. Reasserted both true on Ian and restarted only the identified
Onboard process for current-session retest; no GNOME installation, new permissions,
Ben inspection or browser/application changes. Functional retest pending.

Current-session retest: user confirmed "yes the keyboard now appears" after
Onboard restart. Both GSettings keys read true. Recovery log contains no reported
GNOME accessibility dialog; optional mousetweaks/acpid warnings are not keyboard
startup failures. Text entry was not explicitly confirmed. A planned reboot was
issued to verify persistence and absence of the first-start dialog; retest pending.

Repair reboot acceptance: user answered "yes, it all works mow" to the explicit
check for no accessibility dialog, keyboard appearance on field selection, and
actual text entry after reboot. Record all three as user-verified. The repair is
complete for this reported keyboard defect. Earlier keyboard-pending entries
above describe historical checkpoints; this result supersedes them. Fullscreen
restore/return/minimize, remaining sensors/power/outage checks and story-level
human acceptance remain pending. RustDesk reboot operation is also user-confirmed.
No BensClock access, commits or staging were performed.

## Story closeout

The developer explicitly accepted the story on November 9, 2026. Story 020 is
moved to `docs/stories/completed/020-complete-iansclock-installation.md`.
Earlier active/pending-acceptance statements above are historical checkpoints.
Remaining unperformed checks are retained in the machine-readable status as
follow-ups; acceptance does not convert them into successful test results.
Documentation and script corrections are prepared for the developer's commit.
No commit or push was performed by the assistant.
