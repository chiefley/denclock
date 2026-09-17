# DenClock Status

The .NET 10 time server is implemented; repository history reports it deployed
on the NUC and available on the subnet. These deployment observations have not
been re-tested during documentation work. Existing notes report the NUC and
touchscreen installed, Windows 11 autologon, RustDesk access, and host DenClock.

The browser time client is implemented and accepted. The server now hosts its
assets. Codex has not independently reverified the NUC/LAN browser or touchscreen.
Gateway and sensor firmware remain future work, with no implementation.

## Resume Here

- [Story-driven workflow](docs/DEVELOPMENT.md) is the enduring process reference.
- [Workflow story](docs/stories/002-establish-story-driven-development-workflow.md)
  is the current review item; human acceptance is pending.
- [Time Display](client/docs/stories/completed/001-time-display.md) is accepted;
  run and deployment-verification instructions are in the client README.

See the [server README](server/README.md) for listener/deployment uncertainties,
[client README](client/README.md) for browser hosting and display decisions,
[architecture](docs/ARCHITECTURE.md), and [sensor placeholder](firmware/sensors/README.md).
Active work lives directly in each `docs/stories/`; accepted work lives in its
`completed/` subdirectory.
