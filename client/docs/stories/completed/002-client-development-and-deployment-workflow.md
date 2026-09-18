# Client Development and Deployment Workflow

Status: Complete; accepted by the human developer.

The developer confirmed that stopping the NUC time service produces the client
synchronization warning and restarting the service clears it automatically.

## Goal

Streamline Den Clock client development and deployment so that browser client files can be edited and tested without rebuilding or restarting the ASP.NET server, while retaining simple single-process deployment on the NUC.

Also support development on the XPS where the locally hosted browser client uses the NUC's `/api/time` endpoint as its authoritative time source.

## Current Situation

`DenClock.Server` currently performs two functions:

1. Provides the `/api/time` endpoint.
2. Hosts the browser client as static content.

The client source lives separately under `client/`.

The server project currently explicitly lists the individual client runtime files in `DenClock.Server.csproj` and copies them into `wwwroot` during build and publish.

Consequently, during development a change to a client HTML, CSS, or JavaScript file requires restarting `dotnet run` so the changed source file is copied into the server output directory.

The client currently defaults to a same-origin `/api/time` request. This is appropriate for production on the NUC, but development on the XPS should be able to host the client locally while using the NUC's server as the authoritative time source.

## Required Behavior

### 1. Direct client hosting during development

When `DenClock.Server` is run for development from the repository, it shall serve browser content directly from the `client/` source tree rather than from copies made during the build.

A normal client development cycle should therefore be:

```text
edit client file
save
refresh browser
```

Changes to HTML, CSS, and JavaScript must not require rebuilding or restarting `DenClock.Server`.

No separate Python, Node, or other static web server should be required.

Server-side source changes may still require rebuilding/restarting the ASP.NET application and may use the normal .NET development facilities such as `dotnet watch`.

### 2. Production publishing

The `client/` source tree shall remain logically separate from `server/`.

During `dotnet publish`, the client runtime content shall be copied into the published application's `wwwroot` hierarchy so that the deployed `DenClock.Server` can serve both the browser application and its API.

Client runtime files shall be included recursively rather than enumerated individually in `DenClock.Server.csproj`.

Adding a new runtime client file or subdirectory, such as:

```text
client/weather/weather.js
client/images/cloudy.svg
```

must not require modifying `DenClock.Server.csproj`.

Preserve client runtime subdirectory structure under the published `wwwroot`.

Development-only material such as tests, documentation, package metadata, or other files not required by the browser shall not unnecessarily become production web content.

### 3. Production same-origin operation

On the NUC, the deployed browser client and API shall continue to share the same origin.

For example:

```text
http://localhost:5000/
http://localhost:5000/api/time
```

The production client shall therefore continue to work with a relative/same-origin API address and shall not require CORS.

The same deployed client files should also work when the NUC is accessed by hostname or IP address from another machine.

### 4. XPS development against the NUC time server

During client development, the browser client hosted by the XPS shall be configurable to use the NUC's `/api/time` endpoint rather than the XPS server's own `/api/time`.

Conceptually:

```text
XPS
    DenClock.Server
        └── serves client source directly

    Browser
        └── client application
                │
                │ HTTP / CORS
                ▼
NUC
    DenClock.Server
        └── /api/time
```

The development configuration must not require modifying production source configuration every time the application is deployed.

The normal production configuration remains same-origin.

### 5. CORS

Configure `DenClock.Server` so that the NUC API can accept the required browser request from the XPS development client.

The CORS policy should be deliberately limited to the development origin or origins needed for Den Clock development rather than globally allowing arbitrary origins.

CORS shall not be required for normal same-origin production operation.

Document how the permitted development origin is configured so that a future change to the XPS development URL does not require rediscovering the implementation.

### 6. Preserve time-authority rules

This story must not change the existing time synchronization semantics.

When the XPS browser is configured to use the NUC API, the NUC's `/api/time` response remains authoritative.

The client must continue to:

- use `unixTimeMilliseconds` as authoritative UTC time;
- use the server-provided `utcOffsetSeconds`;
- advance between synchronizations using monotonic elapsed time;
- avoid using the browser host's wall clock or timezone as authoritative time;
- continue from the last successful synchronization during an outage;
- indicate stale synchronization according to the existing client contract.

## Documentation

Update the appropriate enduring documentation to explain:

- how client files are hosted during development;
- how client files become part of a published NUC deployment;
- how to run the XPS development client against the NUC API;
- how CORS is configured;
- the normal edit/save/refresh client workflow;
- when restarting or rebuilding the ASP.NET server is actually required.

Do not duplicate this information unnecessarily between documents. Follow the repository documentation roles established in `docs/DEVELOPMENT.md`.

## Verification

Demonstrate at least the following:

1. Start the development server on the XPS.
2. Load the client in the XPS browser.
3. Verify that it obtains authoritative time from the NUC.
4. Modify a client CSS or JavaScript file while the server remains running.
5. Refresh the browser and verify that the modification appears without rebuilding or restarting the server.
6. Verify that cross-origin `/api/time` requests from the configured XPS development origin succeed.
7. Verify that an unapproved cross-origin origin is not granted CORS access.
8. Publish the application and verify that the required client runtime files and subdirectories appear under the published `wwwroot`.
9. Verify that development-only client files are not unnecessarily published.
10. Verify that the published application serves `/` and `/api/time` from the same origin.
11. Run the existing client and server tests and ensure the existing time behavior remains intact.

Human verification on the actual NUC remains required before this story is considered complete.

## Completion

Follow the repository story workflow in `docs/DEVELOPMENT.md`.

Codex shall not independently declare the story complete. After implementation and testing, report the changes and verification results for human review. Move the story to `completed/` and perform the normal documentation/staging handoff only after human acceptance.


## Implementation Verification

- Seven existing client tests pass; time synchronization logic is unchanged.
- New `node server/tests/hosting.mjs` integration checks pass for live source
  edits/new nested assets, recursive publish/exclusions, production page/API
  hosting from an unrelated working directory, production same-origin settings,
  and allowed/rejected CORS origins including an allowed preflight.
- The repository launch profile was started successfully in Development.
- Build and publish succeeded. NuGet reported NU1900 because vulnerability
  metadata could not be retrieved; retrying restore outside the sandbox did
  not resolve that warning.
- Documentation links and whitespace checks pass.
- Codex performed local automated verification; the developer subsequently
  confirmed NUC service stop/restart recovery in the client and accepted the story.
