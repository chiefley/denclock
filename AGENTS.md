# DenClock Development Instructions

## Project

DenClock is a dedicated clock and weather display for the den.

The project is being developed incrementally. There is substantial
discovery ahead, so avoid speculative architecture and premature
abstraction. Implement the simplest solution that satisfies the current
requirement, and refactor when experience demonstrates a need.

## Target Platform

DenClock runs on Windows on a dedicated Beelink Mini 8 NUC.

Development may be performed from other environments, including WSL,
but production software must run natively on Windows unless explicitly
decided otherwise.

Do not introduce Linux-specific runtime dependencies, services, paths,
shell scripts, or assumptions into the DenClock applications.

## Initial Milestone

The first milestone is a useful, working Internet-synchronized clock.

The initial system uses a server/client architecture:

- The server runs natively on Windows on the DenClock NUC.
- Windows maintains the system clock using its normal network time
  synchronization.
- The DenClock server provides time information to the client.
- The client runs on the same NUC and provides the full-screen
  touchscreen display.
- The client should continue displaying time smoothly between server
  updates.

Weather, LoRa communication, remote sensors, and other features are
future extensions. Do not implement infrastructure for them until it
is needed.

## Repository Structure

- `server/` - DenClock server application
- `client/` - DenClock display application
- `firmware/` - Embedded firmware
- `docs/` - Project documentation

Firmware currently anticipates:

- `firmware/lora-gateway/`
- `firmware/sensor-node/`

Do not create additional directory hierarchies merely to anticipate
possible future requirements.

## Development Philosophy

- Prefer simple, readable implementations.
- Avoid premature abstraction.
- Avoid adding frameworks or dependencies without a concrete benefit.
- Do not implement anticipated features unless requested.
- Keep server and client responsibilities clearly separated.
- Make incremental changes that leave the project in a working state.
- Preserve existing behavior when extending functionality.
- Update documentation when an architectural decision changes.

## Working With This Repository

Before making substantial changes:

1. Read `README.md`.
2. Read relevant files under `docs/`.
3. Inspect the existing implementation before proposing new structure.
4. Follow established project conventions where they exist.

When requirements are ambiguous, favor the smallest implementation that
can be easily changed as the design evolves.

## Development And Deployment Environment

Development host: your XPS 14, VS Code/WSL
Deployment target: Beelink NUC, Windows, 1024×600 touchscreen