# Architecture Reference

The architecture notes formerly maintained here have been reconciled into
[ARCHITECTURE.md](ARCHITECTURE.md), the primary architecture and decisions document.

That document preserves the valid gateway/MQTT, browser, node-behavior, and TLV
principles, alongside the existing clock and Windows hosting decisions. It records
the agreed Gateway → TCP → LoRa Service → MQTT → applications boundary and independent
deployment of Mosquitto, LoRa Service, and DenClock.

For the finalized sensor packet definitions and radio configuration, see
[LoRa Protocol.md](LoRa%20Protocol.md). The earlier statements here that these
details were TBD are superseded by that specification.

This file remains as a navigation link for existing references; it is not a
second architecture specification.
