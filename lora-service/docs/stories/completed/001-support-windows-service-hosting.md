# Story: Support Windows Service Hosting

## Objective

Allow the existing LoRa worker to start and stop as a native Windows Service
using the developer's existing NUC publish, installation, and startup scripts.
The current console host lacks Windows Service lifetime integration.

## Requirements

- Add Microsoft.Extensions.Hosting.WindowsServices, matching the existing
  Microsoft.Extensions.Hosting version, and register AddWindowsService in Program.cs.
- Preserve interactive console execution and graceful shutdown.
- Preserve executable-directory configuration loading and existing TCP/protocol behavior.
- Keep publishing framework-dependent for Windows x64; the NUC supplies .NET 10.
- Document Windows Service hosting, runtime requirements, and diagnostic logging.
- Preserve the developer's existing deployment and installation scripts.

## Verification and acceptance

- Build the worker and run the existing protocol/session/TCP tests.
- Publish for win-x64 with --self-contained false.
- Check interactive host startup and graceful shutdown locally.
- On the NUC, use existing scripts to publish/install/start. Confirm Windows
  reports Running and the gateway's reports are decoded. Stop/restart and confirm
  recovery and resumed reports without changing the firmware.
- Windows Service execution requires developer verification and acceptance.

## Out of scope

Protocol changes, firmware changes, new installation tooling, automatic recovery
policy, MQTT, and self-contained deployment.

## Verification for human review

- Worker build passes with zero warnings/errors after restoring with NuGet access.
- All 254 existing protocol, session, and TCP checks pass with loopback access.
- Framework-dependent win-x64 publish succeeds; runtimeconfig references
  Microsoft.NETCore.App 10.0.0, and Windows Service hosting assemblies are included.
- Console host starts from a different working directory with endpoint overrides
  and exits cleanly on Ctrl+C. Configuration uses the executable directory.
- Existing developer changes in Deploy.sh are preserved separately.

## Human acceptance

The developer confirmed the story is verified complete. Windows Service hosting
on the NUC is accepted. Framework-dependent publishing and interactive console
execution remain supported; protocol behavior is unchanged.
