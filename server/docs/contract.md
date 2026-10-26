# Server Implemented Contract

`GET /api/time` returns HTTP 200 with JSON, for example:

```json
{
  "utcTime": "2026-09-14T16:00:00+00:00",
  "unixTimeMilliseconds": 1789401600000,
  "timeZoneId": "Eastern Standard Time",
  "utcOffsetSeconds": -14400
}
```

Both timestamps describe the same instant sampled while handling the request.
The time zone ID is the host's native ID (normally a Windows ID on the NUC),
and the offset is in seconds east of UTC, including daylight saving time at
that instant. Restart the server after changing the host's configured time zone.
Responses use `Cache-Control: no-store` so clients obtain a fresh sample.

The client should periodically fetch time and advance it between requests using
a monotonic elapsed-time clock. This endpoint is a system-clock snapshot,
not a precision network time protocol. The offset describes the sampled instant;
clients should refresh it periodically, including across daylight saving changes.

## Listener and Browser Access

`Program.cs` explicitly adds `http://0.0.0.0:5000` to `app.Urls`.
`appsettings.json` also specifies `http://localhost:5000`. Do not describe this
as a localhost-only server or assume editing `Urls` removes the code-added
binding. Verify effective startup listeners in the deployment environment.
No authentication is configured. `GET /` serves the browser clock. Development
serves `Client:SourcePath` (default `../client` relative to the working directory)
directly with no-store static responses. Other environments serve executable-local
`wwwroot`. Runtime content is recursively copied there during build/publish,
with development files excluded by the project file.

`GET /client-settings.js` returns a no-store JavaScript module exporting
`serverAddress`: `Client:TimeServerAddress` in Development, otherwise empty
(same-origin). `/api/time` and `/api/presentation` grant CORS to exact origins in
`Cors:DevelopmentOrigins`, with GET allowed and no credentials. Settings use the additional scoped policy
described below. The default list
contains `http://localhost:5000`. Unlisted origins receive no access grant.

## Evidence and Limits

Checked against `Program.cs`, `appsettings.json`, and the project file.
Local development verification confirmed HTTP 200 for the page, runtime assets,
and time API, including the API's `no-store` header. Build and publish succeeded,
and the published output contains the client assets. NUC/Windows deployment has
not been reverified for this change.
Windows Service integration uses service name `DenClock.Server`; the application
content root is the executable directory so service startup does not depend on
its working directory. The project targets .NET 10.

## Sensor Presentation

The server initializes and loads ProgramData presentation.json before starting workers.
See [presentation configuration](presentation-configuration.md) for validation
and the initialization/runtime contract. The executable-local file is included
in build/publish output as a first-run migration source. Invalid existing files
or failed initialization prevent startup; absent files initialize defaults. An
unavailable MQTT broker does not prevent startup.

A separate hosted MQTTnet 5.2.0.1603 subscriber uses MQTT 3.1.1, clean sessions,
and QoS 1 on lora/sensors/+. Mqtt:Host, Mqtt:Port, and Mqtt:ClientId default to
localhost, 1883, and denclock-server. Use unique client IDs for concurrent
DenClock instances. Operations time out after five seconds; the worker retries
at five-second intervals and resubscribes after reconnect.

Latest configured observations are held in memory. No wire-protocol decoding,
history, or raw-packet interpretation occurs here. Malformed envelopes and
topic/payload identity disagreements are logged and ignored. Partial valid
reports replace the whole observation; omitted/invalid measurements become null.
Unknown sensors are ignored. Duplicate or older receivedUtc values do not
refresh observations. Future timestamps are rejected. Shutdown cancels the worker.

GET /api/presentation returns HTTP 200 with Cache-Control: no-store and the same
exact-origin development CORS policy as /api/time. Example:

~~~json
{
  "serverUtc": "2026-09-22T13:24:30+00:00",
  "temperatureUnit": "F",
  "slots": [
    {
      "position": "upperLeft",
      "internalName": null,
      "displayName": "",
      "maxAgeSeconds": null,
      "observation": null
    },
    {
      "position": "upperRight",
      "internalName": "porch",
      "displayName": "Outside",
      "maxAgeSeconds": 60,
      "observation": {
        "receivedUtc": "2026-09-22T13:24:24.66779+00:00",
        "temperatureC": 21.86,
        "relativeHumidity": 52.02
      }
    }
  ]
}
~~~

There are always two slots in upperLeft/upperRight order. Assigned sensors with
no report retain their configured identity, label, and age limit but have a null
observation. Each measurement can independently be null. No secondary view,
battery/radio diagnostics, or broker configuration is exposed. The snapshot is
coherent under concurrent updates; serverUtc is sampled while creating it.

The browser anchors this server timestamp to monotonic elapsed time, converts
Celsius only for presentation, and keeps aging readings between API responses.
Server restart clears memory until MQTT supplies retained or new reports.

## Presentation Settings API

`GET /api/settings/presentation` returns HTTP 200 with
`{"revision":"opaque-token","configuration":{"temperatureUnit":"F","sensors":[]}}`.
Configuration includes all primary and secondary entries and extra fields.
Revisions change on successful saves and service restart.

`PUT /api/settings/presentation` requires a JSON body with the same envelope.
The revision must match the current read response. Configuration is validated
with the presentation rules. Success returns 200 and the new envelope, after
atomic disk persistence and runtime activation. HTTP 400 indicates invalid JSON,
missing/invalid fields, or invalid configuration; 409 indicates a stale revision;
415 indicates a non-JSON content type; 500 indicates a persistence failure.
Errors have an `error` message without a stack trace. Read/write responses,
including errors, use Cache-Control: no-store. Failed saves leave previous disk
and active settings intact.

Browser writes require an Origin matching the request's served origin or an
exact configured `Cors:DevelopmentOrigins` entry; unrelated and null origins
receive 403. Origin-less non-browser JSON requests are permitted. Settings CORS
allows GET/PUT and Content-Type only for explicitly configured origins, with
preflight support and no credentials. Time/presentation keep their GET-only
policy. Settings work from server-hosted LAN clients, not just loopback. This
origin check is not an authentication system.

Configuration replacement is coherent with MQTT reception and snapshots.
Observations are retained only for identities remaining configured, with their
original timestamps. No service restart or MQTT reconnection is required.
