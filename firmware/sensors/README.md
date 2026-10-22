# LoRa sensors and gateway

[Accepted story: Implement Remote LoRa Sensor](docs/stories/completed/002-implement-remote-lora-sensor.md).
The developer confirmed that the implementation works as advertised.
See the [implemented contract](docs/contract.md) for the firmware behavior.
The accepted [end-to-end receive story](../../docs/stories/completed/004-end-to-end-lora-receive-path.md)
adds gateway Wi-Fi/TCP transport to the separate LoRa Service. The developer
confirmed testing and verification. Follow the [LoRa Service setup and test instructions](../../lora-service/README.md)
for the production path.

- `remote-sensor/`: Heltec V3/SX1262, SHT40 and onboard battery sensing.
- `lora-gateway/`: application-agnostic Wi-Fi/TCP transport by default. The
  `diagnostic` PlatformIO environment preserves the accepted serial-only decoder.
- `common/`: shared radio settings and portable packet encoding/validation.
- `../../docs/LoRa Protocol.md`: [wire protocol](../../docs/LoRa%20Protocol.md).
- `../bringup/`: unchanged known-good TX/RX hardware diagnostics described below.

The sensor sends reports with its configured node ID and three two-byte TLVs in a
16-byte binary packet, then enters timer-driven deep sleep with fresh random jitter. Values are rounded to hundredths of degrees C,
hundredths of percent RH, and millivolts, using big-endian encoding.
Sequence starts at 0, advances for every generated/transmission-attempted report
(including radio errors), and wraps from 65535 to 1. Sensor read failures skip
reports and retry initialization; no fabricated or stale readings are sent.
RTC memory retains the next sequence, unrounded reporting baseline, and last
successful transmission time across timer wakes; a true reset or power-up
starts at 0. Failed readings retry on the next wake without consuming a sequence.

Both projects default to 915.000 MHz (frequency is configurable on both), 125 kHz bandwidth, SF9, coding rate 4/7,
10 dBm by default (sensor power is configurable),
eight-symbol RadioLib default preamble, 1.8 V TCXO, explicit header and CRC.
The default sync register bytes are D3 C7 (configurable on both). RadioLib's
[`setSyncWord(syncWord, controlBits)`](https://jgromes.github.io/RadioLib/class_s_x126x.html)
uses split nibbles, so the call is `setSyncWord(0xDC, 0x37)`.

The preserved `diagnostic` receiver rejects radio/CRC errors, unsupported message classes, reserved
source IDs, type zero, truncated TLVs, wrong known lengths, duplicate known
measurements, and RH above 100%. Unknown nonzero TLVs are skipped; reordered
TLVs and reports with missing measurements are legal under the protocol. Only
present measurements are printed after the entire packet validates. The first
sensor always sends all three. Sync-word matching is not authentication.

Hardware wiring and battery calibration assumptions match the bench tests
below. The production gateway drives its OLED continuously; the remote sensor
uses its OLED briefly after reset. Both retain serial diagnostics at 115200;
the serial-only gateway `diagnostic` build does not use an OLED.
The [deep-sleep story](docs/stories/completed/003-remote-sensor-deep-sleep-operation.md)
is verified and accepted by the developer. Production gateway networking forwards raw packets
to the independent LoRa Service; the DenClock time server is not involved.
Copy `lora-gateway/include/config.example.h` to ignored `config.local.h` in the
same directory and set Wi-Fi values before building the production gateway.
`LORA_SERVICE_HOST` there is only the default service host; a host saved through
the [gateway serial console](#persistent-gateway-configuration) takes precedence.
An unconfigured build logs a message and discards packets.
The gateway sends HELLO (ID 1) after every TCP connect, retries connections at
five-second intervals, and discards reports while disconnected without queuing.
RSSI is rounded to the nearest dBm; SNR to the nearest quarter dB. Metadata that
cannot fit the protocol signed bytes is logged and the packet discarded rather
than wrapped or assigned an invented saturation value. Raw LoRa bytes are opaque.

## Gateway OLED diagnostics

[Gateway OLED Infrastructure Diagnostics](docs/stories/completed/004-gateway-oled-infrastructure-diagnostics.md)
is verified and accepted by the developer.
The production build uses the same ThingPulse OLED library and Heltec OLED wiring
as the bringup receiver. It shows a centered node ID, rounded LoRa RSSI (dBm) and SNR
(dB), battery volts (N/A when absent), and full RX/ERR counters on separate rows.
Environmental measurements are never rendered. Before the first valid report,
node and signal values show placeholders.

[Display Wi-Fi and Server Connectivity Status](docs/stories/completed/007-gateway-display-wifi-and-server-connectivity-status.md)
is accepted by the developer. The follow-up
[Gateway OLED Connectivity Status](docs/stories/completed/008-gateway-oled-connectivity-status.md)
is also accepted and refines RSSI refresh and TCP failure logging.
The normal screen uses seven compact 10-pixel-font
rows to retain the sensor diagnostics while adding `WiFi: OK` with RSSI in dBm
or `WiFi: DOWN`, and independently `Server: OK` or `Server: DOWN` for the TCP
connection. Connection state is checked each loop; connected Wi-Fi RSSI is sampled
every second. RSSI changes below 3 dBm from the displayed value do not trigger
a redraw; connection-state changes always update the current signal. The packet-error
screen temporarily covers these rows; expiry restores current connectivity and
the latest sensor diagnostics. Serial retains Wi-Fi disconnection reason codes,
connection/reconnection events, failed TCP connection attempts, and TCP disconnection reporting.

RX counts locally validated sensor reports, even without a TCP connection. ERR
counts observed RadioLib receive failures and reports rejected by the existing
sensor decoder. An error fills the display white with large black `PACKET ERROR`
text for three seconds; another error restarts that interval. Reception continues,
and expiry restores the latest valid report. Exact decoder reasons and radio error
codes remain in RAM and are logged to serial. Silent radio filtering (such as a
different sync word) cannot be counted. No version field or node registry exists.

OLED validation is observational: it does not filter the raw TCP forwarding path.
Disconnected-service drops, TCP write failures, and unrepresentable forwarding
metadata retain their existing logs and are not OLED sensor-validation errors.
The error timer is nonblocking and rollover-safe; existing synchronous network
operations can still delay loop servicing and cause packet loss.

Build both environments with:

```sh
pio run -d firmware/sensors/lora-gateway -e heltec_wifi_lora_32_V3 -e diagnostic
g++ -std=c++11 -Wall -Wextra -Werror -pedantic firmware/sensors/tests/gateway_diagnostics_test.cpp -o /tmp/denclock-gateway-diagnostics-test
/tmp/denclock-gateway-diagnostics-test
```

On hardware, flash the production environment and verify:

1. Startup placeholders, then node, battery, signal units and advancing RX counts;
   no temperature/humidity. Confirm the layout fits and the node stands out.
2. Send a malformed TLV, unsupported class, or reserved node ID using a temporary
   transmitter with matching radio settings. Confirm ERR increments once and
   serial preserves the reason. Exercise observable CRC failures where practical.
3. During the bright error screen, send a valid report from another node and
   confirm raw TCP forwarding continues. At about three seconds, confirm the
   newest node and counters appear. A second error should restart the interval.
4. Send a valid report without a battery TLV and confirm N/A replaces the previous
   battery. Test an error before any valid report and restoration to placeholders.
5. Disconnect the service: valid radio reports must still update RX/the OLED.
   Restore the service and confirm HELLO and unchanged raw forwarding resume.
6. Stop LoRaService while Wi-Fi remains connected: confirm `Server: DOWN` with
   `WiFi: OK` and RSSI. Disable router Wi-Fi: confirm both indicators show DOWN.
   Restore each connection and confirm automatic recovery without a reset,
   continuing LoRa reception, and readable diagnostics throughout.

## Persistent sensor configuration

[Persistent Serial Configuration for LoRa Sensor Nodes](docs/stories/completed/009-persistent-serial-configuration.md)
is accepted by the developer.

Use the existing PlatformIO monitor at 115200 baud and reset/power the sensor.
Enter `?` within ten seconds of reset to enter configuration. The five-second
OLED diagnostics overlap this window; there is no additional consecutive delay.
Timer deep-sleep wakes bypass both. A valid command enters the console; invalid
input does not extend entry or inactivity windows. No sensor radio initialization
or transmission occurs until the console exits. Explicit exit can end startup
diagnostics early.

The sensor echoes serial input as characters arrive. Backspace and Delete erase
the previous input character; CRLF produces a single echoed line ending.
Disable terminal local echo if characters appear twice. Line-buffered terminals
may send characters only after Enter.

The sensor serial console displays `> ` when accepting commands, including the
startup entry window, and repeats it after command responses or invalid input.
Exit, timeout, and reboot do not emit another prompt.

Commands ignore case and extra whitespace, accept CR, LF or CRLF, and use integer
values. Lines are limited to 95 characters; oversized or malformed lines are
rejected in full. `?` or `help` lists commands, ranges, units and examples.

| Command | Range / action | Development default |
| --- | --- | --- |
| `set node N` | 1–239 | 2 |
| `set sample N` | 1–86400 seconds, strictly greater than jitter | 5 |
| `set jitter N` | 0–2147 seconds, ± jitter, less than sample | 1 |
| `set heartbeat N` | 1–604800 seconds | 30 |
| `set threshold N` | 1–1650 tenths of °C; 1 means 0.1°C | 1 |
| `set power N` | -9–22 dBm (SX1262 range) | 10 |
| `set frequency N` | 902125–927875 integer kHz | 915000 (915.000 MHz) |
| `set sync HHHH` | Exactly four hex digits, optional `0x` prefix | D3C7 |
| `show` | All working settings, threshold units and saved/unsaved status | |
| `save` | Persist all settings; remain in console | |
| `defaults` | Restore development defaults in RAM | |
| `exit` | Discard unsaved edits and resume operation | |
| `reboot` | Restart without saving edits | |

For example, enter `set node 3`, `set sample 15`, `set jitter 2`,
`set heartbeat 300`, `set threshold 1`, `set power 10`, then `save`, `show`,
and `exit`, each on its own line. There are no den/remote profiles.

`set` and `defaults` only change RAM. Five minutes without a valid command
prints a timeout and discards unsaved edits, as does `exit`. Only `save` writes
the complete version-2 configuration blob to ESP32 Preferences/NVS namespace
`denclock-sensor`, key `config`. Missing, wrong-size, incompatible or invalid
records fall back to development defaults. Flash write failure is reported and
the session retains its previous saved snapshot. The effective snapshot counts
as Saved even when using defaults with no stored record. Settings survive sleep,
power cycling and firmware uploads that preserve NVS. Sampling never writes NVS.

[Configurable LoRa Frequency and Sync Word](docs/stories/completed/010-configurable-lora-frequency-and-sync-word.md)
is accepted by the developer.
Use `set frequency 915500` for 915.500 MHz and `set sync D3C7` or
`set sync 0xd3c7` for the two SX1262 sync register bytes. `show` displays
frequency in MHz to three decimals and sync as four hexadecimal digits.
Changes apply at the next radio initialization, after leaving configuration.
Sensor and gateway frequency, sync and modulation settings must match to receive
reports. Configure the gateway to match through its
[serial console](#persistent-gateway-configuration).

Version-1 records are rejected entirely: upgrading resets **all** effective
settings to version-2 defaults, including node ID and reporting intervals.
There is no migration, and fallback does not write NVS. Reconfigure each sensor
and explicitly `save`. Both new settings use the same RAM/save/defaults/exit/
timeout/reboot semantics as other settings.

The frequency range does not establish FCC compliance. Custom sync words are
experimental and may differ in reception performance or isolation. Sync words
are radio synchronization parameters, not authentication credentials; firmware
does not replace arbitrary values. Radio initialization failures retain the
existing error log and deep-sleep retry behavior.

Bounds cap sample sleep at a day, heartbeat at a week and threshold at the SHT40
measurement span; jitter keeps the microsecond random span within 32 bits.
Temperature comparisons still use raw SHT40 measurements. Reset/session exit
produces an initial report at the next valid measurement with effective settings.

Additional host checks (use a local executable path on Windows):

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic firmware/sensors/tests/sensor_config_test.cpp -o /tmp/denclock-sensor-config-test
/tmp/denclock-sensor-config-test
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -I firmware/sensors/tests/console_fakes firmware/sensors/tests/config_console_test.cpp -o /tmp/denclock-config-console-test
/tmp/denclock-config-console-test
```

Sync conversion host check:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic firmware/sensors/tests/radio_sync_test.cpp -o /tmp/denclock-radio-sync-test
/tmp/denclock-radio-sync-test
```

For story 010 hardware verification, confirm default gateway reception, saved
frequency/sync persistence through reset, matched-setting reception, mismatched
frequency/sync behavior, and unchanged sampling and sleep. Matching custom gateway
settings use the [gateway console](#persistent-gateway-configuration).

Console tests use fake serial, time and Preferences storage; they do not prove
physical NVS durability or serial timing. Before acceptance, run the hardware
checks in story 009: power/reset entry and overlapping OLED timing, every command,
save/reboot and power-cycle persistence, exit/timeout discard, invalid inputs,
custom reporting/node/power settings, timer wakes with no entry delays, gateway
reception, and two differently configured boards using the same firmware image.

## Persistent gateway configuration

[Persistent Serial Configuration for LoRa Gateway](docs/stories/completed/011-persistent-serial-configuration-for-lora-gateway.md)
is accepted by the developer.

One production gateway image serves every installation. The LoRa frequency, sync
word and LoRa Service host are configured over the existing USB serial port at
115200 baud and saved in NVS. Wi-Fi credentials remain compile-time settings in
`config.local.h`; the TCP port stays 5210 and the gateway ID stays 1. The
`diagnostic` environment is unchanged and always uses the radio defaults.

The gateway stays in normal operation while it is configured: there is no reset
or entry window. Type `config` and Enter in the serial monitor to enter
configuration mode. Other lines outside configuration mode print a hint and are
otherwise ignored. In configuration mode the gateway prints `> ` when accepting
commands. Radio reception, TCP forwarding, the OLED and serial diagnostics carry
on throughout, so log lines may interleave with typing. The console handles only
characters already received and never waits inside the receive loop.

Echo, Backspace/Delete, CR/LF/CRLF handling and whole-line rejection follow the
sensor console. Disable terminal local echo if characters appear twice. Keywords
ignore case and extra whitespace; the hostname keeps the case it was entered in.
Lines are limited to 287 characters, enough for `set host` with a 253-character
name. Oversized, malformed and extra-argument lines change nothing.

| Command | Range / action | Default |
| --- | --- | --- |
| `config` | Enter configuration mode (outside configuration mode only) | |
| `set frequency N` | 902125–927875 integer kHz | 915000 (915.000 MHz) |
| `set sync HHHH` | Exactly four hex digits, optional `0x` prefix | D3C7 |
| `set host NAME` | DNS hostname, `.local` mDNS name, or dotted-quad IPv4; 1–253 characters | `LORA_SERVICE_HOST` (`192.168.50.90` unless overridden locally) |
| `?` / `help` | Commands, ranges, units and examples | |
| `show` | Staged settings, fixed service port and saved/unsaved status | |
| `save` | Persist all settings; remain in configuration mode | |
| `defaults` | Restore defaults in RAM | |
| `exit` | Discard unsaved edits and leave configuration mode | |
| `reboot` | Restart without saving edits | |

For example, enter `config`, `set frequency 915500`, `set sync D3C7`,
`set host denclock-d3c7.local`, `save`, then `reboot`, each on its own line.

Frequency is stored as integer kHz and shown in MHz to three decimals. The sync
word is the two SX1262 sync register bytes, converted with the shared
`radioLibSync` exactly as on the sensor. Sync words are radio synchronization
parameters, not authentication credentials. The frequency range does not
establish FCC compliance. A sensor and gateway receive each other only when
frequency, sync word and the fixed modulation settings all match. Startup serial
output reports the actual frequency, sync word and service host.

Hostnames are letters, digits and interior hyphens in dot-separated labels of
1–63 characters, with no empty labels, trailing dot, underscore, port or URL. Text
whose last label is all digits must be a full dotted quad with octets 0–255 and
no leading zeros. Names are stored in a fixed 254-byte field that always holds the
whole name and its terminator; longer input is rejected rather than truncated.

Edits are staged in RAM and never touch the running radio or TCP connection. Only
`save` writes NVS (namespace `denclock-gateway`, key `config`, version-1 record);
saved settings take effect at the next reboot or power-up. `show` reports
`Saved` or `Modified (unsaved)`, adding `(reboot to apply)` when the saved
settings differ from the running ones. `exit`, and five minutes without a valid
command, discard unsaved edits; invalid commands do not extend the timeout.
`reboot` does not save. Missing, wrong-size, wrong-version or invalid records
fall back to defaults without writing NVS, and running on defaults counts as
Saved. Settings survive power cycles and firmware uploads that preserve NVS; a
flash erase restores defaults. A failed save is reported and the previous saved
snapshot is kept.

Each TCP attempt resolves the configured host again, so the gateway never holds
on to an old address (lwIP may still cache a DNS answer for its TTL). IPv4
literals need no lookup. DNS names use the DHCP-provided servers. `.local` names
are sent as multicast DNS queries by lwIP's built-in support
(`CONFIG_LWIP_DNS_SUPPORT_MDNS_QUERIES` in the pinned framework). The gateway
does not advertise itself. A lookup failure logs `LoRa Service host ... not
resolved; will retry`. A refused or timed-out connection logs the resolved
address. Both retry at the existing five-second interval without a restart. A
failed DNS lookup can block the loop for up to about 15 seconds, the same as
the previous compile-time hostname path, and packets can be lost meanwhile. Until
the LoRa Service advertises its `.local` name, such a host may stay unresolved.

Host checks (use a local executable path on Windows):

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic firmware/sensors/tests/gateway_config_test.cpp -o /tmp/denclock-gateway-config-test
/tmp/denclock-gateway-config-test
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -I firmware/sensors/tests/console_fakes firmware/sensors/tests/gateway_console_test.cpp -o /tmp/denclock-gateway-console-test
/tmp/denclock-gateway-console-test
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -I firmware/sensors/tests/console_fakes firmware/sensors/tests/gateway_connection_test.cpp -o /tmp/denclock-gateway-connection-test
/tmp/denclock-gateway-connection-test
```

These tests use fake serial, time, Preferences, resolver and TCP client objects.
They do not prove NVS durability, serial timing, mDNS/DNS behavior on the
network, or RF compatibility. Hardware verification steps are listed in the story.

## Remote sensor reset diagnostics

[Sensor Node Reset Status Display](docs/stories/completed/005-sensor-node-reset-status-display.md)
is accepted by the developer. Power-up, the Reset button, and other resets that
are not deep-sleep wakes show a prominent node ID and battery voltage with two
decimal places for five seconds. The display reuses the packet battery-measurement
routine; it shows no environmental measurements. All deep-sleep wakes skip OLED
initialization and the display delay.

The sensor uses the same pinned ThingPulse library and GPIO17/18 I2C wiring,
with reset on GPIO21, as the gateway. Diagnostics run before sensor/radio
initialization so they remain available even if those devices fail. After the
interval, firmware sends display-off, stops Wire, removes OLED I2C pull-ups,
asserts OLED reset LOW, and switches Vext off. Vext supplies both OLED and SHT40,
so normal sensor startup restores it; holding OLED reset LOW keeps the display
inactive during measurement and transmission. Deep sleep switches Vext off and
holds OLED reset LOW and its I2C pins as inputs. Actual current consumption,
including any reset-line pull-up load, has not been characterized by Codex.

For hardware regression checks, power-cycle and press Reset separately: confirm
only node ID and battery volts appear, remain readable for about five seconds (unless explicitly exiting the console),
and then disappear. Observe several timer wakes: the screen must stay dark,
reports must continue, and sequence must advance normally. Compare battery
current during measurement and sleep with the previous firmware where practical.

## Change reporting and heartbeat

[Sensor Change Reporting, Heartbeat, and Randomized Sampling](docs/stories/completed/006-sensor-change-reporting-heartbeat-and-randomized-sampling.md)
is verified and accepted by the developer. Development defaults in
`remote-sensor/include/config.h` are five-second nominal sampling sleep,
random jitter bounded by -1 and +1 second, a 0.1 °C temperature threshold,
and a 30-second heartbeat. Every cycle gets fresh jitter, including cycles with
suppressed reports or failures; awake processing adds to the 4–6-second sleep.

The first valid sample after reset triggers a report. Subsequent samples report
when unrounded SHT40 temperature differs from the last successfully transmitted
unrounded measurement by at least the threshold, or the heartbeat expires.
Humidity and battery changes do not trigger reports; every report includes all
three current readings. Packet rounding does not influence reporting decisions.
Only a successful RadioLib transmission updates the baseline and heartbeat time;
there is no gateway acknowledgment. Heartbeats are checked at valid samples, so
reports can follow expiry by up to one sampling cycle, or longer during failures.

RTC memory retains reporting state and RTC-backed system time includes awake and
sleep time. Fresh ESP32 hardware entropy is collected before ADC initialization,
then its source is disabled before battery measurement. Independent jitter avoids
persistent timing alignment without adding radio coordination or retry protocols.

For regression checks, run two nodes together and observe their activity timing
wander. Let temperatures stabilize to check suppression and continuing heartbeats,
then warm/cool a sensor gradually to check accumulated raw temperature changes.
Confirm reset still shows the OLED for five seconds and routine wakes keep it dark.

Portable policy tests:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic firmware/sensors/tests/reporting_policy_test.cpp -o /tmp/denclock-reporting-policy-test
/tmp/denclock-reporting-policy-test
```

## Remote sensor deep-sleep operation

Each wake powers Vext, initializes SHT40 and SX1262, measures, and checks whether
a report is due. Sampling/reporting settings are loaded from NVS with development defaults in
`remote-sensor/include/config.h`; awake processing adds to the sampling period.
There is no one-second startup delay or serial-attachment wait. Initialization,
measurement, and transmission failures also lead to sleep and retry next wake.

After blocking transmission completes or fails, the radio enters cold sleep
(`sleep(false)`) and SPI stops. Wire1 stops, SDA/SCL become inputs without internal
pull-ups, Vext goes HIGH (off), and battery measurement enable stays LOW. GPIO
holds preserve these states and keep radio NSS/reset HIGH through deep sleep.
Holds are released on wake before reinitialization. RTC memory preserves the next sequence, unrounded temperature baseline,
and successful-transmission timestamp; there are no per-report flash writes. These choices follow
[ESP32-S3 GPIO hold requirements](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32s3/api-reference/peripherals/gpio.html)
and the pinned RadioLib blocking transmit/cold-sleep implementation.

This assumes the documented SHT40 VIN-to-Vext wiring. A sensor wired to permanent
3.3 V will remain powered; external I2C pull-ups to an always-on supply can also
back-power it. Verify the actual wiring and Vext voltage during the hardware test.
Board regulator/charger and other residual current have not been characterized.
A radio sleep error is logged and requires hardware investigation; firmware still
sleeps the ESP32 and retries initialization on the next wake.

For acceptance, use the existing gateway and LoRa Service without modification:

1. Reset/power the sensor and confirm sequence 0 followed by 1, 2, 3 across wakes.
2. Confirm fresh samples each wake, randomized 4–6-second sleeps, temperature
   change reports, and stable-temperature heartbeats; inspect serial diagnostics.
3. Confirm Vext switches off between cycles and reporting recovers when the
   SHT40 is disconnected and reconnected, without resetting the sequence.
4. Power-cycle and press reset separately; each must restart at sequence 0.
5. Where practical, measure current to confirm the low-current interval; serial
   output alone does not establish radio or sensor sleep current. USB serial may
   disconnect during deep sleep; battery operation is the relevant power test.

## Sensor/diagnostic build and bench test

For the original serial-only acceptance path, from the repository root:

```sh
pio run -d firmware/sensors/remote-sensor
pio run -d firmware/sensors/lora-gateway -e diagnostic
```

Upload the sensor with `pio run -d <project> --target upload --upload-port <port>`.
For the diagnostic gateway, add `-e diagnostic`; omitting it selects production TCP transport.
Monitor each board with `pio device monitor --port <port> --baud 115200`.
Use the appropriate serial port on Windows or your development host.

1. Start the gateway, then reset the sensor. Check node=2 (or the saved ID), seq=0, then 1, 2, 3.
2. Compare all three RX readings against TX serial output, which includes the
   exact 16 bytes. Warm the SHT40 and confirm changing readings reach RX.
3. Compare battery millivolts against a meter, preferably on battery power.
4. Disconnect the SHT40: reports stop with a TX error message. Reconnect and
   confirm reporting resumes without stale data or an artificial sequence reset.
5. Reset TX again and confirm sequence 0 is received.
6. For malformed-packet bench checks, use a temporary test transmitter configured
   with the same radio settings (do not edit the bringup programs). Send type-zero,
   short-header, truncated-TLV, and incorrect known-length packets; confirm they
   are discarded. Send an unknown nonzero TLV followed by known TLVs; confirm
   known values still decode. A transmitter with the old bringup sync word
   should produce no accepted reports.

Portable tests (host C++ compiler):

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic firmware/sensors/tests/sensor_report_test.cpp -o /tmp/denclock-sensor-report-test
/tmp/denclock-sensor-report-test
```

On Windows, choose a local executable output path. Tests cover a literal wire
fixture, negative and boundary values, rollover, malformed/truncated packets,
reserved identifiers, reordered/unknown TLVs, and rejection without partial
output. They do not establish actual sensor readings or RF compatibility.

## Preserved LoRa sensor bench tests

`lora-test-tx` and `lora-test-rx` are independent PlatformIO/Arduino projects
for Heltec WiFi LoRa 32 V3 (ESP32-S3 / SX1262). These are blocking bench
tests; the other sensor/gateway projects are outside this change.

TX reads the SHT40 and onboard battery voltage before each transmission. Example payload:

```text
T=23.45 C RH=51.20 % BAT=4.08 V
```

Units are Celsius, relative humidity percent, and volts. The original two-second
delay remains after each transmission (sampling and radio time add to it).
The local packet counter is retained for TX progress/error displays; it is not
part of the measurement payload. No formal telemetry protocol is introduced.

RX logs the raw packet, RSSI and SNR at 115200 baud, then logs the parsed values
and shows all three readings on its OLED. Malformed, out-of-range, infinite, truncated,
or trailing-data payloads show an error rather than old measurements. Sensor
failure is sent as `T=nan C RH=nan % BAT=... V`, displayed as N/A; TX retries
sensor initialization on the next cycle. Battery reporting continues.

## Hardware assumptions

- SHT40 VIN connects to Vext, with common GND; address is 0x44.
- The supplied SDA2/SCK2 labels are interpreted as GPIO41 SDA / GPIO42 SCL.
  The installed board definition calls these SDA/SCL. Confirm the physical
  header mapping; SCK2 here means I2C clock, not the radio SPI clock.
- Sensor uses Wire1 at 100 kHz. OLED keeps Wire on GPIO17/18, reset GPIO21.
- Vext GPIO36 is explicitly held LOW on both boards to power peripherals.
- Battery sensing uses the circuit already on the board. No external divider
  or jumper is required. Firmware drives GPIO37 HIGH to enable measurement,
  reads GPIO1, and drives GPIO37 LOW afterwards. This is separate from Vext.
  TX waits 10 ms, discards one conversion, averages 16 calibrated millivolt
  readings at 12-bit/0 dB, then multiplies by 4.9 for the onboard divider.
  This matches Heltec's V3.2 example; confirm the actual board revision and
  compare against a meter. USB-only operation can show a charger-node voltage
  even with no battery attached; this code does not detect battery presence.
- SHT40 uses high precision with heater off. TX adds Adafruit SHT4x Library
  ^1.0.5 (tested 1.0.5), with BusIO 1.17.4 and Unified Sensor 1.1.15 resolved
  during this build. The library manifest also downloads Adafruit display
  dependencies; the sketch continues to use the existing ThingPulse OLED.

Radio defaults on both ends are 915.000 MHz, 125 kHz bandwidth, SF9,
coding rate 4/7, sync registers D3 C7, 10 dBm, eight-symbol preamble, 1.8 V TCXO.
The sensor supports configurable frequency, sync word and transmit power; the
gateway supports configurable frequency and sync word at its fixed 10 dBm.
Existing RadioLib CRC behavior and radio error handling remain in place.

References: [Heltec battery example](https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/examples/ESP32/WiFiLoRa32_battery_read/WiFiLoRa32_battery_read.ino),
[Heltec Vext polarity](https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/src/heltec.h),
[Adafruit SHT4x API](https://github.com/adafruit/Adafruit_SHT4X/blob/master/Adafruit_SHT4x.h).

## Build and bench verification

From each of `firmware/bringup/lora-test-tx` and
`firmware/bringup/lora-test-rx`, run `pio run`.
Use `pio run --target upload --upload-port <port>` for the corresponding board,
then `pio device monitor --port <port> --baud 115200`. In this WSL setup,
PlatformIO is installed at `/home/chief/.platformio/penv/bin/pio`.

1. Flash the matching projects onto the two boards and monitor both serial logs.
2. Check that TX values match RX values and all three readings fit on each OLED.
3. Warm the sensor slightly and confirm temperature/RH changes reach RX.
4. Disconnect sensor power: RX should show N/A for temperature/RH and packets continue.
   Reconnect and confirm automatic recovery. Battery reporting should continue.
5. Compare reported battery voltage with a meter, preferably on battery power.

The portable parser test can be run with a host C++ compiler from `lora-test-rx`:

```sh
g++ -std=c++11 -Wall -Wextra -pedantic tests/test_payload.cpp -o /tmp/denclock-test-payload
/tmp/denclock-test-payload
```

It covers valid/negative/boundary readings, sensor-unavailable markers, malformed
fields, units, truncation, trailing bytes, embedded NUL, nonfinite values, and
range rejection. Hardware flashing and bench verification remain pending.

[Accepted bench-test story](docs/stories/completed/001-lora-environment-test.md)
