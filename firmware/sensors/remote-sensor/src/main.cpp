#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <math.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include "../../common/radio_config.h"
#include "../../common/sensor_report.h"

SX1262 radio = new Module(RadioNss, RadioDio1, RadioReset, RadioBusy);
Adafruit_SHT4x sht40;
constexpr uint8_t NodeId = 1;
constexpr uint64_t DevelopmentSleepUs = 5ULL * 1000000ULL;
constexpr int BatteryAdc = 1, BatteryEnable = 37;
bool sensorReady = false;
RTC_DATA_ATTR uint16_t sequence = 0;
bool i2cReady = false;
bool radioAttempted = false;
constexpr int SleepHeldPins[] = {Vext, BatteryEnable, 41, 42, RadioNss, RadioReset};

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
// transmit() is blocking and finishes TX (or times out) before reaching here.
void enterDeepSleep() {
    if (radioAttempted) {
        int16_t state = radio.sleep(false); // Cold sleep; reinitialize on every wake.
        if (state != RADIOLIB_ERR_NONE)
            Serial.printf("Radio sleep failed: %d; retry next wake\n", state);
        SPI.end();
    }
    if (i2cReady) Wire1.end();
    // Remove I2C pull-ups before cutting sensor power to avoid back-powering.
    pinMode(41, INPUT);
    pinMode(42, INPUT);
    digitalWrite(Vext, HIGH); // Active-low peripheral supply.
    digitalWrite(BatteryEnable, LOW);
    // NSS must remain high so the sleeping SX1262 is not woken by a low edge.
    pinMode(RadioNss, OUTPUT);
    digitalWrite(RadioNss, HIGH);
    pinMode(RadioReset, OUTPUT);
    digitalWrite(RadioReset, HIGH);
    for (int pin : SleepHeldPins) ESP_ERROR_CHECK(gpio_hold_en(gpio_num_t(pin)));
    gpio_deep_sleep_hold_en();
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(DevelopmentSleepUs));
    Serial.println("Entering deep sleep for 5 seconds");
    // No serial-attachment wait or delay to keep diagnostics alive.
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    auto wake = esp_sleep_get_wakeup_cause();
    auto reset = esp_reset_reason();
    if (reset != ESP_RST_DEEPSLEEP || wake != ESP_SLEEP_WAKEUP_TIMER) sequence = 0;
    Serial.printf("Reset reason=%d wake reason=%d next sequence=%u\n", reset, wake, sequence);

    // Set safe levels before releasing the holds retained through deep sleep.
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);
    pinMode(BatteryEnable, OUTPUT);
    digitalWrite(BatteryEnable, LOW);
    pinMode(41, INPUT);
    pinMode(42, INPUT);
    pinMode(RadioNss, OUTPUT);
    digitalWrite(RadioNss, HIGH);
    pinMode(RadioReset, OUTPUT);
    digitalWrite(RadioReset, HIGH);
    for (int pin : SleepHeldPins) ESP_ERROR_CHECK(gpio_hold_dis(gpio_num_t(pin)));
    gpio_deep_sleep_hold_dis();
    digitalWrite(Vext, LOW);
    delay(50);
    analogReadResolution(12);
    analogSetPinAttenuation(BatteryAdc, ADC_0db);
    i2cReady = Wire1.begin(41, 42, 100000);
    if (!i2cReady) {
        Serial.println("Sensor I2C initialization failed; report skipped");
    } else {
        Wire1.setTimeOut(100);
        sensorReady = initializeSensor();
    }
    int16_t state = initializeRadio(radio);
    radioAttempted = true;
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialization failed: %d; report skipped\n", state);
        enterDeepSleep();
        return;
    }
    Serial.println("Remote sensor ready: 915 MHz, sync 0xD3C7, CRC on");
}
void loop() {
    sensors_event_t humidity{}, temperature{};
    bool valid = sensorReady && sht40.getEvent(&humidity, &temperature);
    valid = valid && isfinite(temperature.temperature)
        && isfinite(humidity.relative_humidity)
        && temperature.temperature >= -40 && temperature.temperature <= 125
        && humidity.relative_humidity >= 0 && humidity.relative_humidity <= 100;
    if (!valid) {
        sensorReady = false;
        Serial.println("SHT40 unavailable/invalid; report skipped, retrying");
        enterDeepSleep();
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
    enterDeepSleep();
}
