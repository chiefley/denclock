#include <assert.h>
#include <string.h>
#include <iostream>
#include "../remote-sensor/include/reporting_policy.h"
Command run(const char* input, SensorConfig& c) {
    char line[200];
    strcpy(line, input);
    return parseCommand(line, c);
}
int main() {
    SensorConfig c;
    assert(validConfig(c));
    assert(c.version == 2 && c.frequencyKhz == 915000 && c.sync == 0xD3C7);
    for (const char* command : {"set frequency 902125", "SET FREQUENCY 927875", "set frequency 915500"})
        assert(run(command, c) == Command::Set);
    assert(c.frequencyKhz == 915500);
    for (const char* command : {"set sync d3c7", "SET SYNC 0XD3C7", "set sync 0000", "set sync fFfF"})
        assert(run(command, c) == Command::Set);
    assert(c.sync == 0xFFFF);
    SensorConfig comparison = c;
    ++comparison.frequencyKhz; assert(!sameConfig(c, comparison));
    comparison = c; --comparison.sync; assert(!sameConfig(c, comparison));
    for (const char* bad : {"set frequency 902124", "set frequency 927876", "set frequency -915000", "set frequency 915.5", "set frequency 915000x", "set frequency 999999999999999999999", "set frequency", "set frequency 915000 extra", "set sync", "set sync D3C7 extra", "set sync 0x", "set sync D3C", "set sync D3C70", "set sync 0xD3C70", "set sync D3G7", "set sync -123", "set sync +123", "set sync 0x-123"}) {
        SensorConfig before = c;
        assert(run(bad, c) == Command::Invalid);
        assert(sameConfig(c, before));
    }
    SensorConfig invalidFrequency;
    invalidFrequency.frequencyKhz = 902124;
    assert(sameConfig(validatedConfig(invalidFrequency, true), SensorConfig{}));
    assert(c.node == 2 && c.sample == 5 && c.jitter == 1 && c.heartbeat == 30 && c.threshold == 1 && c.power == 10);
    assert(run("  SeT\tNoDe 239  ", c) == Command::Set && c.node == 239);
    for (const char* bad : {"set", "show extra", "set node", "set node 0", "set node 240", "set node 1 extra", "set jitter 5", "set sample 1", "set power -10", "set power 23", "set threshold 0", "set heartbeat 604801", "set sample 86401", "set jitter 2148", "set node 999999999999999999999999", "set node 2x", "set node 1.0", "set heartbeat -1"}) {
        SensorConfig before = c;
        assert(run(bad, c) == Command::Invalid);
        assert(sameConfig(before, c));
    }
    for (const char* good : {"?", "HELP", "show", "save", "defaults", "reboot", "exit"})
        assert(run(good, c) != Command::Invalid);
    assert(run("set jitter 0", c) == Command::Set);
    assert(run("set sample 1", c) == Command::Set);
    assert(sampleInterval(UINT32_MAX, c) == 1000000);
    assert(run("set sample 86400", c) == Command::Set);
    assert(run("set jitter 2147", c) == Command::Set);
    assert(sampleInterval(UINT32_MAX, c) <= 88547000000ULL);
    assert(run("set heartbeat 300", c) == Command::Set);
    assert(run("set threshold 5", c) == Command::Set);
    ReportingState state{true, 20.004f, 0};
    assert(!reportDue(state, 20.50f, 299999999, c));
    assert(reportDue(state, 20.505f, 1, c));
    assert(reportDue(state, 20.004f, 300000000, c));
    assert(sameConfig(validatedConfig(c, true), c));
    assert(sameConfig(validatedConfig(c, false), SensorConfig{}));
    c.version = 1;
    assert(sameConfig(validatedConfig(c, true), SensorConfig{}));
    c = SensorConfig{}; c.node = 0;
    assert(sameConfig(validatedConfig(c, true), SensorConfig{}));
    CommandLine line;
    for (char ch : {'s','h','o','w'}) assert(!line.feed(ch));
    assert(line.feed('\r')); line.clear(); assert(!line.feed('\n'));
    for (int i = 0; i < 200; ++i) line.feed('x');
    assert(line.feed('\n') && line.invalid); line.clear();
    line.feed('s'); line.feed('\0'); assert(line.feed('\n') && line.invalid);
    std::cout << "Sensor configuration tests passed\n";
}
