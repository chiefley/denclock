# LoRa temperature, humidity, and battery test

Update the existing lora-test-tx and lora-test-rx sketches for the Heltec
WiFi LoRa 32 V3 / SX1262. Read the Adafruit SHT40 powered by Vext on
SDA2/SCK2 and the onboard battery voltage; transmit human-readable values.
Receive, parse, display, and serial-log temperature, relative humidity, and battery voltage. Preserve radio settings and the working blocking test
structure. Do not implement a formal protocol or change production firmware.

Acceptance criteria:
- Both PlatformIO projects compile.
- RX shows all three readings with units; malformed data is reported.
- Sensor failures cannot masquerade as valid or stale measurements.
- Document pin/library assumptions and a bench verification procedure.
- Hardware acceptance remains with the developer after flashing both boards.
