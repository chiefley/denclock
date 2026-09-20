# Story: Reconcile LoRa Service Architecture

## Goal

Inspect the committed protocol and project status documentation, then reconcile
the architecture documents with the agreed separate LoRa Service boundary.

## Requirements

- Confirm the SENSOR_REPORT assignments, Node ID rules, sequence-zero restart
  semantics, and radio configuration including 0xD3C7 in the protocol document.
- Confirm that documentation records the working sensor and diagnostic receiver,
  completed sensor story, and preserved bringup baseline. Report gaps without
  changing protocol, status, README, or completed-story content.
- Preserve valid architecture content and reconcile ARCHITECTURE.md and
  ARCHITECTURE2.md into a clear primary reference.
- Gateway firmware owns radio transport and publishes raw packets and reception
  metadata to MQTT. MQTT provides transport and distribution.
- A separate LoRa Service owns custom wire-protocol parsing and publishes
  normalized/application-friendly messages.
- DenClock consumes those messages and owns environmental history and
  presentation. Future ham-radio applications likewise consume service outputs;
  applications do not parse the custom LoRa wire protocol.
- Mosquitto, LoRa Service, and DenClock may coexist on the Windows NUC but remain
  independently movable so DenClock can remain when LoRa infrastructure moves.
- Distinguish agreed future boundaries from currently implemented behavior.

## Verification

Review against committed documentation, check architecture links and the diff,
and confirm protocol/status/firmware files remain unchanged. No runtime behavior
changes are in scope.

## Acceptance

Accepted by the human developer on 2026-09-20 with explicit authorization to commit.

Architecture documents reconciled; documentation links and diff whitespace checks
passed. Protocol, status, README, and firmware files remain unchanged.
