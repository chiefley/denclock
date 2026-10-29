# Remove Semantic Mapping from LoRaService

## Objective

Make LoRaService independent of gateway and sensor naming conventions. Publish
numeric identifiers without interpreting physical locations or display names.
Semantic naming belongs to DenClock in Story 2.

## Requirements

- Publish integer gatewayId and nodeId; their pair uniquely identifies a sensor.
  Node numbers need only be unique within a gateway.
- Remove gateway/sensor name lookups and all semantic mapping configuration,
  loading, and validation. Do not introduce another naming mechanism.
- Publish one measurement message per report using numeric topic identifiers.
  Preserve measurement fields, units, and relevant metadata. No dual publishing
  or backward compatibility is required; the MQTT contract may break.
- Preserve gateway communication, sensor decoding, and validation. Do not add
  semantics to gateway firmware or make unrelated protocol changes.

## Acceptance criteria

- MQTT payloads contain numeric gatewayId and nodeId, with no generated sensor
  or gateway display names.
- Nodes with the same number on different gateways have distinct MQTT identity
  and retained topics.
- Operation requires no semantic mapping configuration.
- Existing measurements, units, and metadata are preserved.
- Updated tests cover numeric identifiers and duplicate node numbers across gateways.
- Documentation describes the revised MQTT contract.

## Scope and review

DenClock changes, semantic/display-name configuration, sensor UI, database work,
compatibility layers, and unrelated LoRa or firmware changes are out of scope.
Use the smallest implementation following existing conventions. Run relevant
tests and report changed files, final topic/payload with an example, test
results, and decisions Story 2 must accommodate. Leave active for human acceptance.

## Acceptance

Accepted by the developer on 2026-10-28. LoRaService publishes numeric gateway/node
identity on distinct retained topics and no longer loads semantic mapping. Fixed
measurement normalization preserves values, units, and metadata. No intentional
departures from the requested scope.

Verification: build passed with no warnings or errors; 254 protocol/session/TCP
checks and 29 normalization/MQTT checks passed, including separate gateways with
the same node number and real-host startup without a mapping file. Windows/NUC
deployment was not independently reverified. DenClock adaptation remains Story 2.
