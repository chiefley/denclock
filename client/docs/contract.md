# Client Implemented Contract

The browser client displays 12-hour hours/minutes without a leading hour zero
or AM/PM, subordinate two-digit seconds, and full weekday, month, and day.
The normal display has no controls or other content. It targets 1024 × 600.

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
