# Sensor and diagnostic gateway contract

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
sequence. The loop waits two seconds after transmission or a failed reading;
sampling and transmission add to that development interval.

## Radio and reception

Both projects configure SX1262 at 915 MHz, bandwidth 125 kHz, SF9, coding rate
4/7, TX power 10 dBm, sync word 0xD3C7, CRC enabled, eight-symbol preamble
(the pinned RadioLib default), and 1.8 V TCXO.

The serial diagnostic gateway validates a complete received packet before
printing node, sequence, RSSI, SNR, and present measurements at 115200 baud.
It rejects radio/CRC errors, unsupported message classes, reserved source IDs,
type zero, truncated TLVs, wrong known measurement lengths, duplicate known
measurements, and humidity above 100%. It accepts reordered TLVs, skips unknown
nonzero TLVs, and permits missing measurements as the protocol specifies.

Measurement interpretation is diagnostic scaffolding. The final gateway is
intended to remain application-agnostic. No deep sleep, Wi-Fi, MQTT, server
integration, or command handling is implemented. Sync-word matching provides
network discrimination, not authentication.
