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

## Repository

- `server/` — ASP.NET Core time API; see [run and deployment instructions](server/README.md).
- `firmware/` — embedded firmware work.
- `docs/` — architecture and hardware documentation.

The display client will be added in a later increment.

## Hardware

See docs/HARDWARE.md.

## Status

NUC and Touchscreen are in place.
Windows 11 running on the NUC set for autologon.
Remote access to the NUC is established through RustDesk
NUC name is DenClock

## Scope 

DenClock 0.1 provides a reliable, always-on Internet-synchronized time display using a server/client architecture. Weather, LoRa communications, and remote environmental sensing are planned extensions but are not part of the initial implementation.
