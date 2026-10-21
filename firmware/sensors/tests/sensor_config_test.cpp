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
    c.version = 2;
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
