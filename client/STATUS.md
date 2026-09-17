# Client Status

## Current State

No client source is implemented. The existing `src/`, `tests/`, and `assets/`
directories are scaffolding. There is no implemented client contract yet.

## Active / Next Story

[Time Display](docs/stories/001-time-display.md) is planned, following acceptance
of the repository documentation story. Its preserved requirements include
server-authoritative time, monotonic advancement, periodic synchronization,
loss-of-server behavior, and a 1024 × 600 time/date display.

## Latest Result

Documentation audit preserved the original specification as a story and
separated it from implemented server behavior. No client tests or UI exist.

## Next Step

Choose how the browser client is served during LAN development and on the NUC.
The current server neither serves client files nor configures CORS. Then begin
the story's initial synchronization behavior and validate its acceptance criteria.

## Known Issues / Decisions Needed

- LAN reachability alone does not establish browser cross-origin access.
- Choose the stale-sync indication and timeout during development.
- Final typography/layout needs evaluation on the actual touchscreen.
- Production kiosk startup and recovery remain explicitly deferred.

## References

[Client decisions](docs/decisions.md) · [Server contract](../server/docs/contract.md)
