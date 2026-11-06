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
  Cover scaling fills the display without distorting the image; bottom-right
  anchoring with small offsets keeps the embedded logo positioned on the Surface. Disabled pointer
  events and separate stacking keep it independent of clock layout and input.
  Dynamic image selection remains deferred.

- Keep MQTT subscription and canonical latest observations on the server.
  The browser polls one presentation snapshot for both panels; five-second
  polling avoids another transport dependency for two low-rate readings.
- Anchor freshness to server UTC and monotonic browser elapsed time so network
  loss cannot freeze an old reading in the fresh color.
- Convert and format temperatures only in the browser, preserving Celsius in
  MQTT and the API. Round before splitting the whole and fractional glyphs.
- Group the clock and two primary readings across the top to preserve the cabin
  view below. Use condensed typography and one primary digit size for time and
  whole-degree temperature; keep sensor labels and humidity on one supporting
  line. Secondary views and panel interactions remain deferred.

- Use a native modal dialog for settings to provide focus containment and Escape
  cancellation without a framework. The browser owns only a draft; the server
  owns validation, revisions, persistence, and runtime application. Preserve
  unedited configuration fields and secondary identities through full-document
  round trips, and require an explicit reload after conflicts so drafts are not
  silently discarded or newer settings overwritten.

- Keep forecast geocoding and retrieval in the existing browser weather path.
  Persist the resolved location with server-owned presentation settings so all
  displays share it and restarts do not require geocoding. Validate changed
  locations before saving; preserve legacy coordinates when no setting exists.
  Place the location before the forecast heading and center the weather credit
  beneath the time axis to keep it clear of the cabin logo.

- For the Linux XFCE/X11 tablet, use a fullscreen Chromium app window instead
  of locked kiosk mode so touchscreen and RustDesk users can reach desktop
  controls. A separate loopback helper reads and changes actual window-manager
  state for the dedicated DenClock window; browser fullscreen alone cannot
  restore a locked kiosk window. Keep this platform-specific integration outside
  the server and restrict it to the local clock page.

- Keep hourly and daily forecasts in one Open-Meteo response and the existing SVG
  chart area. Changing views only selects cached data, avoiding tap latency and
  extra requests. Keep the view choice in page memory and omit condition icons
  to emphasize the temperature trend without adding visual clutter. Daily Unix
  timestamps are converted with the provider's response offset; calendar-day
  selection uses synchronized UTC in the forecast timezone, independent of host
  time and of elapsed-hour arithmetic across DST.

- Use a fixed canvas tile map for radar: the display needs a regional overview,
  not interactive map navigation, so a small Web Mercator renderer avoids a new
  mapping framework and gesture conflicts with forecast cycling. Keep the base
  layer dark/subdued and radar independent. Radar now deliberately uses an opaque
  backing and full-opacity layers for storm readability, while both forecast
  charts retain their transparent presentation. Follow
  RainViewer's stricter transition limits, serialize and cache tile requests,
  and preload complete frames before animation to avoid partial-image flicker.
  Use synchronized time for expiry; radar failures remain isolated from the
  existing Open-Meteo and clock paths.

- Make radar replay a small native button, independent of forecast cycling, and
  play the cached frames directly. Show newest-frame age using synchronized UTC
  in the existing render loop so paused imagery cannot look fresh just because
  it was recently downloaded. Preserve the distinction between RainViewer's
  composite frame-generation timestamp and unavailable exact observation times.

- Prioritize the latest complete precipitation canvas before base-map tiles or
  historical frames. Keep the shared serialized, throttled tile queue and bounded
  URL cache: independently addressed frames already support reuse, and ready
  sequences replay without requesting tiles again. Preserve provider timestamps
  and existing synchronized-time freshness/expiry checks on every opening.
  Hold the latest image if history is incomplete rather than playing a partial
  sequence. Generation tokens prevent obsolete loading from changing the view.
