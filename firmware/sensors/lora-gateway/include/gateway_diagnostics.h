#pragma once
#include "../../common/sensor_report.h"

// Local OLED diagnostics do not decide which raw packets TCP transports.
struct GatewayDiagnostics {
    static constexpr uint32_t ErrorDurationMs = 3000;
    uint32_t accepted = 0, rejected = 0;
    uint8_t node = 0; // Zero means no accepted report yet.
    uint16_t battery = 0;
    bool hasBattery = false;
    float rssi = 0, snr = 0;

    // Decoder errors are static strings: retaining the exact reason also keeps
    // malformed, unsupported-class, invalid-node and value errors distinct.
    // A subsequent success deliberately does not erase the last failure.
    const char* lastError = nullptr;
    int16_t lastRadioError = 0;
    uint32_t errorStarted = 0;
    bool errorVisible = false, dirty = true;

    void reject(const char* reason, int16_t radioError, uint32_t now) {
        ++rejected;
        lastError = reason;
        lastRadioError = radioError;
        errorStarted = now;
        errorVisible = true;
        dirty = true;
    }

    bool observe(const uint8_t* packet, size_t length, float signal, float noise,
                 uint32_t now) {
        SensorReport::Report report;
        const char* error = SensorReport::decode(packet, length, report);
        if (error) {
            reject(error, 0, now);
            return false;
        }
        ++accepted;
        node = report.node;
        battery = report.battery;
        hasBattery = report.hasBattery;
        rssi = signal;
        snr = noise;
        // Keep the error screen up, but restore the newest report on expiry.
        if (!errorVisible) dirty = true;
        return true;
    }

    void tick(uint32_t now) {
        // Unsigned elapsed time handles the millis() rollover.
        if (errorVisible && uint32_t(now - errorStarted) >= ErrorDurationMs) {
            errorVisible = false;
            dirty = true;
        }
    }
};
