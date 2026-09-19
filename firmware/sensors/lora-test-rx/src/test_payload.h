#pragma once

#include <cmath>
#include <cstddef>
#include <cstdio>

struct TestReadings
{
    float temperature;
    float humidity;
    float battery;
};

// A disposable bench-test text format, not the future telemetry protocol.
// The caller supplies a NUL-terminated string plus its actual packet length.
inline bool parseTestPayload(const char* text, size_t length, TestReadings& result)
{
    TestReadings parsed{};
    int consumed = 0;
    if (std::sscanf(text, "T=%f C RH=%f %% BAT=%f V%n",
            &parsed.temperature, &parsed.humidity, &parsed.battery, &consumed) != 3
        || consumed == 0 || static_cast<size_t>(consumed) != length)
        return false;

    const bool missingSensor = std::isnan(parsed.temperature) && std::isnan(parsed.humidity);
    const bool validSensor = std::isfinite(parsed.temperature)
        && parsed.temperature >= -40 && parsed.temperature <= 125
        && std::isfinite(parsed.humidity) && parsed.humidity >= 0 && parsed.humidity <= 100;
    if ((!missingSensor && !validSensor) || !std::isfinite(parsed.battery)
        || parsed.battery < 0 || parsed.battery > 6)
        return false;

    result = parsed;
    return true;
}
