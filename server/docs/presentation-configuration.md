# DenClock Presentation Configuration

The sensor presentation baseline is accepted in story 008. Runtime settings
loading/saving is accepted in [story 009](../../docs/stories/completed/009-load-save-and-apply-display-settings.md).

## Purpose

`presentation.json` configures how DenClock presents sensor readings. It belongs to DenClock and owns gateway/sensor semantic names. LoRaService supplies numeric identifiers only.

The LoRa Service supplies normalized measurements. DenClock selects sensors, assigns display positions and labels, converts temperature units for presentation, and identifies stale readings.

## File Location and Loading

Production uses `%ProgramData%\DenClock\Server\presentation.json`, resolving
ProgramData through Windows CommonApplicationData, independent of the working
directory. `Presentation:ConfigurationDirectory` explicitly overrides the directory
for development/tests; Development and non-Windows runs require the override.

Before accepting requests, load and validate the destination if present. If
absent, validate executable-local `presentation.json` and atomically copy its
exact contents without changing the source. If both are absent, atomically
create `{"temperatureUnit":"F","sensors":[]}`. Invalid existing destination
files fail startup without fallback. Invalid legacy files, permission failures,
and initialization failures also fail clearly with path diagnostics.

Once initialized, the destination is authoritative. Bundled files remain
initialization sources only and deployments must leave ProgramData intact.
The service account needs directory Modify permissions for same-directory
atomic replacement; see the [server deployment guide](../README.md).

The browser reads settings on each open and PUTs a revision and configuration.
Server validation and revision comparison precede persistence. Saves are
serialized, flushed to a temporary file in the same directory, and atomically
replace the destination before activation. Validation, conflict, and disk
failures preserve prior active settings and the file. The server logs disk
errors while returning a useful error without stack traces.

All fields apply together without restart. Existing configured identities retain
observations and their original timestamps; removed identities lose observations.
New identities start unknown. External file edits are not watched: use the API
for runtime changes, or stop the service before manual edits and restart it.

## Example

```json
{
  "temperatureUnit": "F",
  "gateways": [{"gatewayId": 1, "name": "house", "displayName": "House gateway"}],
  "sensors": [
    {
      "gatewayId": 1,
      "nodeId": 1,
      "name": "den",
      "displayName": "Den",
      "position": "upperLeft",
      "maxAgeSeconds": 60
    },
    {
      "gatewayId": 2,
      "nodeId": 1,
      "name": "porch",
      "displayName": "Outside",
      "position": "upperRight",
      "maxAgeSeconds": 60
    },
    {
      "gatewayId": 1,
      "nodeId": 3,
      "name": "attic",
      "displayName": "Attic",
      "position": "secondary",
      "maxAgeSeconds": 300
    }
  ]
}
```

These names and age limits are illustrative. The deployed configuration must use the installation’s actual numeric sensor identities and intended freshness limits.

## Configuration Fields

All fields shown below are required.

| Field | Type | Meaning |
|---|---|---|
| `temperatureUnit` | String | `"C"` or `"F"`; applies to all displayed temperatures. |
| `sensors` | Array | Sensors selected for presentation. May be empty. |
| `gatewayId` | Integer | Gateway ID 1–255; together with nodeId uniquely identifies a sensor. |
| `nodeId` | Integer | Node ID 1–239; unique within its gateway. |
| `name` | String | Nonempty stable DenClock logical name, unique across configured sensors. |
| `displayName` | String | Nonempty user-facing label, such as “Den” or “Outside.” |
| `position` | String | `"upperLeft"`, `"upperRight"`, or `"secondary"`. |
| `maxAgeSeconds` | Number | Positive, finite maximum age of an observation before it becomes stale. |

Field names and enumerated values are case-sensitive. Sensor identity matching is exact and case-sensitive.

Each primary position may be assigned to at most one sensor. Multiple sensors may be assigned to `secondary`. Either primary position may remain unassigned.

Wrong field types, missing required fields, duplicate sensor identities, duplicate primary-position assignments, and unsupported enumerated values are configuration errors.

## Sensor Identity

Sensor identity is the pair `(gatewayId, nodeId)`, matching the integer MQTT
payload fields and `lora/sensors/{gatewayId}/{nodeId}`. Topic and payload must
agree. Reports from unconfigured sensors are ignored; discovery is not implemented.
Users can add numeric identities manually in the existing settings dialog.
No permanent names are invented from incoming reports.

`name` is the DenClock logical name, independent of MQTT topics. `displayName`
is the client label. Both can change without losing an observation for the same
numeric identity. Changing numeric identity starts with no observation; removing
and re-adding it also clears its observation.

The optional `gateways` array contains entries with required integer `gatewayId`
(1–255), and optional `name` and `displayName`. Optional names may be omitted or
null; when supplied they must be nonempty strings. Gateway IDs and supplied
logical names must be unique within this array. A sensor does not require a
gateway-name entry. Gateway names are configuration metadata exposed through the
settings API and editor; they do not alter MQTT routing or sensor identity.

### One-time configuration adjustment

Stop DenClock before upgrading an installation with the old presentation format.
In its authoritative ProgramData presentation.json, change each sensor's
`internalName` field to `name`, and add its actual numeric `gatewayId` and
`nodeId`. Preserve labels, positions, stale limits, unit choice, and other fields.
Restart with the new build. No automatic ID inference or old-format compatibility
is provided. Invalid existing settings fail startup with a path diagnostic.
The bundled example uses Gateway 1 / Node 1 for Den and Gateway 1 / Node 2 for
Back Porch; confirm those identities match the installation.

DenClock consumes normalized MQTT measurements; it does not decode LoRa packets.

## Display Positions

`upperLeft` and `upperRight` identify the two privileged temperature/humidity panels on the main clock page.

Each panel presents:

- Temperature, with prominent whole-degree digits and smaller tenths.
- The configured sensor display name beneath the temperature.
- A subordinate humidity reading.

An unassigned primary position displays unknown readings and no sensor label. An assigned sensor with no available readings retains its configured label.

`secondary` identifies sensors intended for a later secondary-sensor view. That view and its navigation are outside this MVP.

Positions are named layout assignments, not pixel coordinates. Fonts, sizes, colors, and exact placement belong to the page’s styling.

## Temperature Units

The incoming MQTT temperature measurement remains in Celsius.

For `"C"`, display Celsius directly. For `"F"`, convert using:

```text
Fahrenheit = Celsius × 1.8 + 32
```

Perform conversion before rounding. Display temperature to one decimal place and identify the selected unit.

Conversion affects presentation only. It must not alter sensor transmissions, the MQTT contract, or the canonical received value.

The fractional part is smaller than the whole-degree digits but uses the same color and contrast.

## Unknown Readings

Before a valid observation is available:

- Temperature displays `--.-`.
- Humidity displays `--`.

Missing or invalid measurements must not be replaced with zero or fabricated values. Temperature and humidity availability are evaluated independently.

A valid report that omits a measurement leaves that measurement unknown for that report; it must not make an older measurement appear newly received.

## Freshness and Stale Readings

Freshness is based on the MQTT report’s `receivedUtc`, which represents LoRa Service reception time.

```text
ageSeconds = current UTC time − receivedUtc
stale = ageSeconds >= maxAgeSeconds
```

DenClock uses server-derived time for this decision. A browser’s local timezone or wall-clock setting must not determine freshness.

Receiving a retained or duplicate MQTT message does not reset the observation’s age. The original `receivedUtc` remains authoritative.

Freshness continues to age when no new messages arrive, including during MQTT or server connectivity interruptions.

When a known reading becomes stale:

- Keep the last-known value visible.
- Display stale temperature and humidity values in red.
- Keep the temperature tenths the same color as the whole-degree digits.

A subsequent valid, fresh observation restores normal presentation.

An invalid or missing `receivedUtc` must not establish a fresh observation.

## Readability Requirements

The main page has two reading distances:

- Across the room: time and whole-degree temperature digits.
- Up close: day/date, sensor display names, humidity, and temperature tenths.

Time and whole-degree temperatures should be bold and high contrast, targeting approximately one inch of visible digit height on the 10.1-inch, 1024×600 display. Actual readability must be checked on the shelf display.

Time moves lower on the page and becomes bolder and somewhat smaller than the existing clock presentation. The privileged temperatures occupy the upper-left and upper-right areas.

## Out of Scope

- Clicking temperature panels for diagnostics or additional information.
- Secondary-sensor views and navigation.
- External file watching and hot reload.
- Arbitrary layout coordinates or configurable typography.
- Measurement history, charts, alarms, or equipment control.
- Changes to the LoRa protocol or MQTT measurement units.

## Implemented Validation Details

Duplicate JSON properties, duplicate numeric sensor pairs, and duplicate logical
sensor names are rejected. Logical names are independent of MQTT topic syntax. Extra configuration properties are
preserved through API round trips and browser primary edits; they do not affect
current presentation. Required field names remain exact and case-sensitive.

Reports require an explicit UTC timestamp ending in Z or +00:00. Invalid or future
timestamps are rejected, leaving the prior observation to age normally. Equal
or older timestamps do not replace newer observations. A timestamp equal to the
last accepted timestamp is treated as a duplicate regardless of sequence.

Temperature requires canonical C units and a finite numeric value. Humidity
requires percent units and a finite value from 0 to 100. Invalid measurements
are independently unavailable in an otherwise valid report.

The no-source first-run default selects F with no sensors. The bundled file may
contain installation-specific identities; migration preserves those values.
Sensor discovery remains deferred. The existing settings dialog supports manual
add/edit/remove of sensor mappings and optional gateway names using the same
revision-protected Save/Cancel workflow.
