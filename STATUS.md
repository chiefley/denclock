# DenClock Status

## Current State

The repository implements the .NET 10 time server; the browser client has a
specification but no implementation. No gateway or sensor firmware exists.
The existing project notes report the NUC and touchscreen installed, Windows 11
autologon configured, RustDesk access established, and host name DenClock.
Repository history reports the NUC time service deployed and available on the
subnet. These deployment observations were not re-tested during this docs audit.

## Active Story

[Documentation and context recovery](docs/stories/001-context-recovery.md) —
prepared for human review; completion requires human acceptance.

## Next Step

Review this reconciliation and resolve the questions below. After acceptance,
resume the [Time Display story](client/docs/stories/001-time-display.md), starting
with the browser hosting/origin decision recorded in client status.

## Known Issues / Human Decisions

- Confirm the intended server listener policy: code adds all IPv4 interfaces;
  configuration still specifies localhost. See [server status](server/STATUS.md).
- Choose browser hosting/origin for LAN development; no CORS or static client
  hosting is implemented in the server.
- Confirm the intended deployment route and installed .NET runtime on the NUC;
  manual and scripted deployment differ.
- Client stale-sync timeout and visual indication remain to be decided.

## Component References

- [Server status](server/STATUS.md) and [implemented contract](server/docs/contract.md)
- [Client status](client/STATUS.md) and [design decisions](client/docs/decisions.md)
- [Sensor placeholder](firmware/sensors/README.md)
- [Architecture and rationale](docs/ARCHITECTURE.md)
