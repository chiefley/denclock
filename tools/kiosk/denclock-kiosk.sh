#!/usr/bin/env bash
set -euo pipefail

# This launcher runs within the logged-in XFCE/X11 desktop session.
xset s off
xset s noblank
xset -dpms
systemctl --user import-environment DISPLAY
if [[ -v XAUTHORITY ]]; then
    systemctl --user import-environment XAUTHORITY
fi
systemctl --user restart denclock-window-control.service

while true; do
    chromium \
        --app=http://localhost:5000 \
        --class=DenClock \
        --start-fullscreen \
        --force-renderer-accessibility=complete \
        --window-size=900,500 \
        --window-position=32,32 \
        --no-first-run \
        --noerrdialogs \
        --disable-session-crashed-bubble \
        --password-store=basic \
        --user-data-dir="$HOME/.config/denclock-chromium" || true
    sleep 5
done
