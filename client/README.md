# DenClock Client

The planned browser client provides the NUC's 1024 × 600 time-and-date display.
No client source, tests, UI, or implemented client contract exists yet; `src/`,
`tests/`, and `assets/` are scaffolding. The original specification is preserved
as the pending [Time Display story](docs/stories/001-time-display.md). Its
requirements include server-authoritative time, monotonic advancement, periodic
synchronization, and loss-of-server behavior. Implementation awaits a separate
human request; this workflow change does not begin that story.

## Before Client Implementation

Choose browser serving/origin arrangements for LAN development and the NUC.
The server neither serves client files nor configures CORS; LAN reachability
alone does not establish browser cross-origin access. After that decision,
the Time Display story begins with initial synchronization and its acceptance
criteria.

Choose the stale-sync indication and timeout during client development. Evaluate
final typography/layout on the actual touchscreen. Production kiosk startup
and recovery remain explicitly deferred by the story.

## References

- [Client design decisions](docs/decisions.md)
- [Implemented server contract](../server/docs/contract.md)
- [Development workflow](../docs/DEVELOPMENT.md)

Active/pending stories live in `docs/stories/`; human-accepted stories move to
`docs/stories/completed/`.
