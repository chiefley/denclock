#!/bin/bash
# First-install adaptation of master guide sections 6–9. Ian only.
set -euo pipefail
[[ $(hostname) == iansclock && $(id -un) == ian ]] || exit 1
cd /home/ian/denclock-install
sudo -v
if ! getent passwd denclock >/dev/null; then
 sudo useradd --system --no-create-home --shell /usr/sbin/nologin denclock
fi
sudo install -d -o denclock -g denclock -m 755 /var/lib/denclock/server /var/log/denclock
sudo install -d -o ian -g ian -m 755 /opt/denclock/server /opt/denclock/lora-service
# Fresh installation only: refuse replacing an existing installed application.
[[ ! -e /opt/denclock/server/DenClock.Server.dll && ! -e /opt/denclock/lora-service/LoRa.Service.dll ]] || { echo 'Already installed: inspect before updating'; exit 1; }
rsync -rtp server/ /opt/denclock/server/
rsync -rtp lora-service/ /opt/denclock/lora-service/
# Never initialize with another household's sensor identities.
if [[ ! -e /var/lib/denclock/server/presentation.json ]]; then
 printf '%s\n' '{"temperatureUnit":"F","gateways":[],"sensors":[]}' | sudo tee /var/lib/denclock/server/presentation.json >/dev/null
 sudo chown denclock:denclock /var/lib/denclock/server/presentation.json
fi
sudo install -m 644 denclock-server.service denclock-lora.service /etc/systemd/system/
mkdir -p ~/.local/lib/denclock ~/.local/bin ~/.config/systemd/user ~/.config/autostart
install -m 644 desktop/window-control.py ~/.local/lib/denclock/
install -m 644 desktop/denclock-window-control.service ~/.config/systemd/user/
install -m 755 desktop/denclock-kiosk.sh ~/.local/bin/
install -m 755 desktop/start-clight.sh ~/.local/bin/
install -m 644 desktop/clight.conf ~/.config/clight.conf
install -m 644 desktop/clight.desktop ~/.config/autostart/
cat > ~/.config/autostart/denclock-kiosk.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=DenClock Kiosk
Exec=/home/ian/.local/bin/denclock-kiosk.sh
Terminal=false
X-GNOME-Autostart-enabled=true
EOF
# Set accessibility BEFORE enabling Onboard auto-show or its first graphical start.
# Run as the desktop user on that user's session bus, never with sudo.
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u)/bus"
gsettings set org.gnome.desktop.interface toolkit-accessibility true
gsettings set org.onboard.auto-show enabled true
cp /etc/xdg/autostart/onboard-autostart.desktop ~/.config/autostart/
printf '\nX-XFCE-Autostart-Override=true\n' >> ~/.config/autostart/onboard-autostart.desktop
systemctl --user daemon-reload
systemctl --user enable denclock-window-control.service
sudo systemctl daemon-reload
sudo systemctl enable --now denclock-server denclock-lora
systemctl is-active denclock-server denclock-lora mosquitto
curl -fsS --retry 10 --retry-connrefused --retry-delay 1 http://localhost:5000/api/time
