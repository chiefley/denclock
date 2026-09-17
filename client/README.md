# DenClock Client

The browser client provides the NUC's 1024 × 600 time-and-date display using
plain HTML, CSS, and JavaScript. The [Time Display story](docs/stories/completed/001-time-display.md)
is implemented and accepted.

## Run and Deploy

From the repository root:

```powershell
dotnet run --project server/DenClock.Server.csproj
```

Open `http://localhost:5000/` in Edge or Chromium. Use F11 for full-screen
viewing. The server serves the client and `/api/time` from the same origin.
No JavaScript build, Node.js runtime, or separate web server is needed to run
the display. Restart `dotnet run` after editing client files so they are copied
to the output directory, then reload the page.

Publishing the server also includes the four client files under `wwwroot`.
Follow the [server deployment instructions](../server/README.md) to publish
and copy the whole output to the NUC. From another computer on the LAN, open
`http://DenClock:5000/` (or the NUC's IP address); on the NUC, use
`http://localhost:5000/`. This requires deploying the updated server first.

`config.js` controls the server address, 60-second synchronization interval,
5-second request timeout, and 3-minute stale threshold. The default empty
server address uses the page's origin, so the same files work over the LAN or
on localhost. An explicit address is supported, but using a different origin
requires a separately configured server CORS policy. The supplied deployment
uses one origin and does not enable CORS. Do not open `index.html` as a local file.

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
