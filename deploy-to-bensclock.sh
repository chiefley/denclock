#!/usr/bin/env bash
set -Eeuo pipefail

usage() {
    cat <<'HELP'
Usage: ./deploy-to-bensclock.sh [--restart-unit NAME.service ...]
Publish both .NET 10 services for linux-x64 and mirror them to ben@bensclock.
No services are restarted unless --restart-unit is supplied (repeat for both).
HELP
}

restart_units=()
while (( $# )); do
    case "$1" in
        --restart-unit)
            if (( $# < 2 )) || [[ ! "$2" =~ ^[a-zA-Z0-9][a-zA-Z0-9_.@-]*\.service$ ]]; then
                echo "Expected a systemd NAME.service after --restart-unit." >&2
                exit 2
            fi
            restart_units+=("$2")
            shift 2
            ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

for tool in dotnet ssh rsync mktemp; do
    command -v "$tool" >/dev/null || { echo "Required command missing: $tool" >&2; exit 1; }
done

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REMOTE="ben@bensclock"
PUBLISH_DIR="$(mktemp -d -t denclock-bensclock.XXXXXXXX)"
cleanup() { rm -rf -- "$PUBLISH_DIR"; }
trap cleanup EXIT
trap 'echo "Deployment failed at line $LINENO; remote files may be partially updated. No further steps were run." >&2' ERR

# Finish both publishes before making any remote changes. Never reuse publish/.
for component in server lora-service; do
    case "$component" in
        server) project="DenClock.Server.csproj" ;;
        lora-service) project="LoRa.Service.csproj" ;;
    esac
    echo "Publishing $component for linux-x64..."
    dotnet publish "$SCRIPT_DIR/$component/$project" \
        --framework net10.0 --configuration Release --runtime linux-x64 \
        --self-contained false --output "$PUBLISH_DIR/$component"
done

echo "Preparing directories on $REMOTE (SSH and sudo may prompt for passwords)..."
# Force a terminal so sudo can prompt; do not pipe passwords or use sudo -S.
ssh -t "$REMOTE" 'set -eu; command -v rsync >/dev/null; sudo mkdir -p /opt/denclock/server /opt/denclock/lora-service; sudo chown -R ben:$(id -gn ben) /opt/denclock/server /opt/denclock/lora-service; test -w /opt/denclock/server; test -w /opt/denclock/lora-service'

for component in server lora-service; do
    echo "Deploying $component..."
    # Mirror the complete publish output, including dotfiles and executable bits.
    # Destination-only files are deleted; keep persistent settings/logs elsewhere.
    rsync -rtp --delete --delay-updates -e ssh \
        "$PUBLISH_DIR/$component/" "$REMOTE:/opt/denclock/$component/"
done

if (( ${#restart_units[@]} )); then
    # Unit names are restricted above so they are safe in the remote command.
    printf -v units ' %s' "${restart_units[@]}"
    ssh -t "$REMOTE" "sudo systemctl restart --$units && systemctl is-active --$units"
else
    echo "Files deployed. No systemd services were restarted."
fi
echo "Deployment complete."
