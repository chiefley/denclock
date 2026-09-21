# LoRa Service receive contract

The independent .NET 10 worker listens on a configurable IP address and TCP port
(default `0.0.0.0:5210`). It supports interactive execution and graceful shutdown.
There is no dependency on the DenClock server and no MQTT interface yet.

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
skipped by length. Missing measurements remain absent and are logged as N/A.
Malformed boundaries, type zero, and incorrect known lengths are rejected. The
initial decoder also rejects duplicate known measurements and RH above 100%,
matching the accepted diagnostic decoder's validation policy. It imposes no
SHT40-specific temperature range on the wire's signed representation.

LoRa errors (including unsupported classes) are logged and dropped without
closing a correctly initialized gateway connection. Valid reports log Gateway
ID, UTC time, Node ID, sequence, all present measurements with units, RSSI and SNR.
There is no persistent storage, sequence deduplication, outbound protocol,
application friendly-name mapping, or generalized handler framework.
