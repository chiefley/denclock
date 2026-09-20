#include <Arduino.h>
#include "../../common/radio_config.h"
#include "../../common/sensor_report.h"

SX1262 radio = new Module(RadioNss, RadioDio1, RadioReset, RadioBusy);
void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);
    int16_t state = initializeRadio(radio);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialization failed: %d\n", state);
        while (true) delay(1000);
    }
    Serial.println("Gateway diagnostic receiver: 915 MHz, sync 0xD3C7, CRC on");
}
void loop() {
    uint8_t packet[255];
    // Zero length tells RadioLib to read the actual received packet length.
    int16_t state = radio.receive(packet, 0);
    if (state == RADIOLIB_ERR_RX_TIMEOUT) return;
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("RX discarded: radio error %d\n", state);
        return;
    }
    SensorReport::Report report;
    const char* error = SensorReport::decode(packet, radio.getPacketLength(), report);
    if (error) {
        Serial.printf("RX discarded: %s\n", error);
        return;
    }
    Serial.printf("RX node=%u seq=%u RSSI=%.1f SNR=%.1f\n",
                  report.node, report.sequence, radio.getRSSI(), radio.getSNR());
    if (report.hasTemperature)
        Serial.printf("TEMPERATURE        %.2f C\n", report.temperature / 100.0);
    if (report.hasHumidity)
        Serial.printf("RELATIVE_HUMIDITY  %.2f %%\n", report.humidity / 100.0);
    if (report.hasBattery)
        Serial.printf("BATTERY_VOLTAGE    %u mV\n", report.battery);
    Serial.println();
}
