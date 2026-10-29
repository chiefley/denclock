# Story 2 — Move Gateway and Sensor Semantic Mapping into DenClock

## Objective

Update DenClock to consume the new numeric MQTT identifiers introduced in Story 1 and perform all gateway and sensor semantic mapping within DenClock.

Preserve and adapt the existing client-side configuration UI, which already allows the user to configure sensor mappings.

The result should be a complete, working system with semantic mapping removed from LoRaService.

## Architecture

Responsibilities:

- **LoRa gateway firmware:** Receives sensor transmissions and forwards the packets, identifying the gateway numerically.
- **LoRaService:** Decodes sensor packets and publishes numeric gateway IDs, node IDs, and measurements to MQTT.
- **DenClock service:** Consumes MQTT measurements, resolves numeric identities to configured semantic names, persists mappings, and exposes the relevant configuration and measurement data to clients.
- **DenClock client:** Provides the existing mapping configuration UI and displays measurements using their configured display names.

The unique identity of a sensor is `(gatewayId, nodeId)`, not `nodeId` alone.

## Requirements

### 1. Consume the Revised MQTT Contract

Update DenClock's MQTT subscriber to consume the numeric identifiers published by LoRaService following Story 1.

- Support numeric `gatewayId` and `nodeId`.
- Identify each sensor by the composite key `(gatewayId, nodeId)`.
- Preserve existing measurement values, units, timestamps, and other supported metadata.
- Remove dependencies on semantic names previously supplied by LoRaService.
- Follow the actual MQTT topic and payload contract established in Story 1.

Do not introduce backward compatibility with the previous MQTT contract.

### 2. Move Semantic Mapping Ownership to DenClock

DenClock must own the association between physical sensor identities and their logical meanings.

Each sensor mapping should support:

- `gatewayId` — numeric gateway identifier.
- `nodeId` — numeric sensor node identifier.
- `name` — stable logical name used internally by DenClock.
- `displayName` — human-readable label shown in the client.

Gateway mappings should similarly associate numeric gateway IDs with optional logical and display names.

Use the existing DenClock configuration and persistence mechanisms wherever possible.

Do not introduce a new database, configuration subsystem, or external mapping service unless the existing architecture makes it necessary.

### 3. Adapt the Existing Client Mapping UI

**The client already has a UI for configuring sensor mappings. Preserve and extend that functionality rather than replacing it.**

Inspect the current UI, its service API, configuration model, and persistence behavior before making changes.

Update the existing UI so that:

- Sensor mappings use `(gatewayId, nodeId)` rather than a node number alone.
- Users can assign and edit logical sensor names and display names.
- Gateway identifiers can be associated with optional names and display names.
- Existing add, edit, remove, and save workflows continue to operate.
- Changes persist through the DenClock service using the established configuration mechanism.
- The UI presents numeric identities clearly enough to distinguish sensors belonging to different gateways.

Retain the current UI's layout and interaction conventions as much as practical.

Do not undertake a general UI redesign.

### 4. Sensor Discovery and Unmapped Sensors

Preserve any existing functionality for discovering sensors from incoming measurements.

Where supported by the current design:

- Identify previously unseen sensors by `(gatewayId, nodeId)`.
- Make them available to the existing configuration UI.
- Allow users to assign semantic names through that UI.
- Ensure unknown sensors do not cause processing failures.

Do not invent permanent semantic names for newly discovered sensors.

Keep any additional discovery functionality minimal and consistent with the current implementation.

### 5. Measurement Processing and Display

Update measurement processing and client-facing data models to use the DenClock-owned mapping.

- Resolve incoming sensor identities to configured logical names.
- Use configured display names for presentation.
- Preserve existing sensor display behavior wherever possible.
- Ensure measurements from identically numbered nodes on different gateways cannot overwrite or be confused with one another.
- Handle missing mappings gracefully.

Avoid unnecessary changes to the clock, weather display, or unrelated services.

### 6. Configuration Persistence

Adapt the existing DenClock mapping configuration as necessary.

- Persist mappings using the established configuration mechanism.
- Ensure mappings survive service restarts.
- Enforce uniqueness of the composite sensor identity.
- Preserve existing configuration values where practical.
- Do not create a compatibility or migration framework solely for this change.

A one-time manual configuration adjustment is acceptable if required.

## Acceptance Criteria

- [ ] DenClock consumes the MQTT format produced by Story 1.
- [ ] Sensor identity is based on `(gatewayId, nodeId)` throughout the relevant service and client code.
- [ ] Semantic sensor and gateway mapping is owned by DenClock.
- [ ] The existing client mapping UI remains functional.
- [ ] Users can configure sensor logical names and display names through that UI.
- [ ] Users can configure optional gateway names and display names.
- [ ] Mapping changes are persisted and survive service restarts.
- [ ] Two gateways can each have a node with the same numeric ID without collisions.
- [ ] Previously unknown sensor IDs are handled gracefully.
- [ ] The existing measurement displays use the configured display names.
- [ ] No dependency remains on semantic mappings supplied by LoRaService.
- [ ] Relevant service and client tests are updated and pass.

## Out of Scope

- Changes to LoRaService or ESP32 gateway firmware.
- A new configuration UI.
- Redesigning the existing client UI.
- A new database or persistence framework.
- Historical measurement migration unless required by the existing architecture.
- Backward compatibility with the old MQTT message format.
- Unrelated clock, weather, or display features.

## Implementation Guidance

Before editing code, inspect:

1. The current DenClock MQTT subscriber and measurement data models.
2. The existing client sensor-mapping UI.
3. The API used by that UI to read and update mappings.
4. The existing configuration persistence mechanism.
5. The MQTT contract implemented in Story 1.

Prefer adapting existing components and workflows over introducing parallel implementations.

Keep the distinction between sensor identity, logical name, and display name explicit.

**Do not move mapping configuration out of the client UI.** The DenClock service owns the mapping data and resolution logic; the client UI remains the user's mechanism for configuring it.

## Completion

Run relevant tests and report:

- Files changed.
- Final mapping data model and persistence format.
- Changes made to the existing client configuration UI.
- How unmapped sensors are handled.
- Verification that two gateways with identical node numbers remain distinct.
- Test results and any remaining limitations.

## Acceptance

Accepted by the developer on 2026-10-28. DenClock consumes LoRaService's numeric
MQTT identity and owns sensor and optional gateway naming in its existing
presentation settings and gear dialog. No intentional departures from scope.

Verification: the server build completed without warnings or errors; 80 server
configuration, persistence, and state checks passed; all client tests passed;
and hosting and MQTT integration tests passed. The MQTT integration test verified
that Gateway 1 / Node 1 and Gateway 2 / Node 1 remain distinct. The developer
also corrected the authoritative ProgramData configuration after its startup
validation error. The old presentation format requires the documented one-time
manual adjustment; there is no compatibility parser or automatic ID inference.
