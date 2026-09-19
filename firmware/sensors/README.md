# LoRa sensor bench tests

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

From each of `lora-test-tx` and `lora-test-rx`, run `pio run`.
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

[Active story](docs/stories/001-lora-environment-test.md)
