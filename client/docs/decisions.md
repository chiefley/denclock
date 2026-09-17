# Client Design Decisions

These choices underpin the accepted Time Display implementation.

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

- Serve the four client assets from the existing ASP.NET Core server. A relative
  API address supports both LAN and localhost viewing without a separate web
  server, added runtime dependency, or CORS policy. Source stays in `client/`;
  the server project copies only the runtime assets into `wwwroot` on build/publish.
- Allow five seconds per request and retry every minute. Indicate delayed
  synchronization after three minutes, tolerating a brief interruption without
  distracting from the clock. Retain the last valid sample on errors.
- Apply each successful sample directly, including offset changes. Slewing and
  timezone prediction would add complexity beyond this initial display.

See [client README](../README.md) for operation and verification, and the
[story](stories/completed/001-time-display.md) for the requested behavior.
