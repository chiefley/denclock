# DenClock — Surface Go 3 Installation and Deployment Guide

**Reference platform:** Microsoft Surface Go 3, 4 GB RAM, 64 GB eMMC  
**Operating system:** Debian 13 (Trixie), XFCE  
**Application platform:** .NET 10  
**Services:** DenClock.Server, LoRa.Service, Mosquitto  
**Display:** Chromium kiosk mode  
**Administration:** SSH from WSL Ubuntu

## 1. Overview

This guide describes installing DenClock on a Microsoft Surface Go 3, replacing Windows with Debian Linux, and configuring unattended operation.

The completed system automatically:

1. Boots Debian Linux.
2. Starts Mosquitto and both .NET services.
3. Logs into XFCE without user intervention.
4. Launches Chromium in kiosk mode.
5. Displays the DenClock application.
6. Restarts failed backend services automatically.

The system can be administered remotely using SSH. No physical keyboard or mouse is required during normal operation.

## 2. Install Debian 13

### Prepare the USB installer

Download the Debian 13 amd64 Live XFCE ISO from:

https://www.debian.org/CD/live/

Use Rufus on a Windows PC to write the ISO image to a USB flash drive. A 16 GB drive is sufficient.

**Important:** Copying an ISO file onto a USB drive does not make it bootable. Rufus must write the image.

### Configure Surface UEFI

1. Shut down the Surface.
2. Hold Volume Up and press Power to enter Surface UEFI.
3. Enable USB boot and move USB storage ahead of Windows Boot Manager.
4. If the installer will not boot, disable Secure Boot.
5. Save the settings.

To boot directly from USB, insert the prepared drive, hold Volume Down, and press Power.

### Test Debian Live

Choose **Live System** from the Debian boot menu.

Before installing, verify:

- Display resolution and graphics
- Touchscreen operation
- Wi-Fi detection and connectivity
- USB keyboard operation

The reference Surface Go 3 worked with the standard Debian kernel, without requiring the linux-surface kernel.

XFCE Live did not provide an immediately accessible on-screen keyboard. A USB-C hub and physical keyboard resolved the initial setup problem.

### Install Debian

Launch the graphical installer from the Live desktop.

Select the internal storage device. On the reference 64 GB Surface, it appeared as approximately **58.24 GB**.

Choose to erase the internal disk and install Debian with XFCE.

**Warning:** This permanently removes Windows and existing data.

After installation, remove the USB drive and boot Debian.

### Update Debian

```bash
sudo apt update
sudo apt full-upgrade -y
sudo reboot
```

## 3. Set up remote administration

Install OpenSSH:

```bash
sudo apt install openssh-server
sudo systemctl enable --now ssh
```

Determine the IP address:

```bash
hostname -I
```

From WSL:

```bash
ssh ben@bensclock
```

For future installations, replace `ben` and `bensclock` with the actual username and hostname.

Verify the SSH server fingerprint on the Surface:

```bash
sudo ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub
```

Compare that fingerprint with the one displayed by the SSH client before accepting the connection.

For reliable hostname resolution, configure local DNS or a DHCP reservation as needed.

Do not expose SSH directly to the Internet.

## 4. Enable automatic login

Debian XFCE uses LightDM on the reference installation.

Verify:

```bash
cat /etc/X11/default-display-manager
```

Expected:

```text
/usr/sbin/lightdm
```

Create the configuration directory:

```bash
sudo mkdir -p /etc/lightdm/lightdm.conf.d
```

Create the automatic login configuration:

```bash
sudo nano /etc/lightdm/lightdm.conf.d/50-autologin.conf
```

Contents:

```ini
[Seat:*]
autologin-user=ben
autologin-user-timeout=0
```

Automatic login removes the need for keyboard input during boot. The account password remains available for SSH and `sudo`.

**Security:** Anyone with physical access to the Surface can reach the logged-in desktop.

## 5. Configure the virtual keyboard

Install Onboard:

```bash
sudo apt install onboard
```

Test:

```bash
onboard
```

To start Onboard automatically, open:

**Applications → Settings → Session and Startup → Application Autostart**

Add an entry named `Onboard` with the command:

```text
onboard
```

Because LightDM automatically logs in, Onboard starts without requiring a password.

## 6. Configure power management

### Disable screen locking

The reference installation used `light-locker`, not a separately configured XFCE screensaver.

Check:

```bash
pgrep -a -f 'xfce4-screensaver|xscreensaver|light-locker'
```

If `light-locker` is installed, remove it:

```bash
sudo apt remove light-locker
```

Stop the current instance:

```bash
pkill light-locker
```

### Display power policy

Open **Applications → Settings → Power Manager**.

Recommended settings:

| Setting | AC power | Battery |
|---|---|---|
| System suspend | Never | 15 minutes |
| Display blanking | Never | 5 minutes |
| Display sleep | Never | 5 minutes |
| Automatic screen lock | Disabled | Disabled |

These are recommended settings, not a record of every setting verified on the reference installation.

The kiosk startup script also disables X11 screen blanking and DPMS.

### Surface battery charge limit

Enter Surface UEFI by holding Volume Up during startup.

Enable the **Battery Limit** switch.

On the Surface Go 3, this limits charging to approximately 50%, reducing battery stress during continuous AC operation.

The setting is firmware-controlled and remains effective under Debian.

## 7. Install .NET 10

Install the download utility:

```bash
sudo apt install wget
```

Configure Microsoft's Debian 13 repository:

```bash
wget https://packages.microsoft.com/config/debian/13/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb
```

Install the ASP.NET Core runtime, which includes the base .NET runtime:

```bash
sudo apt update
sudo apt install aspnetcore-runtime-10.0
```

Verify:

```bash
dotnet --list-runtimes
```

Both `Microsoft.NETCore.App` and `Microsoft.AspNetCore.App` should be present at version 10.

The SDK is not needed on the Surface because applications are published from the WSL development computer.

## 8. Install Mosquitto MQTT

```bash
sudo apt install mosquitto mosquitto-clients
sudo systemctl enable --now mosquitto
```

Verify:

```bash
systemctl is-active mosquitto
```

Test with two SSH terminals.

Subscriber:

```bash
mosquitto_sub -h localhost -t 'denclock/test' -v
```

Publisher:

```bash
mosquitto_pub -h localhost -t 'denclock/test' -m 'Hello from bensclock'
```

The subscriber should receive the message.

Both .NET applications use `localhost:1883` by default.

The reference architecture keeps MQTT local to the Surface. The Heltec gateway connects to LoRa.Service over Wi-Fi using TCP port 5210; it does not connect to Mosquitto directly.

For the initial installation, keep Mosquitto accessible only on localhost. Check that broker persistence is enabled if retained readings must survive broker restarts.

## 9. Deploy the applications from WSL

The repository contains:

```text
deploy-to-bensclock.sh
```

It publishes both applications for `linux-x64`, framework-dependent, and transfers them using SSH and rsync.

### Prerequisites

On the WSL development system:

```bash
sudo apt install rsync
```

Ensure the .NET 10 SDK and OpenSSH client are also available.

On the Surface:

```bash
sudo apt install rsync
```

### Run deployment

From WSL:

```bash
~/src/denclock/deploy-to-bensclock.sh
```

The script:

- Builds fresh Release versions of both applications.
- Creates the destination directories.
- Transfers the files with `rsync`.
- Deletes obsolete files within the destination directories.
- Does not restart services unless requested.

Installed application locations:

```text
/opt/denclock/server/
/opt/denclock/lora-service/
```

**Important:** The script uses `rsync --delete`. Persistent settings and logs must not be stored exclusively in those deployment directories.

## 10. Configure the service account

Create a dedicated service account:

```bash
sudo useradd --system --no-create-home --shell /usr/sbin/nologin denclock
```

Create persistent data and logging directories:

```bash
sudo mkdir -p /var/lib/denclock/server
sudo mkdir -p /var/log/denclock

sudo chown -R denclock:denclock /var/lib/denclock /var/log/denclock
sudo chmod -R 755 /var/lib/denclock /var/log/denclock
```

The deployment script maintains the application binaries under `/opt/denclock` using the administrative account.

The dedicated service account needs read access to the deployed files and write access to the persistent directories.

## 11. Configure the systemd services

### DenClock.Server

Create:

```text
/etc/systemd/system/denclock-server.service
```

Contents:

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

The `Presentation__ConfigurationDirectory` setting is required on Linux by the existing application code.

The writable settings file is:

```text
/var/lib/denclock/server/presentation.json
```

The file included in the published application directory provides initial defaults if the persistent file does not yet exist.

### LoRa.Service

Create:

```text
/etc/systemd/system/denclock-lora.service
```

Contents:

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

### Enable and start

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now denclock-server denclock-lora
```

Check:

```bash
systemctl is-active denclock-server denclock-lora mosquitto
```

All three should report:

```text
active
active
active
```

Troubleshoot startup failures with:

```bash
journalctl -u denclock-server -u denclock-lora -n 100 --no-pager
```

### Application verification

```bash
curl http://localhost:5000/api/time
```

This should return a JSON time response.

Open the DenClock display at:

```text
http://localhost:5000
```

Verify the clock interface and presentation configuration page.

## 12. Configure Chromium kiosk mode

Install Chromium if necessary:

```bash
sudo apt install chromium
```

Perform the following steps as the desktop login user (`ben` in the reference installation), **not with sudo**.

Create the directories:

```bash
mkdir -p ~/.local/bin ~/.config/autostart
```

Create:

```text
~/.local/bin/denclock-kiosk.sh
```

Contents:

```bash
#!/bin/bash

# Prevent X11 screen blanking.
xset s off
xset s noblank
xset -dpms

# Relaunch Chromium if it exits.
while true; do
    chromium \
        --kiosk \
        --no-first-run \
        --noerrdialogs \
        --disable-session-crashed-bubble \
        --password-store=basic \
        --user-data-dir="$HOME/.config/denclock-chromium" \
        http://localhost:5000

    sleep 5
done
```

Make executable:

```bash
chmod +x ~/.local/bin/denclock-kiosk.sh
```

Create:

```text
~/.config/autostart/denclock-kiosk.desktop
```

Contents:

```ini
[Desktop Entry]
Type=Application
Name=DenClock Kiosk
Exec=/home/ben/.local/bin/denclock-kiosk.sh
Terminal=false
X-GNOME-Autostart-enabled=true
```

Change the `Exec` path if the account name differs.

**Important:** The `--password-store=basic` Chromium option resolved a GNOME Keyring password prompt encountered after automatic login. It is appropriate here because the dedicated kiosk profile should not contain saved credentials.

## 13. Reboot acceptance test

Reboot:

```bash
sudo reboot
```

Verify the following without touching the Surface:

- [ ] Debian boots successfully.
- [ ] LightDM automatically logs in.
- [ ] No keyring password dialog appears.
- [ ] Chromium opens in full-screen kiosk mode.
- [ ] DenClock is displayed at `localhost:5000`.
- [ ] The display remains illuminated and unlocked.
- [ ] SSH is available remotely.
- [ ] Mosquitto and both .NET services are active.
- [ ] The configuration page works.
- [ ] Existing presentation settings survive reboot.

The reference installation passed the automatic kiosk startup test after adding `--password-store=basic`. Sensor reception was not yet tested because no sensors were connected.

## 14. Updating DenClock

Once the `systemd` units are installed, run this from WSL:

```bash
cd ~/src/denclock

./deploy-to-bensclock.sh \
    --restart-unit denclock-server.service \
    --restart-unit denclock-lora.service
```

The script builds and transfers both applications, then restarts the units.

It does not provide atomic deployment or rollback. Do not deploy while expecting uninterrupted service.

Application settings in `/var/lib/denclock/server` and LoRa logs in `/var/log/denclock` remain separate from the deployment directories.

## 15. Remaining work

The initial installation is functional, but the following items remain:

- Bring up the Heltec LoRa-to-Wi-Fi gateway.
- Connect temperature and humidity sensor nodes.
- Verify TCP data reception on port 5210.
- Verify LoRa.Service publishes actual sensor readings to MQTT.
- Verify DenClock.Server consumes the MQTT messages.
- Test retained data after service and broker restarts.
- Consider reducing display brightness overnight.
- Verify recovery after actual AC power interruption.

---

**Status:** Reference installation operational as a standalone clock and configuration display. Sensor integration pending.
