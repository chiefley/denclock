Server technology: DenClock.Server is implemented in C# using .NET and ASP.NET Core. It is intended to run continuously as a Windows Service on the DenClock NUC. The server provides network APIs to the display client and, eventually, other DenClock components. Platform-independent sensor and control protocols are kept separate from the server implementation.

## Initial server

The first server is a .NET 10 ASP.NET Core minimal API in `server/`.
It runs as a Windows Service in production and as a console application
for development. It listens on `http://localhost:5000` by default because
the display client runs on the same NUC.

`GET /api/time` returns UTC time, Unix milliseconds, the host time zone ID,
and the current UTC offset. Windows is responsible for network time
synchronization. The server reads the system clock on each request and
disables response caching; it does not independently synchronize time or
report synchronization health.

The client will advance time smoothly between requests using elapsed time.
See [server instructions](../server/README.md) for the API contract and
Windows publishing and service installation commands.
