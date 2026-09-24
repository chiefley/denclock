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

The sensor sends node 1 reports with three two-byte TLVs in a 16-byte binary
packet, then enters timer-driven deep sleep for five seconds. Values are rounded to hundredths of degrees C,
hundredths of percent RH, and millivolts, using big-endian encoding.
Sequence starts at 0, advances for every generated/transmission-attempted report
(including radio errors), and wraps from 65535 to 1. Sensor read failures skip
reports and retry initialization; no fabricated or stale readings are sent.
RTC memory retains the next sequence across timer wakes; a true reset or power-up
starts at 0. Failed readings retry on the next wake without consuming a sequence.

Both projects use 915 MHz, 125 kHz bandwidth, SF9, coding rate 4/7, 10 dBm,
eight-symbol RadioLib default preamble, 1.8 V TCXO, explicit header and CRC.
The exact sync register bytes are D3 C7. RadioLib's
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
same directory and set Wi-Fi and service endpoint values before building the
production gateway. An unconfigured build logs a message and discards packets.
The gateway sends HELLO (ID 1) after every TCP connect, retries connections at
five-second intervals, and discards reports while disconnected without queuing.
RSSI is rounded to the nearest dBm; SNR to the nearest quarter dB. Metadata that
cannot fit the protocol signed bytes is logged and the packet discarded rather
than wrapped or assigned an invented saturation value. Raw LoRa bytes are opaque.

## Gateway OLED diagnostics

[Gateway OLED Infrastructure Diagnostics](docs/stories/completed/004-gateway-oled-infrastructure-diagnostics.md)
is verified and accepted by the developer.
The production build uses the same ThingPulse OLED library and Heltec OLED wiring
as the bringup receiver. It shows a prominent node ID, rounded RSSI (dBm) and SNR
(dB), battery volts (N/A when absent), and full RX/ERR counters on separate rows.
Environmental measurements are never rendered. Before the first valid report,
node and signal values show placeholders.

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
only node ID and battery volts appear, remain readable for about five seconds,
and then disappear. Observe several timer wakes: the screen must stay dark,
reports must continue, and sequence must advance normally. Compare battery
current during measurement and sleep with the previous firmware where practical.

## Remote sensor deep-sleep operation

Each wake powers Vext, initializes SHT40 and SX1262, measures, and attempts one
report. `DevelopmentSleepUs` in `remote-sensor/src/main.cpp` sets the five-second
sleep interval; initialization and transmission add to the report-to-report time.
There is no one-second startup delay or serial-attachment wait. Initialization,
measurement, and transmission failures also lead to sleep and retry next wake.

After blocking transmission completes or fails, the radio enters cold sleep
(`sleep(false)`) and SPI stops. Wire1 stops, SDA/SCL become inputs without internal
pull-ups, Vext goes HIGH (off), and battery measurement enable stays LOW. GPIO
holds preserve these states and keep radio NSS/reset HIGH through deep sleep.
Holds are released on wake before reinitialization. RTC memory preserves only the
next sequence; there are no per-report flash writes. These choices follow
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
2. Confirm one report per wake with fresh readings and roughly five seconds of
   sleep plus awake time between reports; inspect wake/reset and TX diagnostics.
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

1. Start the gateway, then reset the sensor. Check node=1, seq=0, then 1, 2, 3.
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

Radio settings are unchanged on both ends: 915 MHz, 125 kHz bandwidth, SF9,
coding rate 4/7, private sync word, 10 dBm, eight-symbol preamble, 1.8 V TCXO.
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
