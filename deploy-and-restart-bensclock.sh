#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

bash "$SCRIPT_DIR/deploy-to-bensclock.sh" \
    --restart-unit denclock-server.service \
    --restart-unit denclock-lora.service

echo "Hard-refreshing the Chromium kiosk on bensclock..."
password_runner=()
if [[ -e "$SCRIPT_DIR/.bensclock-password" ]]; then
    password_runner=(sshpass -f "$SCRIPT_DIR/.bensclock-password")
fi
"${password_runner[@]}" ssh ben@bensclock 'set -eu
command -v xdotool >/dev/null || { echo "Install xdotool on bensclock: sudo apt-get install xdotool" >&2; exit 1; }
DISPLAY=:0 xdotool search --onlyvisible --class chromium windowactivate --sync key --clearmodifiers ctrl+shift+r'
echo "Deployment, service restarts, and kiosk refresh complete."
