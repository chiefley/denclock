#pragma once
#include <math.h>
#include <stdint.h>
#include "config.h"

struct ReportingState {
    bool hasTransmitted;
    float lastTransmittedRawTemperature;
    uint64_t lastTransmittedUs;
};

inline bool reportDue(const ReportingState& state, float temperature, uint64_t nowUs) {
    return !state.hasTransmitted
        || fabsf(temperature - state.lastTransmittedRawTemperature) >= TemperatureChangeThresholdC
        || nowUs - state.lastTransmittedUs >= HeartbeatIntervalUs;
}

inline uint64_t sampleInterval(uint32_t random) {
    return SampleIntervalUs - SampleJitterUs
        + random % (2ULL * SampleJitterUs + 1);
}
