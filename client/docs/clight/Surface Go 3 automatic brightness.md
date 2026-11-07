# Surface Go 3 automatic brightness

This records the completed Debian 13 / XFCE installation on the Surface Go 3
known in the source conversation as `bensclock`. Brightness is controlled by
Clight and the hardware ALS, independently of the DenClock browser application.
The Windows NUC documentation continues to describe that separate deployment.
The configuration, launcher, and desktop entry were retrieved through read-only
SSH access to the tablet. No live files were changed.

## Evidence and limits

Source: [Brightness Configuration Modes](chatgpt-conversation://6aecab94-7d78-83ea-b31d-4b7af1a18b19),
retrieved for this documentation. The November 5, 2026 build/install exchanges
were recovered through the browser on November 7; the conversation tool's recent
preview alone did not expose them. Later exchanges include the user's post-reboot
output and “i verified it is working.”
The user subsequently recovered the exact working configuration from `bensclock`
and verified it on Debian 13, Surface Go 3, including automatic operation after
reboot. [clight.conf.example](clight.conf.example) now preserves those settings.
The startup script and desktop entry were subsequently recovered directly from
`bensclock` and are preserved as examples alongside the configuration. The build
commands and their supporting evidence are recorded below. No tablet access or
installation commands were needed for this command-recovery update.

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
   ALS/backlight. Kernel version, Surface-specific drivers,
   and XFCE power-manager settings from the successful installation are unknown.
2. Follow the [recovered November 5 build/install sequence](#recovered-november-5-buildinstall-sequence).
   Preserve the component versions and distinguish the evidence levels below.
   Building/installing software is separate from launching the user controller;
   install the proven configuration before starting Clight.
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

The source-build sequence is recovered, but a fresh-device reproduction is still
untested. The commands assume the existing Debian/XFCE hardware setup and working
APT sources; they do not establish the missing kernel/driver or power-manager setup.

## Recovered November 5 build/install sequence

These commands come from the November 5 conversation, not a newly tested recipe.
Run them on the intended Debian tablet when reproducing an installation, not on
the administration laptop. The original work used SSH for building and inspection.
The user controller must subsequently run in the local XFCE session.

Evidence terminology: **output-verified** means the user pasted relevant terminal
output; **user-reported** means the user confirmed success without the full output;
**proposed** means the assistant supplied instructions without direct execution
evidence for that individual command. Later installed artifacts establish results,
but do not prove that every preceding command was executed exactly as written.
No commands in this section were executed during this documentation update.

### 1. Build dependencies

The tablet's `/etc/os-release` output identified Debian 13 (Trixie), and
`apt-cache policy clight clightd` reported both packages unavailable in its
configured repositories. The following initial dependency installation was
**user-reported** successful: “yep. that last stuff seemed to run fine.”

```sh
sudo apt update
sudo apt install \
    git build-essential cmake pkg-config \
    libsystemd-dev libpopt-dev libconfig-dev \
    libgsl-dev libdbus-1-dev
```

The additional Clightd dependencies below were **proposed**. No separate APT
transcript or explicit confirmation was pasted for this command; the subsequent
Clightd configure and compile stages succeeded.

```sh
sudo apt install \
    libudev-dev \
    libjpeg-dev \
    libpolkit-gobject-1-dev
```

These are the recovered package lists, not a claim of a minimal dependency set
for a clean Debian installation.

### 2. libmodule 5.0.1

The download/configure/compile sequence was supplied as one block. The user
then confirmed “yes, the build succeeded” (**user-reported**).

```sh
mkdir -p ~/src
cd ~/src

git clone --branch 5.0.1 --depth 1 \
    https://github.com/FedeDP/libmodule.git

cmake -S libmodule -B libmodule/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib

cmake --build libmodule/build -j 2
```

The next instructions were:

```sh
cd ~/src

sudo cmake --install libmodule/build
sudo ldconfig

pkg-config --modversion libmodule
```

The user's reply was `5.0.1`, verifying the reported pkg-config result. The
individual install and linker-cache commands were not echoed in a full transcript.

### 3. Clightd 5.7

The recovered configuration instructions were:

```sh
cd ~/src

git clone --branch 5.7 --depth 1 \
    https://github.com/FedeDP/Clightd.git

cmake -S Clightd -B Clightd/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DENABLE_DDC=0 \
    -DENABLE_GAMMA=0 \
    -DENABLE_DPMS=0 \
    -DENABLE_SCREEN=0 \
    -DENABLE_YOCTOLIGHT=0 \
    -DENABLE_PIPEWIRE=0

cmake --build ~/src/Clightd/build -j 2
```

Configuration and compilation are **output-verified**: the user pasted
`Configuring done`, `Generating done`, the build directory
`/home/ben/src/Clightd/build`, and `[100%] Built target clightd`. The six flags
above are the supplied command line; no CMake cache or complete configure log
was pasted proving that each option was recognized and applied.

Installation instructions were:

```sh
sudo cmake --install ~/src/Clightd/build
sudo ldconfig
```

The install transcript was not pasted, but the resulting executable and D-Bus
activation file were **output-verified** using:

```sh
sudo find /usr /usr/local -type f -name 'clightd' 2>/dev/null
ls -l /usr/share/dbus-1/system-services/*clight*
cat /usr/share/dbus-1/system-services/org.clightd.clightd.service
```

The executable was `/usr/libexec/clightd`, and the activation file contained:

```ini
[D-BUS Service]
Name=org.clightd.clightd
Exec=/usr/libexec/clightd
User=root
```

`command -v clightd` and `clightd --version` failed because the executable was
outside the shell's search path. `systemctl status clightd.service` returned
`Unit clightd.service could not be found.` These were unsuccessful diagnostic
checks, not missing installation steps. This installation uses D-Bus activation;
no manual systemd unit creation or `systemctl enable` command was established.
Later Clight logs found Clightd version `5.7-4a3dcde` and successful local-session
sensor captures established working communication. They do not independently
record exactly when the daemon was activated.

### 4. Clight 4.10

The clone is **output-verified**, including release commit
`c304f4a774f5e5533b290fe31f48da3759e95300`:

```sh
cd ~/src

git clone --branch 4.10 --depth 1 \
    https://github.com/FedeDP/Clight.git
```

The assistant first supplied relative `Clight` / `Clight/build` paths from
`~/src`, then repeated the following equivalent absolute-home paths after the
clone. Compilation is **output-verified** by `[100%] Built target clight`.
There is no separate pasted CMake configure transcript for this component.

```sh
cmake -S ~/src/Clight -B ~/src/Clight/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib

cmake --build ~/src/Clight/build -j 2
```

The recovered install and verification instructions were:

```sh
sudo cmake --install ~/src/Clight/build
sudo ldconfig

command -v clight
clight --version
```

The verification commands and results are **output-verified**: `/usr/bin/clight`
and version `4.10-c304f4a`. The user also listed `/etc/clight/clight.conf` and
`/etc/clight/modules.conf.d/`, with installed module files. As with Clightd,
the individual install/ldconfig commands have no pasted execution transcript.
The version check emitted deprecated `/etc/default/clight.conf` and missing-user-
config warnings; later inspection confirmed the new system config existed and
the deprecated path did not. This was before creating the user configuration.

Proceed with the proven configuration and startup assets in the installation
procedure above. The original 5–60% test curve was subsequently adjusted to the
current eleven-point curve; do not restore the initial calibration.

### Proposals that were not the verified installation

- `sudo apt install clight clightd` was conditional on packages being available;
  the tablet reported them unavailable, and the source-build path was followed.
- Building Debian packages was suggested, but no packaging commands or `.deb`
  installation were recorded. The recovered instructions install with CMake.
- `clight -v` was initially proposed for verbose operation, but the user's output
  showed it prints the version. Verbosity was enabled in the configuration.
- A custom Polkit grant was considered, but not established as necessary or
  implemented. The active local XFCE session succeeded with existing authorization.
- A 30-second production interval was suggested. The recovered working config
  and successful reboot log retain 10-second polling.

Remaining evidence limits: no complete APT transaction logs, CMake caches,
installation manifests, or fresh-device test were supplied. The libmodule full
commit and Clightd full commit were not pasted; their release tags and Clightd's
short runtime revision were recovered. Do not promote proposed commands or
successful artifacts into a claim that every original shell invocation is known.

## Session authorization: local XFCE versus SSH

The user corrected an earlier diagnosis: the successful manual Bash launch was
in a terminal directly on the tablet. A later SSH-launched process discovered
the sensor but received `Operation not permitted`. That SSH process had an
unset `DISPLAY` and belonged to a separate login-session scope. Its failure did
not establish that XFCE autostart would fail; the subsequent reboot succeeded.

Launch Clight in the authorized local graphical session. SSH is useful for
reading process/config/log state after login, but an SSH launch does not test
the same session's Polkit authorization. Do not work around this with root
execution, permission changes, or blanket Polkit grants.

The recovered November 5 terminal output directly verifies the Capture policy:

```sh
pkaction --verbose \
    --action-id org.clightd.clightd.Capture
```

```text
implicit any:      no
implicit inactive: no
implicit active:   yes
```

`pkaction | grep -i clight` also listed the Capture, GetClient, Lower/LowerAll,
Raise/RaiseAll, Set/SetAll, and SetTimeout actions. A verbose Set-action check was
proposed, but its output was not pasted. The record establishes Capture defaults
and successful local-session operation, not a complete inventory of installed
policy files or local rules. No custom rule is part of the recovered procedure.

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
