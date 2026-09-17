# Client Design Decisions

These are the design direction recorded in the original client specification;
they do not claim that a client has been implemented.

- Use plain HTML, CSS, and JavaScript for the initial display; no framework or
  build system is required for this small increment.
- Derive displayed time from server UTC, server offset, and monotonic elapsed
  time so development-host clock and timezone settings cannot change it.
- Keep `TimeClient` synchronization separate from `ClockDisplay` presentation
  so display formatting does not own authoritative time or server requests.
- Let the server interpret timezone/DST rules; the client uses the returned
  offset and refreshes it on synchronization instead of carrying a timezone database.
- Estimate half the round-trip time for transit compensation, assuming roughly
  symmetric LAN latency. This is a simple estimate, not a precision guarantee.

Hosting/origin policy and stale-sync treatment remain open; see
[client README](../README.md) and the [story](stories/001-time-display.md).
