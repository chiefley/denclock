#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <math.h>
#include "../../common/radio_config.h"
#include "../../common/sensor_report.h"

SX1262 radio = new Module(RadioNss, RadioDio1, RadioReset, RadioBusy);
Adafruit_SHT4x sht40;
constexpr uint8_t NodeId = 1;
constexpr uint32_t ReportIntervalMs = 2000;
constexpr int BatteryAdc = 1, BatteryEnable = 37;
bool sensorReady = false;
uint16_t sequence = 0;

bool initializeSensor() {
    if (!sht40.begin(&Wire1)) return false;
    sht40.setPrecision(SHT4X_HIGH_PRECISION);
    sht40.setHeater(SHT4X_NO_HEATER);
    return true;
}
uint16_t readBatteryMillivolts() {
    digitalWrite(BatteryEnable, HIGH);
    delay(10);
    analogReadMilliVolts(BatteryAdc);
    uint32_t sum = 0;
    for (int i = 0; i < 16; ++i) {
        sum += analogReadMilliVolts(BatteryAdc);
        delay(1);
    }
    digitalWrite(BatteryEnable, LOW);
    return uint16_t(lroundf(sum / 16.0f * 4.9f));
}
void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);
    pinMode(BatteryEnable, OUTPUT);
    digitalWrite(BatteryEnable, LOW);
    analogReadResolution(12);
    analogSetPinAttenuation(BatteryAdc, ADC_0db);
    if (!Wire1.begin(41, 42, 100000)) {
        Serial.println("Sensor I2C initialization failed");
        while (true) delay(1000);
    }
    Wire1.setTimeOut(100);
    sensorReady = initializeSensor();
    int16_t state = initializeRadio(radio);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialization failed: %d\n", state);
        while (true) delay(1000);
    }
    Serial.println("Remote sensor ready: 915 MHz, sync 0xD3C7, CRC on");
}
void loop() {
    if (!sensorReady) sensorReady = initializeSensor();
    sensors_event_t humidity{}, temperature{};
    bool valid = sensorReady && sht40.getEvent(&humidity, &temperature);
    valid = valid && isfinite(temperature.temperature)
        && isfinite(humidity.relative_humidity)
        && temperature.temperature >= -40 && temperature.temperature <= 125
        && humidity.relative_humidity >= 0 && humidity.relative_humidity <= 100;
    if (!valid) {
        sensorReady = false;
        Serial.println("SHT40 unavailable/invalid; report skipped, retrying");
        delay(ReportIntervalMs);
        return;
    }
    int16_t temp = lroundf(temperature.temperature * 100);
    uint16_t rh = lroundf(humidity.relative_humidity * 100);
    uint16_t battery = readBatteryMillivolts();
    uint8_t packet[SensorReport::PacketSize];
    SensorReport::encode(packet, NodeId, sequence, temp, rh, battery);
    Serial.printf("TX node=%u seq=%u T=%.2f C RH=%.2f %% BAT=%u mV bytes=",
                  NodeId, sequence, temp / 100.0, rh / 100.0, battery);
    for (uint8_t byte : packet) Serial.printf("%02X ", byte);
    int16_t state = radio.transmit(packet, sizeof(packet));
    Serial.printf("result=%d\n", state);
    // A generated report consumes its sequence even if transmission fails.
    sequence = SensorReport::nextSequence(sequence);
    delay(ReportIntervalMs);
}
