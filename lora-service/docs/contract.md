# LoRa Service receive contract

The independent .NET 10 worker listens on a configurable IP address and TCP port
(default `0.0.0.0:5210`). It supports interactive execution and Windows Service hosting with graceful shutdown.
The host selects the Windows Service lifetime when launched by the Windows service
manager. Configuration loads from the executable directory. Deployment is
framework-dependent and requires the .NET 10 runtime on the Windows host.
There is no dependency on the DenClock server. The receive path now feeds
configured semantic mapping and an MQTT publisher. The developer has verified
NUC deployment with Mosquitto, live publications, retained-state persistence,
and service/broker restart and NUC reboot recovery.

Each TCP connection has independent framing and Gateway ID state. The first
message must be HELLO with a one-byte, nonzero ID. Later messages must be
RX_PACKET. Repeated HELLO, unsupported/zero message type, invalid type-specific
length, and truncated frames are Gateway Protocol errors: they are logged and
only that connection closes. A disconnected gateway can reconnect and identify
itself again; one idle or malformed client does not stop other clients.
Gateway IDs identify connections but are not authentication or globally enforced
unique sessions. No duplicate-ID takeover policy is introduced.

Framing uses the three-byte header and big-endian uint16 payload length, not
TCP read boundaries. The declared length bounds each allocation to 65535 bytes.
RX_PACKET requires the two radio metadata bytes; its remaining bytes are kept
unchanged. An empty raw payload is classified as a LoRa protocol error. RSSI is
signed int8 dBm; SNR is signed int8 divided by 4 dB. UTC time is captured as soon
as the complete frame is reconstructed, before LoRa decoding. It describes
service reception, not RF reception or measurement time.

LoRa Message Class 1 dispatches to the SENSOR_REPORT decoder. Source IDs must
be 1–239. The sequence is big-endian uint16, including legal reset value 0.
Temperature is signed int16 / 100 C, humidity unsigned uint16 / 100 %, and battery
unsigned uint16 / 1000 V. TLVs can appear in any order; unknown nonzero types are
skipped by length. Missing measurements remain absent from the published measurements object.
Malformed boundaries, type zero, and incorrect known lengths are rejected. The
initial decoder also rejects duplicate known measurements and RH above 100%,
matching the accepted diagnostic decoder's validation policy. It imposes no
SHT40-specific temperature range on the wire's signed representation.

LoRa errors (including unsupported classes) are logged and dropped without
closing a correctly initialized gateway connection. Valid reports preserve Gateway ID, UTC time, Node ID, sequence, measurements,
RSSI and SNR in the semantic MQTT message. Routine successful values are not logged.

## Semantic mapping and publication

The startup-loaded JSON mapping supplies gateway/node names and measurement
names, signed/unsigned 16-bit decoding, scale, units, and precision metadata.
Assigned types are validated against immutable LoRa definitions. Mapping/config
errors stop startup before listeners start. Runtime unknown identifiers warn;
gateways fall back to decimal ID strings, nodes to `node-{id}`, and unmapped TLVs
are skipped. Mapped duplicate measurements are rejected as ambiguous. Unknown
unmapped TLVs remain skippable. Values use decimal arithmetic without presentation
rounding. Mapping changes require restart.

The [semantic mapping](../../docs/Lora%20Service%20Semantic%20Mapping.md) and
[MQTT interface](../../docs/MQTT-INTERFACE.md) own detailed definitions. The service
attempts one JSON publication per valid report on `lora/sensors/{node-name}`,
QoS 1, retained. MQTTnet connects independently with clean sessions and retries
at five-second polling intervals. Direct publishes time out after five seconds;
unavailable/failed attempts are logged and discarded. Gateway reception continues,
without an offline queue, spool, or replay. MQTT failure is not a LoRa framing error.

ILogger uses Serilog console output interactively and daily files with seven-day
age retention. Files are infrastructure diagnostics, not measurement history.
The [README](../README.md) documents paths, configuration, and validation.
There is no persistent measurement storage, sequence deduplication, outbound
application protocol, or generalized handler framework.

## Destination hostname ownership

Hostname resolution is a deployment responsibility independent of this worker.
The LoRa Service does not register DNS-SD or run an mDNS responder. The gateway
can explicitly target the installation's unique Windows hostname plus `.local`,
subject to the [deployment A-query and ownership checks](mdns-deployment.md),
or an explicitly configured ordinary DNS name/IPv4 address. Stopping this worker
does not withdraw the computer hostname. TCP remains on port 5210 by default.
Resolution does not authenticate the installation; duplicate names must be
resolved administratively before deployment. No discovery or automatic fallback
to another installation is implemented. The gateway uses the ESP32 mDNS client
for explicit `.local` A queries, without setting a gateway mDNS hostname or
registering services. Ordinary DNS and IPv4 remain supported.
