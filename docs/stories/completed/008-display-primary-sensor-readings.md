# Display Primary Sensor Readings

## Story

As a DenClock user heating the den with a wood-burning fireplace insert, I want
the den and outside temperatures prominently visible alongside the time so that
I can monitor the conditions that matter most without approaching the display.

Implement the first DenClock sensor presentation using the existing semantic
MQTT reports. This story spans the server and browser client. The committed
presentation configuration specification is authoritative; do not redesign it.

## Governing References

- [Presentation configuration](../../../server/docs/presentation-configuration.md)
- [MQTT interface](../../MQTT-INTERFACE.md)
- [Server contract](../../../server/docs/contract.md)
- [Client contract](../../../client/docs/contract.md)
- [Client decisions](../../../client/docs/decisions.md)
- [Architecture](../../ARCHITECTURE.md)
- [Development workflow](../../DEVELOPMENT.md)

## Existing Baseline

The Windows NUC hosts DenClock, the independent LoRa Service, and Mosquitto.
The LoRa Service publishes semantic sensor reports on lora/sensors/{node-name},
QoS 1 and retained. Mosquitto listens on 127.0.0.1:1883; live reports, retained
persistence, and restart/reboot recovery have been verified by the developer.

DenClock currently serves the time API and static browser clock. It does not
yet subscribe to MQTT or expose sensor readings to the client.

The target display is 10.1 inches diagonal at 1024 x 600. The cabin background,
date/day display, smooth time advancement, and clock synchronization recovery
must be preserved while adjusting the layout.

## Responsibilities and Data Flow

LoRa Service -> MQTT broker -> DenClock server -> browser client.

The DenClock server owns presentation configuration, MQTT subscription, and
the latest canonical observations. The browser owns layout, temperature-unit
conversion, formatting, and ongoing visual freshness updates. The browser never
connects to MQTT or reads a local configuration file.

Do not modify sensor firmware, the gateway, the LoRa Service, or their protocols.

## Presentation Configuration

Implement server/docs/presentation-configuration.md as written:

- Load presentation.json beside the executable at startup, regardless of the
  working directory; include it in build and publish output.
- Validate required fields, types, exact enumerated values, unique sensor names,
  primary-position uniqueness, and positive finite maxAgeSeconds.
- Missing or invalid configuration fails startup with an actionable diagnostic.
- Changes require server restart.
- internalName matches MQTT node.name and the topic suffix exactly.
- displayName and position govern presentation; neither renames the MQTT node.
- Support C and F as the display-wide temperatureUnit.
- Support secondary assignments in configuration without creating their view.

Supply a usable initial file with temperatureUnit F and the known porch sensor
labelled Outside in upperRight, with maxAgeSeconds 60. Leave upperLeft unassigned
unless the developer supplies the actual den sensor identity. Do not invent a
live den sensor or map the same physical sensor to both roles. Document how to
assign the den sensor when available; tests may use synthetic sensor identities.

## Server MQTT Reception

- Add an independent hosted subscriber to the existing DenClock server.
- Configure broker host, port, and client ID through ordinary server settings.
  Default to localhost:1883 and a client ID distinct from the LoRa Service's
  lora-service identity, such as denclock-server.
- Subscribe to lora/sensors/+ at QoS 1, including retained observations.
- Reconnect and restore subscriptions automatically after broker loss.
- Broker unavailability must not prevent the time API, page hosting, or
  configured unknown-reading presentation from operating.
- Keep the latest observations in memory; no database or offline history.
- Ignore unconfigured sensors and reject topic/payload identity disagreements.
- Validate the semantic message type, timestamp, and relevant measurements.
  Do not infer units from raw protocol identifiers or convert wire packets.
- Treat temperature and humidity independently. Invalid/missing measurements
  are unknown; a valid newer partial report replaces the prior observation
  without carrying forward missing measurements under the new timestamp.
- Invalid envelopes must not overwrite a good observation or terminate the
  subscriber. Use infrastructure diagnostics for malformed reports and outages.
- Preserve receivedUtc. Duplicate delivery must not refresh an observation.
  A delayed older report must not replace a newer observation. Sequence numbers
  are not globally unique and must not be used alone as ordering/deduplication keys.
- Keep MQTT handling independent of the time endpoint and graceful service shutdown.

## Server-to-Browser Interface

Add GET /api/presentation, returning one coherent snapshot of both primary
positions and their configured labels, identities, age limits, and observations,
plus temperatureUnit and a server UTC timestamp sampled for the response.
Keep canonical temperature values in Celsius and expose explicit unavailable
values rather than fabricated defaults. Document the implemented JSON shape
in the server contract.

- Return Cache-Control: no-store.
- Return configured slots even before MQTT supplies any observations.
- Do not expose broker credentials or unrelated diagnostic payloads.
- Preserve the existing production same-origin hosting and XPS development
  workflow. Apply the existing exact-origin CORS policy to the new endpoint.
- Reuse the configured server address for browser requests; do not introduce a
  separate hard-coded API host.

The browser requests a snapshot immediately and then every five seconds through
a single polling loop, with a five-second timeout and no overlapping requests.
Keep these timings in client configuration, not per-sensor presentation entries.
HTTP errors and malformed responses preserve the last good snapshot and allow retry.
No WebSockets, SSE, or browser MQTT dependency is required.

## Freshness During Interruptions

Follow the configured maxAgeSeconds and the original receivedUtc. Re-fetching a
retained reading or an unchanged API snapshot must not make that reading younger.

Use server-derived UTC anchored to monotonic browser elapsed time, independently
of the browser wall clock/timezone. Continue recomputing age between HTTP requests
and during failed requests. After a background-tab pause, recompute elapsed age
rather than counting timer ticks.

- At age >= maxAgeSeconds, keep known values but render them in red.
- Fresh subsequent observations restore the normal color.
- Unknown temperature is --.- and unknown humidity is --.
- Missing/invalid timestamps never establish a fresh observation.
- An assigned sensor retains its display label while readings are unknown.
- An unassigned primary slot has unknown readings and no sensor label.

## Main Display

Place the two privileged panels in the upper-left and upper-right portions of
the clock page. Move the time downward; make its main digits bolder and somewhat
smaller than the current presentation.

There are two reading distances:

- Across the room: time and whole-degree temperature digits.
- Up close: day/date, sensor display names, humidity, and temperature tenths.

Target approximately one inch of visible digit height for the principal time
and whole-degree temperature digits, using bold, high-contrast typography.
Nominal CSS font size is not visible glyph height. Final sizing and balance must
be assessed on the real shelf display; a screenshot alone cannot establish
across-room readability.

Temperature panels:

- Whole-degree digits approximately match the main time digits in size.
- Show one decimal place, with the tenths smaller but the same color and contrast.
- Convert C to F before rounding when configured, using F = C * 1.8 + 32.
- Handle negative values and rounding across whole-degree boundaries correctly;
  format the complete rounded number before separating the main/fractional parts.
- Show the selected unit without competing with the principal digits.
- Place a smaller display name and humidity value beneath the temperature.
- Stale humidity and all temperature digits, including tenths, use red.
- Panels are display-only with no click interaction.

Preserve the cabin background and keep all content legible and unclipped at
1024 x 600, including negative temperatures, three-digit Fahrenheit values,
unknown placeholders, and the supported day/date display.

## Acceptance and Verification

1. Valid configuration loads from the executable directory in development and
   published operation; malformed/missing configuration fails clearly.
2. Both primary assignments, either unassigned slot, empty sensor lists, and
   secondary assignments behave according to the specification.
3. A real MQTT report for porch reaches the DenClock API and Outside panel.
   A second synthetic report can verify independent left/right assignments
   until the physical den sensor is available.
4. Fahrenheit and Celsius render correctly: 0 C -> 32.0 F, 22.15 C -> 71.9 F,
   and -40 C -> -40.0 F. Test negative fractions and decimal carry boundaries.
5. Temperature/humidity missing-value handling, identity mismatches, malformed
   payloads, unknown sensors, duplicate reports, and older reports are covered.
6. Retained old observations are stale immediately when appropriate. Values
   transition to stale at the threshold without new MQTT or HTTP responses.
   Fresh reports restore normal appearance.
7. Browser wall-clock/timezone differences do not change sensor age or clock
   authority. Poll timeouts recover without overlapping requests.
8. Broker stop/start and server restart recover automatically. Time continues
   through sensor/MQTT faults, and browser data requests recover after service loss.
9. Existing clock and hosting tests pass; new focused tests cover configuration,
   MQTT subscription/state, API/CORS, units, and freshness. Use local fixtures or
   a test peer for automation rather than requiring the production broker.
10. Build/publish includes presentation.json and all new client assets while
    preserving existing exclusions and full-output deployment practice.
11. Inspect a 1024 x 600 browser rendering. The developer verifies physical
    readability and balance on the shelf and the equivalent laptop page.
12. Update affected contracts/READMEs and the specification's implementation
    status after implementation; report automated versus manual results clearly.

## Exclusions

Secondary-sensor screens/navigation; panel clicks; RSSI/battery diagnostics;
history/charts; alarms; equipment control; hot reload; configuration editors;
arbitrary layout configuration; changes to sensor/MQTT canonical units; broker
deployment; and previously deferred deployment-preservation changes.

## Handoff

Implement against this story, report changes and verification, and leave work
uncommitted for developer review. Do not claim physical readability or hardware
checks without developer confirmation. Human acceptance is required before
moving this story to completed and preparing the staged commit handoff.

## Acceptance and Verification

Verified and accepted by the human developer after implementation and shelf/NUC
review.

DenClock now loads presentation configuration, subscribes to semantic MQTT
reports, and serves the two primary slots to the browser. The display converts
temperature units, renders the agreed hierarchy, and marks aging observations
stale even during connectivity loss. Secondary views and panel interactions
remain deferred.

Verification passed: 44 server configuration/state checks, 14 JavaScript tests,
hosting and MQTT integration checks, Windows x64 publishing, and 1024 x 600
browser layout checks. The developer subsequently confirmed the story verified.
No changes to the sensor, gateway, or LoRa Service were required.
