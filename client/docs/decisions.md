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

- Keep one ASP.NET Core host. Development serves `client/` directly so browser
  edits need only a refresh. Build/publish recursively copies runtime files to
  `wwwroot`, excluding development material. Generated static-asset manifests
  are disabled because ordinary static-file middleware handles these files.
- Supply API selection through a server-generated module. A development launch
  profile selects the NUC; Production always supplies an empty same-origin
  address. This prevents a development address from leaking into deployment.
- Permit configured exact development origins on the time endpoint only, so the
  XPS browser can use NUC time without a proxy or a second static server.
- Allow five seconds per request and retry every minute. Indicate delayed
  synchronization after three minutes, tolerating a brief interruption without
  distracting from the clock. Retain the last valid sample on errors.
- Apply each successful sample directly, including offset changes. Slewing and
  timezone prediction would add complexity beyond this initial display.

See [client README](../README.md) for operation and verification, and the
[story](stories/completed/001-time-display.md) for the requested behavior.

- Keep the cabin background in its own fixed decorative layer and stylesheet.
  Cover scaling fills the display without distorting the image; disabled pointer
  events and separate stacking keep it independent of clock layout and input.
  Dynamic image selection remains deferred.

- Keep MQTT subscription and canonical latest observations on the server.
  The browser polls one presentation snapshot for both panels; five-second
  polling avoids another transport dependency for two low-rate readings.
- Anchor freshness to server UTC and monotonic browser elapsed time so network
  loss cannot freeze an old reading in the fresh color.
- Convert and format temperatures only in the browser, preserving Celsius in
  MQTT and the API. Round before splitting the whole and fractional glyphs.
- Reserve upper-left/right positions for the strategic readings; defer secondary
  views and panel interactions. Use one shared primary digit size for time and
  whole-degree temperature.

- Use a native modal dialog for settings to provide focus containment and Escape
  cancellation without a framework. The browser owns only a draft; the server
  owns validation, revisions, persistence, and runtime application. Preserve
  unedited configuration fields and secondary identities through full-document
  round trips, and require an explicit reload after conflicts so drafts are not
  silently discarded or newer settings overwritten.
