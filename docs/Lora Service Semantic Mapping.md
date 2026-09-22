# LoRa Service Semantic Mapping

This is the specified mapping and logging contract for a future implementation
increment. The [current worker](../lora-service/docs/contract.md) decodes and logs
reports without configuration-driven semantic mapping or MQTT publishing.

## 1. Purpose

The LoRa Service translates the compact numeric representation used by the LoRa application protocol into semantic, application-friendly data.

LoRa airtime is a constrained resource, so the radio protocol uses compact numeric identifiers and scaled integer values. Resources on the IP network are not similarly constrained. Applications consuming data from the LoRa Service should therefore receive meaningful names, normalized numeric values, and units rather than requiring knowledge of the LoRa wire representation.

The semantic mapping is configuration-driven so that new sensor nodes and new measurement types can normally be introduced without modifying LoRa Service code.

[LoRa Protocol](LoRa%20Protocol.md) remains authoritative for assigned LoRa protocol identifiers and their immutable wire semantics. The mapping file is the machine-readable representation used by the LoRa Service.

## 2. Architectural Boundary

The LoRa Service owns translation from LoRa protocol representation to application semantics.

Conceptually:

```text
LoRa packet
    |
    | compact numeric protocol
    v
LoRa Service
    |
    | semantic mapping
    v
Normalized application data
    |
    v
MQTT
    |
    v
Applications
```

An application consuming MQTT data should not need [LoRa Protocol](LoRa%20Protocol.md) in order to understand a measurement.

Application-specific presentation policy is outside this mapping. For example, whether DenClock displays porch temperature on its front page belongs to DenClock configuration, not the LoRa Service.

## 3. Configuration File

The LoRa Service shall use a single JSON semantic mapping file.

The file contains three primary mapping domains:

- `nodes` — maps LoRa Node IDs to human-readable node identity.
- `measurements` — maps LoRa TLV Type IDs to measurement semantics and wire decoding information.
- `gateways` — maps Gateway IDs to human-readable gateway identity.

Example:

```json
{
  "gateways": {
    "1": {
      "name": "den-gateway",
      "description": "Den LoRa/Wi-Fi gateway"
    }
  },
  "nodes": {
    "1": {
      "name": "porch",
      "description": "Porch temperature and humidity sensor"
    }
  },

  "measurements": {
    "1": {
      "name": "temperature",
      "valueType": "int16",
      "length": 2,
      "scale": 0.01,
      "units": "C",
      "precision": 4
    },

    "2": {
      "name": "relativeHumidity",
      "valueType": "uint16",
      "length": 2,
      "scale": 0.01,
      "units": "percent",
      "precision": 4
    },

    "3": {
      "name": "batteryVoltage",
      "valueType": "uint16",
      "length": 2,
      "scale": 0.001,
      "units": "V",
      "precision": 4
    }
  }
}
```

## 3.1 Gateway Mapping

A gateway entry maps the numeric Gateway ID carried by Gateway Protocol HELLO
to installation-specific identity.

Each gateway entry initially contains

- `name` — short, stable, human-readable identity.
- `description` — optional descriptive text.

Gateway names are installation configuration and are not part of the LoRa wire protocol.

If a Gateway ID has no configured mapping, the service may use its decimal ID
as the fallback name (for example, Gateway ID 37 becomes `"37"`). The numeric
Gateway ID remains unchanged; no friendly-name mapping is required for this
fallback.

A gateway therefore needs to know only its numeric Gateway ID. Moving or repurposing a physical gateway may require only a mapping-file change rather than a firmware change.

Friendly gateway identity must not be embedded in the LoRa packet solely for application presentation.


## 4. Node Mapping

A node entry maps the numeric Source Node ID carried by the LoRa protocol to installation-specific identity.

Each node entry initially contains:

- `name` — short, stable, human-readable identifier.
- `description` — optional descriptive text.

For example:

```text
Node ID 1 -> porch
```

Node names are installation configuration and are not part of the LoRa wire protocol.

A sensor node therefore needs to know only its numeric Node ID. Moving or repurposing a physical sensor may require only a mapping-file change rather than a firmware change.

Friendly node identity must not be embedded in the LoRa packet solely for application presentation.

## 5. Measurement Mapping

Each measurement entry maps a numeric TLV Type to the information required to decode and describe that measurement.

The initial fields are:

### `name`

Semantic application-level name of the measurement.

Examples:

```text
temperature
relativeHumidity
batteryVoltage
```

### `valueType`

Numeric representation of the TLV Value on the wire.

The initial implementation needs only the numeric types required by the existing protocol:

```text
int16
uint16
```

Additional representations shall be added only when an actual protocol requirement exists.

### `length`

Expected TLV Value length in bytes.

A received TLV whose length does not agree with its configured definition is malformed.

### `scale`

Multiplier applied to the decoded wire value to obtain the normalized value.

For example:

```text
raw temperature = 2143
scale           = 0.01
normalized      = 21.43
```

### `units`

Canonical units of the normalized value.

Initial examples are:

```text
C
percent
V
```

Units describe the normalized measurement. They are not intended to dictate how an application must present the value.

For example, DenClock may eventually choose to display a temperature in degrees Fahrenheit even though the canonical value produced by the LoRa Service is degrees Celsius.

### `precision`

Recommended presentation precision expressed as significant figures.

The initial default is four significant figures.

Examples include:

```text
21.43 C
3.126 V
1013 hPa
```

Precision is metadata. The LoRa Service shall not discard numeric fidelity merely to satisfy the presentation precision.

An application remains free to choose a different presentation appropriate to its user interface.

## 6. Decoding

For a known measurement TLV, the LoRa Service shall:

1. Look up the TLV Type in the measurement mapping.
2. Verify the expected length.
3. Decode the Value using the configured numeric wire type and the byte-order rules defined by [LoRa Protocol](LoRa%20Protocol.md).
4. Apply the configured scale.
5. Associate the resulting numeric value with its semantic name and units.

For example, the wire representation:

```text
Node ID = 1
TLV Type = 1
Value    = 2143
```

may become semantically:

```text
Node        = porch
Measurement = temperature
Value       = 21.43
Units       = C
```

The exact MQTT representation of this semantic information is defined separately by the [MQTT interface contract](MQTT-INTERFACE.md).

## 7. Configuration Validation

The semantic mapping file shall be loaded and validated during LoRa Service startup.

Configuration errors that make correct interpretation of configured data impossible shall prevent service startup.

Diagnostics should identify the configuration item and reason for failure sufficiently to allow the configuration to be corrected.

Examples include:

- invalid numeric IDs;
- missing required fields;
- unsupported `valueType`;
- invalid lengths;
- invalid scaling values;
- duplicate or otherwise ambiguous definitions.

The MVP does not require runtime hot reload. Configuration changes may require restarting the LoRa Service.

## 8. Unknown Runtime Identifiers

An unknown Node ID or TLV Type received at runtime is not itself a service failure.

The LoRa Service shall handle unknown identifiers gracefully and generate an appropriate infrastructure warning.

Unknown TLVs remain skippable according to the Length field as required by [LoRa Protocol](LoRa%20Protocol.md).

This permits deployments and protocol additions to occur without causing the service to fail when it encounters data not yet represented in its local mapping.

## 9. Logging

Application code shall use the standard .NET `ILogger<T>` abstraction.

Serilog shall initially provide the logging implementation.

The initial sinks are:

- console output when running interactively;
- daily rolling log file for unattended operation.

Rolling log files shall be retained for seven days.

Logging is intended for infrastructure and operational diagnostics rather than measurement history.

Examples of information appropriate for logging include:

- service startup and shutdown;
- mapping/configuration load and validation;
- gateway connection, disconnection, and reconnection;
- MQTT connection, disconnection, and reconnection;
- malformed Gateway Protocol messages;
- malformed LoRa application messages;
- unknown Node IDs or TLV Types;
- unexpected exceptions and operational failures.

Routine valid sensor measurements shall not normally be logged.

Conceptually:

```text
Logs        -> What happened to the infrastructure?
MQTT        -> What is happening now?
App storage -> What happened to measurements over time?
```

## 10. Authority of the LoRa Protocol

[LoRa Protocol](LoRa%20Protocol.md) remains authoritative for LoRa wire protocol assignments and semantics.

Once a TLV Type has been defined, its protocol meaning, representation, units, length, and scaling are immutable as specified by the LoRa protocol.

The semantic mapping configuration shall not be used as a mechanism for redefining an existing wire identifier.

If the wire semantics of a measurement must change, a new TLV Type shall be assigned.

## 11. Out of Scope

The initial semantic mapping mechanism does not provide:

- hot configuration reload;
- a configuration editor or UI;
- a generalized schema or expression engine;
- plug-in measurement decoders;
- database storage;
- measurement history;
- alarm thresholds;
- graph ranges;
- DenClock display selection;
- unit preferences for individual users or applications;
- MQTT topic or payload definitions.

These features shall be introduced only when an actual requirement justifies them.

## 12. MVP Acceptance

The first implementation is complete when the existing Node 1 `SENSOR_REPORT` can be translated using the mapping file so that:

```text
Node 1 -> porch

TLV 1 -> temperature
TLV 2 -> relativeHumidity
TLV 3 -> batteryVoltage
```

and the three existing wire values are correctly decoded and normalized without hard-coded knowledge of those measurement types in the general LoRa Service processing path.

Invalid mapping configuration shall be detected at startup. Unknown runtime identifiers shall be handled gracefully.

The service shall use `ILogger<T>`, with Serilog providing console and daily rolling-file logging with seven-day retention.

No additional configuration framework is required for the MVP.
