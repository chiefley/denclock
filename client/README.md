# DenClock Client

The browser client provides the NUC's 1024 × 600 time-and-date display using
plain HTML, CSS, and JavaScript. The [Time Display story](docs/stories/completed/001-time-display.md)
is implemented and accepted.

## Run and Deploy

From the repository root:

```powershell
dotnet run --project server/DenClock.Server.csproj
```

Open `http://localhost:5000/` in Edge or Chromium on the XPS. Leave the terminal
running. The default `DenClock.Development` launch profile serves files directly
from `client/` and tells the browser to request time from `http://192.168.50.90:5000`.
The JavaScript executes in the XPS browser; the NUC supplies authoritative time.

Edit HTML, CSS, or JavaScript, save, and refresh the browser. No rebuild or
restart is needed for client files, including newly added files. Development
responses use `Cache-Control: no-store`. Use F11 for full-screen viewing.
No separate static server, JavaScript build, or Node runtime is required.

Deploy the updated server to the NUC first so its API permits the development
origin. See [server CORS configuration](../server/README.md#development-browser-access-cors).
If the NUC is unavailable, the clock retains its last sample or shows startup
placeholders; it does not silently switch to the XPS clock.

The development profile in `server/Properties/launchSettings.json` sets
`ASPNETCORE_ENVIRONMENT=Development` and `Client__TimeServerAddress`. To override
the NUC address for one run, without changing production configuration:

```powershell
dotnet run --project server/DenClock.Server.csproj -- --Client:TimeServerAddress=http://DenClock:5000
```

For an intentional local-API test, pass `--Client:TimeServerAddress=` instead.
The default source path is `../client` relative to the server process working
directory (`server/` under `dotnet run`). Other launchers can pass an absolute
`--Client:SourcePath=...` while using the Development environment. A missing
source directory causes startup to fail rather than serving stale copies.

Restart for server/configuration/profile changes; `dotnet watch` may be used for
server source changes. `config.js` retains the synchronization timing values.
The server-generated `client-settings.js` module selects the time API address.
Do not open `index.html` directly as a local file.

## Publishing and Production

Follow the [server deployment instructions](../server/README.md). Publishing
recursively includes client runtime content under `wwwroot`, preserving nested
paths. New runtime files require no project-file edits. Tests, docs, node_modules,
Markdown, package metadata, lockfiles, hidden files, and source maps are excluded.
Keep development-only additions in `tests/` or `docs/`.

Published applications normally run in Production and serve `wwwroot` beside
the executable, independently of the working directory. Launch profiles do not
apply to the deployed service. Production `client-settings.js` always selects
same-origin time, even if `Client:TimeServerAddress` is set. Do not configure the
NUC service with the Development environment.

On the NUC open `http://localhost:5000/`; from another computer open
`http://DenClock:5000/` or `http://192.168.50.90:5000/`. These use the published
client and API together without requiring CORS. Editing local client source
does not update the deployed NUC files; publish and deploy again for that.

The clock displays `--:--` and “Waiting for time” until it receives valid server
time. Once synchronized it advances using `performance.now()`, retains its
last sample through failures, and shows “Sync delayed” after three minutes
without a successful response. A successful response clears that indication
and applies the server's current offset. Background tabs may pause rendering;
the next frame calculates elapsed time rather than counting missed ticks.

## Verification

With Node.js 18 or newer installed for development only:

```powershell
npm test --prefix client
```

The dependency-free tests cover transit compensation, monotonic advancement,
host timezone/wall-clock independence, midnight/noon and date rollover, updated
offsets, periodic synchronization, invalid responses, timeout, and outage recovery.

The hosting integration check requires an unused port 5000 and .NET 10:

```powershell
dotnet build server/DenClock.Server.csproj
node server/tests/hosting.mjs
```

It verifies live source changes without restart, recursive publishing and
exclusions, production configuration, and allowed/rejected CORS origins.

For deployment verification on the NUC and development browser:

1. Open the NUC's page over the LAN; verify its displayed time and date against
   `/api/time`, including from a computer configured with a different timezone.
2. View full-screen at 1024 × 600 and assess legibility, spacing, and clipping.
3. After synchronization, block `/api/time` using browser developer tools.
   Verify seconds continue, “Sync delayed” appears after three minutes, and
   unblocking clears it at the next successful synchronization.
4. Reload with `/api/time` blocked. Verify placeholders remain until unblocked
   and the next retry succeeds. Observe that normal API requests occur roughly
   once per minute, rather than once per displayed second.

Codex has not independently verified NUC/LAN browser behavior or touchscreen
appearance. Final typography should be evaluated on the actual display. Production kiosk startup and recovery are deferred.

## References

- [Implemented client contract](docs/contract.md)
- [Client design decisions](docs/decisions.md)
- [Implemented server contract](../server/docs/contract.md)
- [Development workflow](../docs/DEVELOPMENT.md)

Active/pending stories live in `docs/stories/`; human-accepted stories move to
`docs/stories/completed/`.

For the [development workflow story](docs/stories/completed/002-client-development-and-deployment-workflow.md),
load the XPS page and confirm in browser Network tools that time requests go
to the NUC and succeed. Edit a local CSS declaration, save, refresh, and verify
the change while the same server process runs. Restore the edit afterwards.
Verify NUC stop/restart recovery with this page still open. The developer confirmed NUC service stop/restart recovery and accepted this
workflow story.
