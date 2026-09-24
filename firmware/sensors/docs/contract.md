# Sensor and gateway contract

The Heltec WiFi LoRa 32 V3 sensor uses an SHT40 on Wire1 (GPIO41/42)
and the onboard switched battery divider (GPIO37 enable, GPIO1 ADC).
See the [README](../README.md) for wiring, calibration assumptions, and operation.

## Reports

The sensor emits the [SENSOR_REPORT wire format](../../../docs/LoRa%20Protocol.md):
message class 1, node 1, big-endian sequence, then temperature (type 1), relative
humidity (type 2), and battery voltage (type 3). Each value is two bytes; the
whole packet is 16 bytes. Temperature is signed hundredths of degrees C,
humidity unsigned hundredths of percent RH, and battery unsigned millivolts.
Values are rounded to the nearest wire unit.

Sequence starts at 0 after reset, advances for each generated report even when
transmission fails, and wraps from 65535 to 1. Invalid or unavailable SHT40
readings cause a skipped report and initialization retry, without consuming a
sequence. The next sequence is RTC-retained across timer deep-sleep wakes, with
no per-report flash writes. Non-timer wake or a true reset starts again at 0.
Each wake normally attempts one report, then sleeps for five seconds; sampling
and transmission add to that development interval. Initialization/read failures
also sleep and retry on the next wake.

After blocking transmission, the SX1262 is commanded into cold sleep, SPI and
Wire1 stop, I2C pins become inputs without pull-ups, Vext switches off, and the
battery divider remains disabled. GPIO holds preserve peripheral power controls
and radio NSS/reset through ESP32 deep sleep. The sensor must be wired to Vext
as documented; always-powered wiring cannot be switched off by this firmware.
The radio and sensor are initialized again on wake. Radio sleep failures are
logged; the ESP32 still sleeps and retries on wake.

## Sensor reset OLED diagnostics

Power-up, manual reset, and other resets except ESP32 deep-sleep wakes display
only the configured node ID and battery voltage for five seconds before normal
sensor/radio initialization. The centered node ID uses a 24-pixel font; voltage
uses a 16-pixel font with two decimal places and the existing battery-measurement
routine. Deep-sleep wakes do not initialize the OLED or wait for this interval.
OLED initialization failure does not prevent normal sensor operation.

After diagnostics, firmware sends display-off, stops the OLED Wire bus, leaves
GPIO17/18 as inputs without internal pull-ups, asserts reset GPIO21 LOW, and
cuts Vext power. Because Vext also supplies the SHT40, it is restored for normal
sensor operation with OLED reset held LOW. During deep sleep Vext is off, and
GPIO holds preserve OLED reset and I2C pin states. Packet format, measurement
behavior, sequence handling, and the five-second sleep interval are unchanged.

## Radio and reception

Both projects configure SX1262 at 915 MHz, bandwidth 125 kHz, SF9, coding rate
4/7, TX power 10 dBm, sync word 0xD3C7, CRC enabled, eight-symbol preamble
(the pinned RadioLib default), and 1.8 V TCXO.

The selectable `diagnostic` build of the gateway validates a complete received packet before
printing node, sequence, RSSI, SNR, and present measurements at 115200 baud.
It rejects radio/CRC errors, unsupported message classes, reserved source IDs,
type zero, truncated TLVs, wrong known measurement lengths, duplicate known
measurements, and humidity above 100%. It accepts reordered TLVs, skips unknown
nonzero TLVs, and permits missing measurements as the protocol specifies.

## Production gateway transport

The default gateway transport is application-agnostic. It receives radio/CRC-valid
packets and forwards the exact bytes over persistent TCP to the separately
configured LoRa Service. Forwarding does not depend on Message Class, Node ID,
sequence, or TLVs, and does not timestamp or convert packets to JSON. A local
OLED observer separately reuses the sensor decoder without changing the raw
forwarding decision or bytes.

The gateway connects to configured Wi-Fi and initiates TCP to the service.
It sends HELLO with Gateway ID 1 after every successful connection. RX_PACKET
contains signed-byte RSSI in dBm, signed-byte SNR in quarter-dB units, and the
unchanged radio bytes. Signal values are rounded to the nearest wire unit;
unrepresentable metadata causes a logged discard rather than numeric wrap.
Partial TCP writes finish before another frame is sent; failed writes close the
connection so later data cannot be appended to an incomplete frame.

Wi-Fi/TCP connections are retried with a five-second retry interval. Radio
reception is interrupt-signaled so the normal loop can maintain networking
without waiting for a blocking receive timeout. Network operations can still
cause packet loss; there is no application queue or persistent buffer. Packets
received without an available service connection are discarded.

Local credentials/endpoint values belong in the ignored `include/config.local.h`
next to its checked-in example. Missing configuration logs a diagnostic and
leaves forwarding disabled. The `diagnostic` environment has no Wi-Fi dependency
and retains the original sensor decoder source unchanged.

## Production gateway OLED

The continuously powered 128×64 OLED shows the most recently validated sensor
node, RSSI in dBm, SNR in dB, battery volts, and cumulative RX/ERR counts since
boot. It never displays environmental measurements. Missing battery is N/A;
before any accepted report, node and signal fields are placeholders.

RX counts sensor-decoder successes independently of network availability. ERR
counts observed radio receive failures and sensor-decoder rejections, retaining
the last exact decoder reason or RadioLib error code even after subsequent
success. Unobservable radio filtering, connection failures and transport metadata
discards are not sensor-validation errors. The protocol has no version field and
there is no node registry; assignable node IDs remain valid.

A rejection displays large black `PACKET ERROR` text on a white background for
three seconds from the most recent error. The rollover-safe timer does not wait
or suspend packet processing. Valid reports received during the interval update
the cached diagnostics; expiry restores that latest report and current counters.
Reception is rearmed before OLED/TCP work. Existing synchronous networking can
still delay loop servicing; no lossless receive guarantee is added.

No MQTT, DenClock integration, or command handling is implemented.
Sync-word matching provides network discrimination, not authentication.
