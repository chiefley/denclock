#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLISH_DIR="/tmp/denclock-server-publish"
DEPLOY_DIR="/mnt/denclock/DenClockServer"

echo "Publishing DenClock.Server..."
rm -rf "$PUBLISH_DIR"

dotnet publish "$SCRIPT_DIR/DenClock.Server.csproj" \
    -c Release \
    -r win-x64 \
    --self-contained false \
    -o "$PUBLISH_DIR"

echo "Deploying to DENCLOCK..."
rsync -rt --delete \
    "$PUBLISH_DIR/" \
    "$DEPLOY_DIR/"

echo "Deployment complete."
