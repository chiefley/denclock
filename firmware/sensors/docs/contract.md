# Sensor and gateway contract

The Heltec WiFi LoRa 32 V3 sensor uses an SHT40 on Wire1 (GPIO41/42)
and the onboard switched battery divider (GPIO37 enable, GPIO1 ADC).
See the [README](../README.md) for wiring, calibration assumptions, and operation.

## Reports

The sensor emits the [SENSOR_REPORT wire format](../../../docs/LoRa%20Protocol.md):
message class 1, configured node ID, big-endian sequence, then temperature (type 1), relative
humidity (type 2), and battery voltage (type 3). Each value is two bytes; the
whole packet is 16 bytes. Temperature is signed hundredths of degrees C,
humidity unsigned hundredths of percent RH, and battery unsigned millivolts.
Values are rounded to the nearest wire unit.

Sequence starts at 0 after reset, advances for each generated report even when
transmission fails, and wraps from 65535 to 1. Invalid or unavailable SHT40
readings cause a skipped report and initialization retry, without consuming a
sequence. The next sequence is RTC-retained across timer deep-sleep wakes, with
no per-report flash writes. Non-timer wake or a true reset starts again at 0.
Each wake samples and checks reporting policy before packet quantization. The
first valid sample after reset reports immediately. Later samples report when
absolute unrounded SHT40 temperature change from the last successfully transmitted
unrounded measurement is at least the configured threshold, or the heartbeat
interval has elapsed. Humidity and battery do not independently trigger reports;
every transmitted report includes their current values.

Development defaults in `remote-sensor/include/config.h` are 0.1 °C threshold,
30-second heartbeat, and five-second nominal sleep with fresh random jitter
bounded by -1 and +1 second. Jitter applies on every cycle, including suppressed
reports and failures. Awake processing adds to the 4–6-second sleep duration.
Heartbeat expiry is checked at each valid sample; a report may follow expiry by
one sampling cycle, or longer during failures.

RTC memory retains the unrounded baseline, successful-transmission timestamp,
and whether an initial report has succeeded. RTC-backed system time measures
awake and sleep time. Only RadioLib TX success updates reporting state; failures
leave it unchanged. Success is local transmission completion, not gateway receipt.
Reset clears this state along with sequence. Hardware entropy is acquired every
wake before ADC initialization and disabled before battery measurement, avoiding
ADC contention. Independent jitter prevents fixed periodic alignment without
introducing acknowledgments or radio scheduling. Initialization/read failures
sleep and retry on the next wake.

After blocking transmission, the SX1262 is commanded into cold sleep, SPI and
Wire1 stop, I2C pins become inputs without pull-ups, Vext switches off, and the
battery divider remains disabled. GPIO holds preserve peripheral power controls
and radio NSS/reset through ESP32 deep sleep. The sensor must be wired to Vext
as documented; always-powered wiring cannot be switched off by this firmware.
The radio and sensor are initialized again on wake. Radio sleep failures are
logged; the ESP32 still sleeps and retries on wake.

## Sensor serial configuration

The sensor loads a validated version-2 complete Preferences/NVS configuration
at every startup, including timer wakes. Missing, incompatible or invalid data
uses development defaults: node 2, sample 5 seconds, jitter ±1 second,
heartbeat 30 seconds, threshold 1 tenth °C, power 10 dBm, frequency 915000 kHz
and sync-register value 0xD3C7. Version-1 records are rejected without migration
of any settings; fallback defaults are not written until explicit `save`.
Only explicit `save` writes NVS; measurement/reporting cycles never do.

After non-deep-sleep reset, a ten-second serial entry window at 115200 baud
overlaps OLED diagnostics. A recognized valid command enters configuration;
radio initialization and transmissions wait until exit. Timer wakes bypass
entry and OLED entirely. Commands are case-insensitive, whitespace-tolerant,
CR/LF/CRLF terminated and bounded to 95 characters. Invalid commands leave
settings unchanged and do not refresh inactivity time.

The sensor echoes serial input as characters arrive. Backspace and Delete erase
the previous input character; CRLF produces a single echoed line ending.
Disable terminal local echo if characters appear twice. Line-buffered terminals
may send characters only after Enter.

The sensor serial console displays `> ` when accepting commands, including the
startup entry window, and repeats it after command responses or invalid input.
Exit, timeout, and reboot do not emit another prompt.

`?`/`help` lists commands, ranges, units and examples; `show` displays working
settings and Saved or Modified (unsaved). `set node/sample/jitter/heartbeat/
threshold/power N` edits RAM after validating the whole candidate. Ranges are
node 1–239, sample 1–86400 seconds, jitter 0–2147 seconds with sample > jitter,
heartbeat 1–604800 seconds, threshold 1–1650 integer tenths °C, and power
-9–22 dBm. `set frequency N` accepts 902125–927875 integer kHz;
`set sync HHHH` accepts exactly four hexadecimal digits, case-insensitive, with
an optional `0x` prefix. All 16-bit sync values are accepted unchanged.
`show` renders frequency in MHz with three decimals and sync as `0xHHHH`.
Microsecond conversion and jitter arithmetic remain bounded.

`save` persists the entire record and remains in the console. `defaults` resets
RAM only. `exit` and five minutes without a valid command restore the saved
snapshot (or default fallback) and resume operation. `reboot` restarts without
saving. Explicit exit can end OLED diagnostics early. First valid measurement
after reset/configuration reports using effective settings. Serial configuration
does not change packet format or gateway behavior. See the [console reference](../README.md#persistent-sensor-configuration).

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
behavior and sequence handling are independent of OLED diagnostics. Sampling
and reporting follow the policy above.

## Radio and reception

Both projects default to 915.000 MHz, 10 dBm and sync registers D3 C7.
The sensor uses configured frequency, power and sync when initializing the radio;
the gateway retains fixed defaults. Bandwidth 125 kHz, SF9, coding rate 4/7,
CRC enabled, eight-symbol preamble and 1.8 V TCXO remain fixed. Startup serial
diagnostics display the actual sensor frequency and sync. Radio initialization
errors retain existing sleep/retry handling.

`radioLibSync` converts register-byte representation to RadioLib's split-nibble
arguments; 0xD3C7 produces `setSyncWord(0xDC, 0x37)`. Settings apply on the next
radio initialization, never during configuration. Sensor and gateway settings
must match. Arbitrary sync words are experimental and do not guarantee equivalent
reception or perfect isolation. They are synchronization parameters, not
credentials. The permitted frequency range does not establish FCC compliance.

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

The normal screen also shows `WiFi: OK` with Wi-Fi RSSI in dBm or `WiFi: DOWN`,
using `WiFi.status()`, and independently `Server: OK` or `Server: DOWN`, using
`service.connected()`. All seven rows use the 10-pixel font to preserve node,
LoRa signal, battery, and full counters on the 128×64 display. Connection state
is checked each loop; Wi-Fi RSSI is sampled on connection and every second
while connected. RSSI changes below 3 dBm from the displayed value are suppressed;
connection-state changes always update the current signal. Changes mark the display dirty; unchanged connectivity and
signal values do not trigger a redraw. No network connection attempt is made
in the display update path. Serial logs retain Wi-Fi disconnection reason codes
and connection/reconnection events, failed TCP connection attempts, and TCP
disconnection reporting.

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
Connectivity changes during the alert are cached and restored on expiry as well.
Reception is rearmed before OLED/TCP work. Existing synchronous networking can
still delay loop servicing; no lossless receive guarantee is added.

No MQTT, DenClock integration, or command handling is implemented.
Sync-word matching provides network discrimination, not authentication.
