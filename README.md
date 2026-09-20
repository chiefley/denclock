# DenClock

DenClock is a smart clock and weather display for the den.

The project grew out of frustration with dedicated Internet-connected
clock and weather displays that proved unreliable. The initial goal is
deliberately simple: build a reliable display that obtains accurate time
from the Internet and presents it on a dedicated touchscreen.

The system will be expanded incrementally as the design develops.

## Goals

- Reliable Internet-synchronized clock
- Dedicated always-on touchscreen display
- Local and Internet weather information
- Remote environmental sensors
- LoRa communication with remote sensors
- Simple, maintainable hardware and software

## Architecture

DenClock is expected to consist of:

- Server — background service running on the NUC
- Client — touchscreen clock/weather display
- LoRa gateway — interface between remote LoRa devices and the server
- Sensor nodes — remote temperature/humidity and other sensors

The architecture is intentionally being developed incrementally.
Components and abstractions will be added when experience demonstrates
that they are needed.

## Start or resume here

Read [project status](STATUS.md), then the relevant component's README and
active story. See [development conventions](docs/DEVELOPMENT.md) for document
roles and human acceptance of stories.

## Repository

- [Server](server/README.md) — implemented ASP.NET Core time API and deployment instructions.
- [Client](client/README.md) — browser time display, run instructions, and accepted story.
- [Sensors and diagnostic gateway](firmware/sensors/README.md) — accepted LoRa environmental reporting firmware.
- `firmware/lora-gateway/` — empty scaffold for the future LoRa/Wi-Fi gateway.
- [Architecture](docs/ARCHITECTURE.md) — cross-component responsibilities and rationale.
- [Hardware](docs/HARDWARE.md) — NUC, display, and recorded network address.

The initial milestone is a working Internet-synchronized time display.
LoRa environmental reporting is implemented and accepted on the sensor/gateway
bench path. Weather display and integration with the DenClock server remain future work.
