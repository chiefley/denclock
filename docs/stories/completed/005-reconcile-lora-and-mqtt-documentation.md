# Reconcile LoRa and MQTT Documentation

## Story

Reconcile the existing DenClock documentation with the recently accepted LoRa Service semantic-mapping and MQTT-interface contracts.

The repository has evolved incrementally. Several older architecture and service documents still describe MQTT behavior as undefined or assign some semantic responsibilities differently from the newer contracts.

The purpose of this story is to make the enduring documentation internally consistent with the decisions that have already been made.

This is a documentation-maintenance story. It does not introduce new architecture, change protocols, or require implementation changes.

## Governing Documents

Treat the following existing documents as authoritative for their respective boundaries:

- `docs/LoRa Protocol.md` — LoRa application wire protocol.
- `docs/Gateway Protocol.md` — Gateway-to-LoRa-Service transport protocol.
- `docs/Lora Service Semantic Mapping.md` — semantic mapping performed by the LoRa Service.
- `docs/MQTT-INTERFACE.md` — application-facing MQTT contract.

Existing implemented behavior shall not be rewritten to imply that functionality exists when it has only been specified.

## Required Reconciliation

### 1. `docs/ARCHITECTURE.md`

Update the architecture document to reflect that the MQTT application boundary is now specified.

Remove or revise statements that describe the following as TBD or deliberately unresolved:

- MQTT sensor topic structure;
- normalized sensor-message schema;
- QoS;
- retained-message behavior;
- basic MQTT reconnect/delivery behavior already established by `MQTT-INTERFACE.md`.

Reference `MQTT-INTERFACE.md` rather than duplicating its detailed contract.

Correct the responsibility assignment for friendly names.

The current architecture assigns friendly sensor names to DenClock/application configuration. The accepted architecture is now:

- LoRa Service semantic mapping resolves Gateway IDs to friendly gateway names.
- LoRa Service semantic mapping resolves Node IDs to friendly node names.
- LoRa Service semantic mapping resolves TLV Type IDs to semantic measurement names and normalized values.
- Applications such as DenClock remain responsible for presentation behavior, display-unit preferences/conversion, history, alarms, and other application-specific policy.

Revise statements suggesting that applications determine publication policy for the retained `SENSOR_REPORT` MQTT topics. The initial sensor MQTT publication contract is now defined by `MQTT-INTERFACE.md`, including:

```text
Topic:    lora/sensors/{node-name}
QoS:      1
Retained: true
```

Preserve the distinction between MQTT retained last-known state and application measurement history.

Do not duplicate the complete MQTT payload schema in the architecture document. Reference the authoritative interface document instead.

### 2. `docs/LoRa Service.md`

Update the LoRa Service specification to acknowledge that semantic normalization and the initial MQTT sensor interface have now been defined.

Replace statements such as:

> The exact normalized application-message schema is not yet defined.

with references to the semantic-mapping and MQTT-interface documents.

Update the MQTT section so it no longer lists already-decided SENSOR_REPORT behavior as intentionally unspecified.

The service specification should make clear that:

```text
LoRa wire representation
        |
        v
LoRa Service semantic mapping
        |
        v
normalized semantic representation
        |
        v
MQTT interface
```

The LoRa Service owns interpretation and normalization.

MQTT consumers shall not need knowledge of the LoRa or Gateway wire protocols.

Do not imply that MQTT publishing has already been implemented if the current implementation does not yet provide it.

Future COMMAND, RESPONSE, EVENT, transmission, authentication, and other bidirectional behavior remains undefined except where another authoritative document explicitly defines it.

### 3. `docs/LoRa Protocol.md`

Preserve this document as the authoritative LoRa wire-protocol specification.

Do not move MQTT semantics into it.

Update stale wording that describes MQTT merely as a future boundary. Where useful, reference `MQTT-INTERFACE.md` as the separately defined application-facing interface.

Continue to state clearly that MQTT topics, payload representation, QoS, retention, application persistence, and presentation are outside the LoRa wire protocol itself.

The wire protocol definitions must not change as part of this story.

In particular, do not alter:

- Message Class assignments;
- Node ID ranges;
- sequence-number semantics;
- byte order;
- TLV encoding;
- TLV Type assignments;
- measurement wire representations;
- unknown-TLV behavior;
- radio parameters;
- sync word;
- CRC requirements.

### 4. Implemented-State Documentation

Review:

- `STATUS.md`
- `lora-service/README.md`
- root `README.md`

Do not change accurate statements describing the currently implemented state merely because future behavior has now been specified.

In particular, if MQTT publishing is not yet implemented:

- `STATUS.md` may continue to say MQTT integration is deferred.
- `lora-service/README.md` may continue to describe the currently implemented service as decoding/logging sensor reports without requiring MQTT.

Specification and implementation status are different concepts.

Update these files only if an actual contradiction or misleading statement is found.

## Authority and Duplication

Use references rather than copying detailed definitions between documents.

The intended authority boundaries are:

```text
LoRa Protocol.md
    LoRa application wire representation

Gateway Protocol.md
    Gateway <-> LoRa Service transport

Lora Service Semantic Mapping.md
    numeric/protocol representation -> semantic representation

MQTT-INTERFACE.md
    semantic application-facing publication contract

ARCHITECTURE.md
    component responsibilities and system boundaries

LoRa Service.md
    LoRa Service component responsibilities
```

Do not create competing definitions of the same protocol or interface.

## No Design Changes

This story is reconciliation, not another architecture exercise.

Do not introduce:

- new MQTT topics;
- new payload fields;
- different QoS;
- different retained-message policy;
- new mapping behavior;
- new LoRa fields;
- new Gateway Protocol fields;
- authentication or TLS requirements;
- databases;
- message queues;
- offline spooling;
- command protocols;
- multi-gateway deduplication;
- new abstractions intended only for hypothetical future requirements.

If reconciliation exposes a genuine unresolved design question, report it rather than inventing a decision.

## Implementation Changes

No production-code or firmware changes are expected.

Do not change implementation merely to make it match behavior that is currently only specified for a future implementation story.

If an apparent documentation inconsistency actually indicates that existing implementation violates an already-accepted implemented contract, report that separately rather than silently changing code.

## Verification

After making the documentation changes, review the relevant documents together and verify that they agree on the following:

- Gateway firmware transports packets and does not interpret the LoRa application protocol.
- Gateway Protocol transports raw LoRa bytes plus reception metadata.
- LoRa Service assigns the authoritative UTC reception timestamp.
- LoRa Service owns LoRa application-protocol interpretation.
- LoRa Service semantic mapping resolves gateway, node, and measurement identities and normalizes values.
- MQTT is the semantic application-facing distribution boundary.
- `SENSOR_REPORT` MQTT messages use `lora/sensors/{node-name}`, QoS 1, and retained messages.
- MQTT retained state is last-known state, not measurement history.
- DenClock does not parse the LoRa wire protocol.
- DenClock remains responsible for its application state/history and presentation behavior.
- The LoRa and Gateway wire protocols remain unchanged.
- Documentation distinguishes specified future behavior from currently implemented behavior.

Run any repository documentation/link checks that already exist. Do not introduce new tooling solely for this story.

## Acceptance Criteria

This story is complete when:

1. `docs/ARCHITECTURE.md` no longer describes the defined SENSOR_REPORT MQTT contract as TBD.
2. `docs/ARCHITECTURE.md` assigns friendly gateway/node naming and measurement semantic mapping to the LoRa Service rather than DenClock.
3. `docs/LoRa Service.md` references the accepted semantic-mapping and MQTT contracts instead of describing them as undefined.
4. `docs/LoRa Protocol.md` remains authoritative and unchanged in wire semantics while acknowledging the separately defined MQTT interface.
5. No document incorrectly claims that MQTT publishing is already implemented.
6. Current implementation-status documentation remains accurate.
7. No production code or firmware behavior is changed.
8. A final consistency review finds no contradictory ownership, protocol, mapping, or MQTT definitions among the governing documents.

Report the files changed, the stale statements reconciled, verification performed, and any remaining ambiguity requiring a human decision.

Do not commit or push. Leave the changes for developer review and acceptance.

## Reconciliation and review results

- Preserved the supplied working tree, including correct friendly-name ownership
  and removal of obsolete MQTT unresolved items. The tree was clean relative to
  Git when this review began; existing manual edits were treated as the baseline.
- Updated architecture and service specifications to reference the accepted mapping
  and MQTT contracts, separate implemented logging from specified infrastructure
  logging, and retain application history/presentation ownership.
- Fixed the LoRa protocol's MQTT reference without changing wire semantics.
- Corrected the mapping example JSON, mapping-domain count, and Gateway ID origin
  and terminology. Added implementation-status notes to mapping/MQTT specifications.
- Root README, STATUS, and LoRa Service README remain accurate and unchanged.
- Reviewed ownership, timestamps, topic/QoS/retention, history boundaries, and
  implementation status across the governing documents. Local file links and
  JSON examples pass; git diff --check passes. No existing documentation/link
  checker was found; no tooling was added. No production code or firmware changed.
- Gateway Protocol is unchanged; LoRa Protocol changes only the MQTT reference.
- The developer resolved the unknown Gateway ID ambiguity: its ID may serve as
  the fallback name. Documented the decimal ID string in Gateway Mapping and
  referenced that rule from MQTT gateway identity. No wire semantics changed.

## Human acceptance

The developer accepted this story as complete, including the clarification that
an unmapped Gateway ID may use its decimal ID as the fallback name. Documentation
is reconciled; specified MQTT and semantic-mapping behavior remains distinct from
implemented behavior. No code, firmware, or wire-protocol semantics changed.
