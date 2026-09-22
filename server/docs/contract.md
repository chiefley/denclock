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
`Cors:DevelopmentOrigins`, with GET allowed and no credentials. The default list
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

The server loads executable-local presentation.json before starting workers.
See [presentation configuration](presentation-configuration.md) for validation
and the startup/restart contract. The file is included in build/publish output.
Missing/invalid configuration fails startup; an unavailable MQTT broker does not.

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
