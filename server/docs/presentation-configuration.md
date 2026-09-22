# DenClock Presentation Configuration

Status: Agreed specification; not yet implemented.

## Purpose

`presentation.json` configures how DenClock presents sensor readings. It belongs to DenClock, separately from the LoRa Service’s semantic mapping.

The LoRa Service supplies normalized measurements. DenClock selects sensors, assigns display positions and labels, converts temperature units for presentation, and identifies stale readings.

## File Location and Loading

The file resides beside the DenClock server executable and is loaded at startup, independently of the process’s working directory.

Configuration changes require restarting the DenClock server. Hot reload and browser-based configuration editing are outside this MVP.

A missing file or invalid configuration prevents successful server startup and produces a clear diagnostic identifying the problem.

## Example

```json
{
  "temperatureUnit": "F",
  "sensors": [
    {
      "internalName": "den",
      "displayName": "Den",
      "position": "upperLeft",
      "maxAgeSeconds": 60
    },
    {
      "internalName": "porch",
      "displayName": "Outside",
      "position": "upperRight",
      "maxAgeSeconds": 60
    },
    {
      "internalName": "attic",
      "displayName": "Attic",
      "position": "secondary",
      "maxAgeSeconds": 300
    }
  ]
}
```

These names and age limits are illustrative. The deployed configuration must use the installation’s actual MQTT sensor names and intended freshness limits.

## Configuration Fields

All fields shown below are required.

| Field | Type | Meaning |
|---|---|---|
| `temperatureUnit` | String | `"C"` or `"F"`; applies to all displayed temperatures. |
| `sensors` | Array | Sensors selected for presentation. May be empty. |
| `internalName` | String | Nonempty sensor identity matching the MQTT node name. Must be unique within the array. |
| `displayName` | String | Nonempty user-facing label, such as “Den” or “Outside.” |
| `position` | String | `"upperLeft"`, `"upperRight"`, or `"secondary"`. |
| `maxAgeSeconds` | Number | Positive, finite maximum age of an observation before it becomes stale. |

Field names and enumerated values are case-sensitive. Sensor identity matching is exact and case-sensitive.

Each primary position may be assigned to at most one sensor. Multiple sensors may be assigned to `secondary`. Either primary position may remain unassigned.

Wrong field types, missing required fields, duplicate sensor identities, duplicate primary-position assignments, and unsupported enumerated values are configuration errors.

## Sensor Identity

`internalName` matches both:

- The MQTT payload’s `node.name`.
- The node-name suffix in `lora/sensors/{node-name}`.

A report whose topic and payload identities disagree must not update a displayed sensor. Reports from sensors absent from the configuration are ignored.

`displayName` is a DenClock presentation label. It does not rename the MQTT node or change the LoRa Service’s mapping.

DenClock consumes the semantic MQTT interface; it does not decode LoRa packets.

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
- Browser-based configuration editing or hot reload.
- Arbitrary layout coordinates or configurable typography.
- Measurement history, charts, alarms, or equipment control.
- Changes to the LoRa protocol or MQTT measurement units.
