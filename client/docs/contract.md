# Client Implemented Contract

The browser client displays 12-hour hours/minutes without a leading hour zero
or AM/PM, subordinate two-digit seconds, and full weekday, month, and day.
The normal display includes an unobtrusive lower-left Settings gear with no
visible button background or border. It targets 1024 × 600, with the canonical
cabin image filling the viewport behind the clock. The decorative background
preserves aspect ratio using centered cover scaling and cannot intercept input
or alter the clock's layout. Background selection is static.

`TimeClient` requests `/api/time` immediately and approximately every 60 seconds.
It requires an integer Unix timestamp in the supported JavaScript date range
and an integer UTC offset in seconds within ±24 hours. Failed HTTP requests,
invalid JSON/data, and requests exceeding five seconds leave the previous
sample intact. Requests do not overlap. Responses are requested without caching.

At receipt, server UTC is adjusted by half the measured round-trip duration.
Displayed civil time is that sample plus the server offset and elapsed
`performance.now()` time. Neither the host wall clock nor its timezone supplies
display time. Each successful synchronization replaces the sample and offset;
the client does not interpret timezone IDs or predict DST transitions.

Before the first successful response, the display shows `--:--`, `--` seconds,
no date, and “Waiting for time”. Afterwards it continues through communication
failures. “Sync delayed” appears when the last successful response is at least
three minutes old and clears on success. A page reload discards the sample.

`ClockDisplay` formats the supplied civil time using UTC accessors and an
explicit UTC date formatter. Animation frames refresh the view; frames do not
advance a counter and the display does not request time itself.

The production server hosts the client assets and API on one origin.
`config.js` supplies timing values; the server-generated `client-settings.js`
selects the API address. Production always selects the page's origin.
Development uses `Client:TimeServerAddress` (empty means same-origin), with the
repository launch profile selecting the NUC. Cross-origin development requires
that API server to permit the page's exact origin. No JavaScript build is needed.

See the [README](../README.md) for operation and verification limitations.

## Primary Sensor Panels

The upper-left and upper-right panels show whole-degree temperature with bold
digits at the main time's size. The time is lower and uses a 700-weight face;
main digits use min(16.5vw, 28vh), or 168 CSS pixels at 1024 x 600. Visible glyph
height, rather than CSS font size, targets approximately one inch. The developer verified and accepted the display; subsequent layout changes should
still be checked on the physical shelf monitor.

Tenths, unit, sensor label, humidity, day/date, and seconds are subordinate.
Temperature tenths have the same color as whole-degree digits. Humidity is rounded
to whole percent for presentation. The cabin background remains a separate layer.
Panels have no click action; secondary sensors have no view.

PresentationClient requests /api/presentation immediately and every five seconds,
using the same serverAddress as time. Requests use no-store, have five-second
timeouts, and do not overlap. HTTP errors, invalid JSON, and invalid snapshot
shapes preserve the prior complete snapshot. The first response provides labels,
units, and both configured slots. Before it arrives, placeholders appear.

Canonical Celsius values convert to F only when configured, then round once to
one decimal before splitting whole/fractional digits; negative fractions and
decimal carry are preserved, while negative rounded zero displays as 0.0.
Unknown temperature is --.- and unknown humidity is --.

A successful response anchors serverUtc plus half the request's round-trip time
to performance.now(). Age is calculated from the original receivedUtc using that
anchor and monotonic elapsed time; browser wall clock and timezone are irrelevant.
Freshness keeps aging through HTTP/MQTT outages and background-tab pauses.
At age >= maxAgeSeconds, known temperature and humidity values turn red without
being discarded. A fresh report restores normal color. Receiving the same old
report in a new snapshot does not reset its age.

## Settings Preview

The Settings gear opens a full-screen modal dialog. Controls preview the
example configuration in the [configuration specification](../../server/docs/presentation-configuration.md),
not configuration loaded from the service. They include Celsius/Fahrenheit,
upper-left/right sensor assignments (including Unassigned), display labels,
and stale-reading limits in seconds.

Edits stay in the form only while open. Cancel or Escape closes the dialog,
discards edits, and restores focus to the gear. Save is disabled, with visible
text: “Saving settings will be available in a later update.” No configuration
requests, persistence, or live presentation changes occur.

The dialog provides associated labels, native modal keyboard navigation,
visible focus, and controls at least 48 CSS pixels high. It scrolls when needed
and stacks sensor fields in one column at viewport widths of 600 pixels or less.
Time synchronization, presentation polling, and rendering continue while open.
