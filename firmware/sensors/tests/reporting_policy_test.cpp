#include <assert.h>
#include <cmath>
#include <iostream>
#include "../remote-sensor/include/reporting_policy.h"

int main() {
    ReportingState state{};
    assert(reportDue(state, 20.0f, 0));
    state = {true, 20.004f, 1000000};
    // Small changes accumulate against the transmitted baseline, not the last sample.
    for (float t : {20.03f, 20.06f, 20.09f, 20.101f})
        assert(!reportDue(state, t, 2000000));
    // 20.004 -> 20.101 rounds to a 0.10 wire change but is below the raw threshold.
    assert(reportDue(state, 20.105f, 2000000));
    assert(reportDue(state, 19.90f, 2000000));
    assert(!reportDue(state, 20.004f, state.lastTransmittedUs + HeartbeatIntervalUs - 1));
    assert(reportDue(state, 20.004f, state.lastTransmittedUs + HeartbeatIntervalUs));
    // A failed send leaves state alone, so a due heartbeat remains due.
    assert(reportDue(state, 20.004f, state.lastTransmittedUs + HeartbeatIntervalUs + 1));
    state = {true, 20.105f, 40000000};
    assert(!reportDue(state, 20.11f, 40000001));
    state = {true, 0.0f, 0};
    assert(reportDue(state, TemperatureChangeThresholdC, 1));
    assert(reportDue(state, -TemperatureChangeThresholdC, 1));
    assert(!reportDue(state, std::nextafter(TemperatureChangeThresholdC, 0.0f), 1));
    assert(sampleInterval(0) == SampleIntervalUs - SampleJitterUs);
    assert(sampleInterval(2 * SampleJitterUs) == SampleIntervalUs + SampleJitterUs);
    for (uint32_t random : {0U, 1U, 12345678U, UINT32_MAX}) {
        assert(sampleInterval(random) > 0);
        assert(sampleInterval(random) >= SampleIntervalUs - SampleJitterUs);
        assert(sampleInterval(random) <= SampleIntervalUs + SampleJitterUs);
    }
    std::cout << "Reporting policy tests passed\n";
}
