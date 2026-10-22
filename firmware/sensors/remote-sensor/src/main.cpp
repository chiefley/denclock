#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <SSD1306Wire.h>
#include <math.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <bootloader_random.h>
#include <sys/time.h>
#include "reporting_policy.h"
#include "config_console.h"
#include <driver/gpio.h>
#include "../../common/radio_config.h"
#include "../../common/sensor_report.h"

SX1262 radio = new Module(RadioNss, RadioDio1, RadioReset, RadioBusy);
Adafruit_SHT4x sht40;
SensorConfig config;
uint32_t startupStarted;
uint32_t cycleRandom = 0;
RTC_DATA_ATTR ReportingState reportingState = {};

uint64_t currentTimeUs() {
    timeval now{};
    gettimeofday(&now, nullptr); // ESP32 RTC-backed time continues through deep sleep.
    return uint64_t(now.tv_sec) * 1000000ULL + now.tv_usec;
}
constexpr int BatteryAdc = 1, BatteryEnable = 37;
constexpr int OledSda = 17, OledScl = 18, OledReset = 21;
bool sensorReady = false;
RTC_DATA_ATTR uint16_t sequence = 0;
bool i2cReady = false;
bool radioAttempted = false;
constexpr int SleepHeldPins[] = {
    Vext, BatteryEnable, 41, 42, RadioNss, RadioReset,
    OledSda, OledScl, OledReset
};

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
void showResetStatus(ConfigConsole& console) {
    // Vext is shared with the SHT40; show diagnostics before initializing it.
    digitalWrite(Vext, LOW);
    delay(50);
    digitalWrite(OledReset, HIGH);
    delay(20);
    SSD1306Wire display(0x3C, OledSda, OledScl);
    if (display.init()) {
        display.flipScreenVertically();
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        char line[16];
        snprintf(line, sizeof(line), "NODE %02u", unsigned(config.node));
        display.setFont(ArialMT_Plain_24);
        display.drawString(64, 5, line);
        snprintf(line, sizeof(line), "%.2f V", readBatteryMillivolts() / 1000.0);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 39, line);
        display.display();
        const uint32_t shown = millis();
        while (!console.finished && uint32_t(millis() - shown) < 5000) {
            if (!console.finished) console.poll();
            delay(1);
        }
        display.displayOff();
    } else {
        Serial.println("OLED initialization failed; continuing without display");
    }
    Wire.end();
    pinMode(OledSda, INPUT);
    pinMode(OledScl, INPUT);
    digitalWrite(OledReset, LOW);
    digitalWrite(Vext, HIGH);
    delay(20); // Allow the shared supply to power down before sensor startup.
    // Keep OLED reset LOW when Vext is restored for the SHT40.
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
    pinMode(OledSda, INPUT);
    pinMode(OledScl, INPUT);
    pinMode(OledReset, OUTPUT);
    digitalWrite(OledReset, LOW);
    digitalWrite(Vext, HIGH); // Active-low peripheral supply.
    digitalWrite(BatteryEnable, LOW);
    // NSS must remain high so the sleeping SX1262 is not woken by a low edge.
    pinMode(RadioNss, OUTPUT);
    digitalWrite(RadioNss, HIGH);
    pinMode(RadioReset, OUTPUT);
    digitalWrite(RadioReset, HIGH);
    for (int pin : SleepHeldPins) ESP_ERROR_CHECK(gpio_hold_en(gpio_num_t(pin)));
    gpio_deep_sleep_hold_en();
    const uint64_t intervalUs = sampleInterval(cycleRandom, config);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(intervalUs));
    Serial.printf("Entering deep sleep for %llu us\n", intervalUs);
    // No serial-attachment wait or delay to keep diagnostics alive.
    esp_deep_sleep_start();
}

void setup() {
    startupStarted = millis();
    Serial.begin(115200);
    config = loadConfig();
    auto wake = esp_sleep_get_wakeup_cause();
    auto reset = esp_reset_reason();
    if (reset != ESP_RST_DEEPSLEEP || wake != ESP_SLEEP_WAKEUP_TIMER) {
        sequence = 0;
        reportingState = {};
    }
    // Obtain fresh hardware entropy every wake, before battery ADC initialization.
    // Disable the entropy source before using the ADC (they share hardware).
    bootloader_random_enable();
    cycleRandom = esp_random();
    bootloader_random_disable();
    Serial.printf("Reset reason=%d wake reason=%d next sequence=%u\n", reset, wake, sequence);

    // Set safe levels before releasing the holds retained through deep sleep.
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);
    pinMode(BatteryEnable, OUTPUT);
    digitalWrite(BatteryEnable, LOW);
    pinMode(41, INPUT);
    pinMode(42, INPUT);
    pinMode(OledSda, INPUT);
    pinMode(OledScl, INPUT);
    pinMode(OledReset, OUTPUT);
    digitalWrite(OledReset, LOW);
    pinMode(RadioNss, OUTPUT);
    digitalWrite(RadioNss, HIGH);
    pinMode(RadioReset, OUTPUT);
    digitalWrite(RadioReset, HIGH);
    for (int pin : SleepHeldPins) ESP_ERROR_CHECK(gpio_hold_dis(gpio_num_t(pin)));
    gpio_deep_sleep_hold_dis();
    analogReadResolution(12);
    analogSetPinAttenuation(BatteryAdc, ADC_0db);
    if (reset != ESP_RST_DEEPSLEEP) {
        ConfigConsole console(config);
        console.begin();
        showResetStatus(console);
        while (!console.finished && (console.active || uint32_t(millis() - startupStarted) < 10000)) {
            console.poll();
            delay(1);
        }
    }
    digitalWrite(Vext, LOW);
    delay(50);
    i2cReady = Wire1.begin(41, 42, 100000);
    if (!i2cReady) {
        Serial.println("Sensor I2C initialization failed; report skipped");
    } else {
        Wire1.setTimeOut(100);
        sensorReady = initializeSensor();
    }
    int16_t state = initializeRadio(radio, config.power, config.frequencyKhz, config.sync);
    radioAttempted = true;
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialization failed: %d; report skipped\n", state);
        enterDeepSleep();
        return;
    }
    Serial.printf("Remote sensor ready: %.3f MHz, sync 0x%04X, CRC on\n",
                  config.frequencyKhz / 1000.0, unsigned(config.sync));
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
    const float rawTemperature = temperature.temperature;
    const uint64_t nowUs = currentTimeUs();
    if (!reportDue(reportingState, rawTemperature, nowUs, config)) {
        Serial.printf("Report suppressed: T=%.5f C last=%.5f C age=%llu us\n",
                      rawTemperature, reportingState.lastTransmittedRawTemperature,
                      nowUs - reportingState.lastTransmittedUs);
        enterDeepSleep();
        return;
    }
    Serial.printf("Report due: %s T=%.5f C\n",
                  !reportingState.hasTransmitted ? "initial" :
                  (nowUs - reportingState.lastTransmittedUs >= uint64_t(config.heartbeat) * 1000000ULL
                   ? "heartbeat" : "temperature"), rawTemperature);
    int16_t temp = lroundf(rawTemperature * 100);
    uint16_t rh = lroundf(humidity.relative_humidity * 100);
    uint16_t battery = readBatteryMillivolts();
    uint8_t packet[SensorReport::PacketSize];
    SensorReport::encode(packet, uint8_t(config.node), sequence, temp, rh, battery);
    Serial.printf("TX node=%u seq=%u T=%.2f C RH=%.2f %% BAT=%u mV bytes=",
                  unsigned(config.node), sequence, temp / 100.0, rh / 100.0, battery);
    for (uint8_t byte : packet) Serial.printf("%02X ", byte);
    int16_t state = radio.transmit(packet, sizeof(packet));
    Serial.printf("result=%d\n", state);
    if (state == RADIOLIB_ERR_NONE) {
        reportingState = {true, rawTemperature, currentTimeUs()};
    }
    // A generated report consumes its sequence even if transmission fails.
    sequence = SensorReport::nextSequence(sequence);
    enterDeepSleep();
}
