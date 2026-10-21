#pragma once
#include <math.h>
#include <stdint.h>
#include "config.h"

struct ReportingState {
    bool hasTransmitted;
    float lastTransmittedRawTemperature;
    uint64_t lastTransmittedUs;
};

inline bool reportDue(const ReportingState& state, float temperature, uint64_t nowUs, const SensorConfig& config = SensorConfig{}) {
    return !state.hasTransmitted
        || fabsf(temperature - state.lastTransmittedRawTemperature) >= config.threshold / 10.0f
        || nowUs - state.lastTransmittedUs >= uint64_t(config.heartbeat) * 1000000ULL;
}

inline uint64_t sampleInterval(uint32_t random, const SensorConfig& config = SensorConfig{}) {
    return uint64_t(config.sample - config.jitter) * 1000000ULL
        + random % (2ULL * config.jitter * 1000000ULL + 1);
}
