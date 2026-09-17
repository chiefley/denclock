# Server Status

## Current State

Implemented: .NET 10 ASP.NET Core `/api/time`, UTC/Unix timestamps, host timezone
and current offset, `Cache-Control: no-store`, and Windows Service integration.
Windows owns network time synchronization; the API does not report sync health.
See [contract](docs/contract.md) and [run/deploy instructions](README.md).

Repository history reports local testing, NUC deployment, and subnet availability.
The documentation audit inspected source; it did not revalidate the running NUC.

## Active Story

No separate server story is recorded. Support for the planned
[client Time Display story](../client/docs/stories/001-time-display.md) may require
a hosting/origin decision before implementation.

## Next Step

Confirm effective NUC listeners and intended deployment method; decide how the
browser client will access the API during LAN development.

## Known Issues

- Source adds `0.0.0.0:5000`; configuration specifies localhost. Prior local-only
  documentation did not reflect the source. Listener policy needs confirmation.
- No CORS, authentication, or static client hosting is configured.
- Manual publishing is self-contained; `deploy.sh` is framework-dependent and
  does not manage service stop/start. NUC runtime availability is unverified here.
- Restart after changing the host timezone; verify Windows time synchronization
  independently of the API.

## Design Rationale

Use the Windows-maintained clock rather than a second synchronization service.
Return UTC plus the host's offset so clients need not interpret Windows timezone
IDs. Use the executable directory as content root because Windows Services may
start from System32. These choices are reflected in source and existing docs.
