#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBLISH_DIR="/tmp/lora-server-publish"
DEPLOY_DIR="/mnt/denclock/LoRaServer"
#DEPLOY_DIR="/mnt/c/temp/lora-service"

echo "Publishing Lora.Server..."
rm -rf "$PUBLISH_DIR"

dotnet publish "$SCRIPT_DIR/LoRa.Service.csproj" \
    -c Release \
    -r win-x64 \
    --self-contained false \
    -o "$PUBLISH_DIR"

echo "Deploying to DENCLOCK..."
rsync -rt --delete \
    "$PUBLISH_DIR/" \
    "$DEPLOY_DIR/"

echo "Deployment complete."
