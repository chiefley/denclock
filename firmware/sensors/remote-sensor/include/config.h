#pragma once
#include <stdint.h>

// Short development intervals; sleep excludes sensor/radio processing time.
constexpr uint64_t SampleIntervalUs = 5ULL * 1000000ULL;
constexpr uint32_t SampleJitterUs = 1000000; // +/- one second.
constexpr float TemperatureChangeThresholdC = 0.1f;
constexpr uint64_t HeartbeatIntervalUs = 30ULL * 1000000ULL;
static_assert(SampleIntervalUs > SampleJitterUs, "Sleep must remain positive");
static_assert(SampleJitterUs <= (UINT32_MAX - 1) / 2, "Jitter range must fit");
static_assert(TemperatureChangeThresholdC > 0, "Threshold must be positive");
static_assert(HeartbeatIntervalUs > 0, "Heartbeat must be positive");
