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
No authentication or CORS policy is configured. `GET /` serves the browser
clock from `wwwroot/index.html`; `clock.css`, `clock.js`, and `config.js` are
also served from that directory. The server project includes these four client
assets in build and publish output. The default client requests `/api/time`
from the same origin.

## Evidence and Limits

Checked against `Program.cs`, `appsettings.json`, and the project file.
Local development verification confirmed HTTP 200 for the page, runtime assets,
and time API, including the API's `no-store` header. Build and publish succeeded,
and the published output contains the client assets. NUC/Windows deployment has
not been reverified for this change.
Windows Service integration uses service name `DenClock.Server`; the application
content root is the executable directory so service startup does not depend on
its working directory. The project targets .NET 10.
