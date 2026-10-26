# 009 — Load, Save, and Apply Display Settings

## Purpose

Connect the existing gear-triggered settings UI to DenClock.Server. Replace example values with actual service-owned configuration, persist changes in ProgramData, and apply them without restarting the service.

Configuration must work from the NUC touchscreen and any DenClock client on the LAN.

Follow the repository workflow. Determine the next story number under root `docs/stories/`, inspecting active and completed stories. Work in the existing `~/src/denclock` workspace.

## Configuration Ownership and Storage

DenClock.Server owns validation, persistence and active display configuration. The browser edits a temporary draft.

Use `%ProgramData%\DenClock\Server\presentation.json` as the production configuration file. Resolve ProgramData through the Windows common application data directory.

Provide an explicit configuration-directory override for development and automated tests so they do not modify production settings.

Only presentation configuration moves in this story. Leave `appsettings.json` and LoRa Service configuration unchanged.

## First-Run Initialization

Before accepting requests:

1. If the ProgramData presentation file exists, validate and load it.
2. Otherwise, if `presentation.json` exists beside the executable, validate it and initialize the ProgramData file from it. Preserve its settings and leave the original file unchanged.
3. If neither exists, create a default configuration selecting Fahrenheit with no assigned sensors.

Create the destination directory as needed. Write initialization atomically.

An existing invalid file must produce a clear diagnostic identifying its path and validation problem. Do not replace it with defaults or fall back to another file.

Initialization or permission failures must be reported clearly. Do not claim successful initialization if persistence failed.

Once initialized, ProgramData is authoritative. Subsequent deployments must not overwrite it with bundled examples.

## Settings API

Add service endpoints to read and update presentation settings, following existing API conventions.

- Return actual configuration, including entries not shown in the two primary panels.
- Use no-store responses.
- Validate updates on the server using the existing presentation rules.
- Require a revision supplied by the read response when saving.
- Reject stale revisions with a conflict response so one LAN client cannot silently overwrite another client’s changes.
- Serialize saves and persist through a temporary file in the same directory followed by atomic replacement.
- Publish the new active configuration only after persistence succeeds.
- On validation, conflict or persistence failure, retain the previous file and active configuration.
- Return useful errors without exposing stack traces.

## Existing Settings UI

Replace preview behavior with actual service configuration.

- Fetch fresh settings whenever the view opens.
- Show loading and failure states. Disable Save until loading succeeds.
- Remove the example-preview notice and unavailable-Save explanation.
- Populate assignments from configured sensor identities, including secondary entries. Remove hardcoded example sensor choices.
- Include Unassigned for both primary positions.
- Allow editing temperature unit, primary assignments, display labels and stale-reading limits.
- Preserve secondary sensors and configuration fields outside this view.
- When a primary sensor is unassigned or replaced, retain its configuration as secondary.
- Reject assigning the same sensor to both primary positions.
- Disable label and stale-limit inputs for unassigned positions.
- Keep edits local until Save succeeds.
- Cancel or Escape discards unsaved changes.
- Prevent duplicate saves and prevent closing the view while a save request is pending.
- On success, close the view, return focus to the gear, and refresh presentation immediately.
- On failure, keep the draft open and show a clear error.
- On conflict, explain that another client changed settings and provide an explicit reload action. Do not automatically overwrite or discard the draft.

Sensor discovery and creating new sensor identities are deferred.

## Apply Changes Without Restart

Refactor the server’s presentation state to support replacing configuration safely while MQTT reception and HTTP requests continue.

- Apply units, labels, positions and freshness limits together as one configuration.
- Retain valid observations for sensor identities that remain configured.
- Remove observations for identities no longer configured.
- Newly configured sensors show unknown readings until a valid observation arrives.
- Do not reset observation timestamps or make old readings appear fresh.
- Keep time synchronization, presentation polling and MQTT reception operating throughout editing and saving.
- Other clients receive the updated presentation through their normal polling.

## LAN and Development Access

Settings must work through the existing server-hosted client from any machine on the LAN. Do not impose a loopback-only restriction or require browser access to MQTT or local files.

Support the established development client/API-address arrangement, including narrowly scoped CORS and preflight handling where required.

Require JSON update requests and validate browser origins for writes. Permit the served client’s origin and explicitly configured development origins; reject unrelated origins. Do not introduce a login system in this story.

## Deployment and Documentation

Ensure deployment leaves ProgramData settings intact. Bundled `presentation.json` is an initialization source only after this change.

Document:

- Production storage path and service-account permissions.
- First-run migration and default behavior.
- Development/test directory override.
- Save, conflict and error behavior.
- Runtime application without restart.
- Configuration preservation during deployment.

Prepare enduring documentation updates according to the repository’s acceptance and closeout workflow.

## Acceptance and Verification

Verify that:

- First run migrates existing settings exactly.
- Missing legacy and ProgramData files produce the documented default.
- Existing ProgramData settings take precedence over bundled settings.
- Invalid files and failed writes do not silently reset configuration.
- Successful saves survive service restart and deployment.
- Validation failures leave disk and active settings unchanged.
- Two clients editing the same revision produce a conflict for the second save.
- Secondary configuration survives primary-panel edits.
- Display changes apply without service restart and reach other clients.
- Cancel leaves configuration unchanged.
- Time and sensor operation continue during settings use.
- The UI remains usable at 1024×600.

Use isolated temporary directories for automated persistence tests. Run relevant existing client, server, hosting and MQTT checks. Report LAN, Windows-service and physical touchscreen checks that require human verification.

## Out of Scope

- LoRa Service management or configuration.
- MQTT broker or gateway settings.
- Sensor discovery, node renaming or measurement mapping.
- Windows time configuration.
- Authentication accounts, history or additional settings screens.

Leave the story active and changes uncommitted for human review. Human acceptance is required before story closeout, staging and the commit handoff.

## Accepted Result

Accepted by the human developer on October 26, 2026. Implemented service-owned
ProgramData settings, first-run migration/defaults, revision-protected atomic
saves, runtime configuration replacement, and the live settings editor.
Secondary configuration and observation timestamps are preserved as required.
Build, 67 server checks, client tests, hosting and MQTT integration checks,
documentation links, and diff checks passed. Codex did not independently perform
LAN browser, Windows-service, or physical touchscreen verification. No intentional
departures from the requested scope.
