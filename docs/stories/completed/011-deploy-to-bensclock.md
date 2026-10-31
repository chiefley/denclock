# Deploy both services to bensclock

## Request
Create a reusable root deploy-to-bensclock.sh in the existing WSL repository.
Publish DenClock.Server and LoRa.Service for .NET 10/linux-x64 and transfer via
SSH to ben@bensclock under /opt/denclock/server and /opt/denclock/lora-service.

## Requirements
- Inspect existing conventions; retain existing Windows helpers.
- Create remote directories and permissions, allowing sudo password prompts.
- Publish fresh output rather than copying stale repository output.
- Fail on publish, setup, transfer, or requested restart errors.
- Keep restarts optional until systemd units exist.
- Document prerequisites, mirroring, persistent data and Linux configuration.
- Verify publishing and orchestration without deploying to the real host.

## Acceptance
The developer verified and accepted this story. The reusable script publishes
both Linux services, prepares remote permissions, and mirrors clean output with
optional explicit systemd restarts. Both real publishes and simulated transfer,
failure, restart and cleanup checks passed. No intentional departures were
reported.
