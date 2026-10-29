# LoRa Service Normalization Boundary

The previous configuration-driven semantic mapping has been removed. LoRaService
loads no semantic mapping file and generates no gateway or sensor names.
Historical requirements remain in the [accepted mapping story](../lora-service/docs/stories/completed/002-implement-semantic-mapping-and-mqtt-publishing.md).

LoRaService retains decoding and validation of the fixed [LoRa protocol](LoRa%20Protocol.md)
measurement definitions. It publishes normalized temperature (C), relative humidity
(percent), and battery voltage (V), with precision 4 metadata and numeric device
identifiers. Unknown TLVs warn and are skipped. Measurement definitions are code,
not installation configuration; assigned wire semantics remain immutable.

See the [MQTT contract](MQTT-INTERFACE.md) and [worker contract](../lora-service/docs/contract.md)
for current behavior. Consumers identify sensors by `(gatewayId, nodeId)` and own
physical locations and display names. DenClock support is deferred to Story 2.

## Logging

ILogger through Serilog writes interactive console and daily rolling files retained
for seven days. Logs cover infrastructure, connectivity, malformed packets, unknown
TLV types, and operational failures; routine measurements go to MQTT.
