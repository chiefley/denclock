# Implement LoRa Service Semantic Mapping and MQTT Publishing

## Story

Implement the already-specified semantic-mapping and MQTT-publishing capability in the LoRa Service.

The existing LoRa Service receives Gateway Protocol `RX_PACKET` messages, assigns the authoritative UTC reception timestamp, parses the LoRa application protocol, and decodes `SENSOR_REPORT` messages.

The next increment shall extend that working receive path so that valid `SENSOR_REPORT` messages are translated through the configured semantic mapping and published through the MQTT interface already defined by the repository contracts.

This is an implementation story, not an architecture or deployment story.

The intended path after this increment is:

```text
Remote Sensor
    |
    | LoRa
    v
Gateway
    |
    | Gateway Protocol / TCP
    v
LoRa Service
    |
    | LoRa protocol parsing
    v
Semantic Mapping
    |
    | normalized semantic representation
    v
MQTT Publisher
    |
    v
Configured MQTT Broker
```

The LoRa Service shall become capable of publishing to a correctly configured MQTT broker.

Installing, configuring, or deploying that broker is explicitly outside this story.

## Governing Contracts

Treat the following documents as authoritative:

- `docs/LoRa Protocol.md`
- `docs/Gateway Protocol.md`
- `docs/LoRa Service.md`
- `docs/Lora Service Semantic Mapping.md`
- `docs/MQTT-INTERFACE.md`
- relevant existing LoRa Service implementation contracts and decisions

Do not redefine behavior already specified by those documents.

If implementation exposes a genuine ambiguity or contradiction in the accepted contracts, stop and report it for a human decision rather than inventing policy in code.

## Preserve the Existing Receive Path

The currently accepted Gateway Protocol and LoRa receive behavior shall remain intact.

In particular:

- Gateway TCP framing behavior shall not change.
- Gateway `HELLO` behavior shall not change.
- Gateway IDs remain properties of Gateway Protocol connections.
- `RX_PACKET` continues to carry raw LoRa bytes plus RSSI and SNR.
- The LoRa Service continues to assign the authoritative UTC reception timestamp.
- LoRa application parsing remains the responsibility of the LoRa Service.
- Existing LoRa and Gateway wire protocols shall not change.
- `COMMAND`, `RESPONSE`, `EVENT`, and application transmission behavior remain outside this story.

Do not move application-protocol interpretation into the gateway.

## Semantic Mapping Configuration

Implement the JSON semantic mapping specified by:

`docs/Lora Service Semantic Mapping.md`

The service shall load the mapping during startup.

The mapping shall support the currently specified domains:

```text
gateways
nodes
measurements
```

The implementation shall support the currently required measurement definition fields, including:

```text
name
valueType
length
scale
units
precision
```

The initial required wire value types are only:

```text
int16
uint16
```

Do not introduce a generalized schema, expression engine, plug-in decoder architecture, or other speculative extensibility.

## Configuration Validation

Validate the semantic mapping during service startup.

Configuration errors that prevent correct interpretation shall prevent successful startup and produce a useful diagnostic.

Validation shall cover the requirements established by the semantic-mapping contract, including applicable cases such as:

- invalid numeric identifiers;
- missing required fields;
- unsupported `valueType`;
- invalid measurement lengths;
- invalid scale values;
- duplicate or ambiguous definitions;
- mapping definitions inconsistent with the authoritative LoRa wire protocol.

The semantic mapping is not permitted to redefine an existing LoRa Type.

For the currently assigned measurement Types, the configured representation, length, scaling, and canonical units must agree with `docs/LoRa Protocol.md`.

No runtime hot reload is required.

## Semantic Normalization

For each known measurement TLV in a valid `SENSOR_REPORT`, the LoRa Service shall:

1. locate its semantic mapping;
2. validate its expected length;
3. decode the configured numeric representation using the LoRa protocol byte-order rules;
4. apply the configured scale;
5. associate the result with its semantic measurement name;
6. associate its canonical units;
7. preserve its configured presentation-precision metadata without rounding away numeric fidelity.

The general processing path shall not contain hard-coded special cases for:

```text
temperature
relativeHumidity
batteryVoltage
```

Their semantics shall come from the mapping configuration.

The implementation may contain protocol validation necessary to ensure that configured definitions agree with the immutable LoRa protocol contract.

## Gateway Mapping

Resolve Gateway IDs using the semantic mapping contract.

For a configured Gateway ID, use its configured friendly name.

For an unmapped Gateway ID, use the fallback behavior defined by the current semantic-mapping contract.

The numeric Gateway ID shall always remain available in the normalized message.

An unmapped gateway shall not cause an otherwise valid sensor report to be discarded solely because a friendly mapping is absent.

Generate the appropriate infrastructure warning.

## Node Mapping

Resolve Source Node IDs using the semantic mapping contract.

For a configured Node ID, use its configured friendly name.

For an unmapped Node ID, use the MQTT contract fallback:

```text
node-{id}
```

For example:

```text
Node ID 37 -> node-37
```

The numeric Node ID shall remain in the normalized message.

Publish an otherwise valid report using the fallback node identity and generate the appropriate infrastructure warning.

## Unknown Measurement Types

An unknown TLV Type shall not cause an otherwise valid `SENSOR_REPORT` to be discarded.

The service shall:

- skip the TLV using its Length field;
- generate the appropriate infrastructure warning;
- continue processing known valid TLVs in the report;
- publish the report containing the measurements that were successfully interpreted.

Do not invent semantic names, units, or representations for unknown Types.

## MQTT Publishing

Add MQTT publishing to the LoRa Service according to:

`docs/MQTT-INTERFACE.md`

The implementation shall support the initial `SENSOR_REPORT` publication contract only.

One received `SENSOR_REPORT` produces one MQTT message.

The topic shall be:

```text
lora/sensors/{node-name}
```

For example:

```text
lora/sensors/porch
```

The MQTT message shall use:

```text
QoS:      1
Retained: true
```

Do not introduce additional sensor topics or split individual measurements into separate MQTT messages.

## MQTT Payload

Publish the semantic JSON representation defined by `docs/MQTT-INTERFACE.md`.

The payload shall include the contract-defined information for:

- gateway identity;
- node identity;
- `messageType`;
- sequence number;
- authoritative LoRa Service reception timestamp;
- radio metadata;
- normalized measurements.

The initial semantic message type is:

```text
sensor-report
```

The original raw LoRa packet shall not be included in normal MQTT sensor messages.

MQTT consumers shall not need knowledge of either the LoRa application wire protocol or Gateway Protocol to interpret the published sensor report.

JSON serialization shall preserve numeric values as numbers rather than converting measurement values to presentation strings.

## MQTT Configuration

Provide the LoRa Service configuration necessary to identify and connect to an MQTT broker as specified by the MQTT contract.

Use the smallest configuration required by the accepted contract and existing project conventions.

Do not add requirements for:

- TLS;
- authentication;
- certificate management;
- broker installation;
- broker user administration;
- DenClock-specific configuration.

Those concerns are outside this implementation increment unless already required by an authoritative contract.

## MQTT Connection and Failure Behavior

Implement the failure/recovery behavior already defined by `docs/MQTT-INTERFACE.md`.

MQTT broker unavailability shall not invalidate the Gateway Protocol receive service.

The implementation shall support the contract-defined reconnect behavior and infrastructure diagnostics.

Do not add:

- offline message queues;
- local spooling;
- databases;
- replay mechanisms;
- dead-letter queues;
- guaranteed delivery across broker outages.

Loss of an individual sensor report while MQTT is unavailable is acceptable under the current contract.

## Logging

Application code shall continue to use:

```text
ILogger<T>
```

Implement the logging policy defined by the semantic-mapping and MQTT contracts.

Use Serilog underneath the standard .NET logging abstraction with:

- console logging for interactive operation;
- daily rolling file logging for unattended operation;
- seven-day log retention.

Logs are for infrastructure and operational diagnostics.

Appropriate events include:

- service startup and shutdown;
- semantic mapping load and validation;
- gateway connection/disconnection;
- MQTT connection/disconnection/reconnection;
- malformed protocol messages;
- unknown Gateway IDs;
- unknown Node IDs;
- unknown TLV Types;
- configuration errors;
- unexpected operational failures.

Routine successful sensor measurements shall no longer normally be emitted as operational log entries merely to expose measurement data.

MQTT becomes the normal path for valid sensor observations.

## Architecture and Implementation Structure

Preserve the existing separation of concerns.

The implementation should make the processing stages apparent:

```text
Gateway transport
        |
LoRa protocol interpretation
        |
Semantic normalization
        |
MQTT publication
```

Do not make MQTT code responsible for decoding LoRa wire packets.

Do not make LoRa parsing dependent upon MQTT.

Do not introduce DenClock-specific behavior into the LoRa Service.

Use straightforward interfaces/classes where they provide useful test seams, but do not introduce speculative abstractions merely to anticipate future message classes or brokers.

## Automated Verification

Add or update automated tests sufficient to verify the new behavior without requiring installation of the production MQTT broker on the NUC.

Tests shall cover, as applicable:

- valid semantic mapping load;
- invalid mapping startup failure;
- mapping consistency with authoritative LoRa definitions;
- signed `int16` decoding;
- unsigned `uint16` decoding;
- big-endian interpretation;
- configured scaling;
- canonical units;
- precision metadata;
- configured gateway mapping;
- unmapped gateway fallback;
- configured node mapping;
- `node-{id}` fallback;
- unknown measurement skipping;
- continued processing after an unknown TLV;
- MQTT topic construction;
- semantic JSON payload construction;
- numeric JSON measurement values;
- Gateway ID/name in payload;
- Node ID/name in payload;
- sequence preservation;
- authoritative reception timestamp preservation;
- RSSI preservation;
- SNR normalization/preservation according to the existing contracts;
- one report producing one MQTT publication;
- QoS 1;
- retained publication;
- broker-unavailable behavior;
- reconnect behavior where practical at the appropriate test level;
- existing Gateway Protocol behavior;
- existing LoRa Protocol behavior.

Existing tests shall continue to pass.

Prefer deterministic unit/component tests over tests dependent on external infrastructure.

Do not require the developer to install Mosquitto merely to run the normal automated test suite.

## Documentation

Update implementation-facing documentation necessary to describe the capability that now exists.

In particular, after implementation:

- documentation shall no longer describe semantic mapping as merely future functionality;
- documentation shall no longer describe MQTT publishing as unimplemented;
- the LoRa Service README shall describe required mapping and MQTT configuration;
- implementation status shall accurately distinguish implemented MQTT capability from broker installation/deployment and end-to-end broker acceptance;
- enduring architecture and protocol contracts shall not be rewritten unnecessarily.

Do not revise completed historical stories to make them describe the new implementation.

## Out of Scope

This story does **not** include:

- installing Mosquitto;
- configuring a production Mosquitto instance;
- changing Windows firewall rules for MQTT;
- deploying the updated LoRa Service to the NUC;
- configuring DenClock as an MQTT subscriber;
- modifying DenClock;
- modifying gateway firmware;
- modifying sensor firmware;
- modifying either wire protocol;
- measurement-history storage;
- database storage;
- MQTT offline spooling;
- MQTT replay;
- TLS or MQTT authentication;
- `COMMAND`;
- `RESPONSE`;
- `EVENT`;
- `TX_PACKET` application behavior;
- remote antenna control;
- multi-gateway duplicate suppression;
- generalized message routing infrastructure.

Those capabilities require separate stories when needed.

## Hardware Acceptance

No new hardware behavior is required for acceptance of this story.

Existing sensor and gateway hardware behavior shall remain unchanged.

A later integration/deployment story will install/configure the MQTT broker, deploy this LoRa Service version, and demonstrate a real sensor report through the complete path.

## Acceptance Criteria

This story is complete when:

1. The LoRa Service loads and validates the specified semantic mapping at startup.
2. Invalid mapping that prevents correct interpretation causes startup failure with a useful diagnostic.
3. Existing Node 1 `SENSOR_REPORT` data can be semantically mapped without hard-coded measurement-specific behavior in the general processing path.
4. Gateway, node, and measurement identities follow the accepted mapping contracts, including their defined fallback behavior.
5. Known measurement values are correctly decoded and normalized from mapping configuration.
6. Unknown measurement Types are skipped without discarding other valid measurements in the report.
7. A valid `SENSOR_REPORT` produces exactly one semantic MQTT publication.
8. The publication uses `lora/sensors/{node-name}`, QoS 1, and `Retained = true`.
9. The JSON payload conforms to `docs/MQTT-INTERFACE.md`.
10. MQTT broker unavailability does not terminate the Gateway Protocol receive service.
11. No offline spool, replay queue, database, or other unrequested reliability mechanism is introduced.
12. Application code uses `ILogger<T>` with the specified Serilog console and seven-day daily rolling-file behavior.
13. Routine successful measurements use MQTT as their normal output rather than operational logging.
14. Existing Gateway Protocol and LoRa Protocol behavior remains unchanged.
15. Existing automated tests continue to pass and appropriate new tests cover semantic mapping and MQTT publication.
16. The normal automated test suite does not require a production Mosquitto installation.
17. No gateway, sensor, DenClock, or wire-protocol behavior is changed.
18. Implementation-facing documentation accurately describes the new capability without claiming that Mosquitto has been installed/configured or that the new path has been deployed and hardware-accepted on the NUC.

## Implementation Report

When finished, report:

- files changed;
- implementation structure;
- mapping configuration added;
- MQTT configuration added;
- MQTT library/dependency selected and why;
- semantic normalization behavior implemented;
- MQTT publication behavior implemented;
- logging changes;
- tests added or changed;
- total automated tests executed and passed;
- build/publish verification performed;
- any contract ambiguity discovered;
- anything intentionally left for the broker installation/integration story.

Do not install or configure Mosquitto.

Do not deploy to the NUC.

Do not commit or push.

Leave all changes for developer review and acceptance.

## Acceptance and verification

The developer verified and accepted this story as complete. The LoRa Service
now validates semantic mapping at startup, normalizes sensor reports, publishes
retained QoS 1 MQTT JSON, and uses Serilog infrastructure logging with daily files
and seven-day retention. Existing receive behavior is preserved.

All 312 automated checks passed. Build and framework-dependent Windows publish
succeeded; console startup, unavailable-broker operation, and graceful shutdown
were verified. No contract departures were required.

Broker installation/configuration, NUC deployment of this increment, and real-sensor
MQTT integration remain separate work, as specified by this story.
