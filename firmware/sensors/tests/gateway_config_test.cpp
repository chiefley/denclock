#include <assert.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lora-gateway/include/gateway_config.h"
#include "../common/radio_sync.h"
ConsoleCommand run(const char* input, GatewayConfig& c) {
    char line[400];
    strcpy(line, input);
    return parseCommand(line, c);
}
int main() {
    GatewayConfig c;
    assert(validConfig(c));
    assert(c.version == 1 && c.frequencyKhz == 915000 && c.sync == 0xD3C7);
    assert(!strcmp(c.host, LORA_SERVICE_HOST));
    for (const char* command : {"set frequency 902125", "SET FREQUENCY 927875", "set frequency 915500"})
        assert(run(command, c) == ConsoleCommand::Set);
    assert(c.frequencyKhz == 915500);
    for (const char* command : {"set sync d3c7", "SET SYNC 0XD3C7", "set sync 0000", "set sync fFfF", "set sync 0xab12"})
        assert(run(command, c) == ConsoleCommand::Set);
    assert(c.sync == 0xAB12);
    // The configured register value reaches RadioLib through the shared conversion.
    const RadioLibSync configured = radioLibSync(c.sync);
    assert(configured.syncWord == 0xA1 && configured.controlBits == 0xB2);
    const RadioLibSync defaults = radioLibSync(GatewayConfig{}.sync);
    assert(defaults.syncWord == 0xDC && defaults.controlBits == 0x37);
    // Hostnames keep their case; keywords do not need to.
    for (const char* command : {"set host denclock-d3c7.local", "SET HOST DenClock-D3C7.Local", "set host den", "set host a.b-c.example.com", "set host 10.0.0.1", "set host 255.255.255.255", "set host 0.0.0.0", "set host 1host.local", "set host 192.168.50.90"})
        assert(run(command, c) == ConsoleCommand::Set);
    assert(run("Set Host DenClock-D3C7.Local", c) == ConsoleCommand::Set);
    assert(!strcmp(c.host, "DenClock-D3C7.Local"));
    const std::string label63(63, 'a');
    std::string longest = label63 + "." + label63 + "." + label63 + "." + std::string(61, 'b');
    assert(longest.size() == MaxHostLength);
    assert(run(("set host " + longest).c_str(), c) == ConsoleCommand::Set);
    assert(!strcmp(c.host, longest.c_str()) && validConfig(c));
    assert(run("  set\thost   DenClock-D3C7.Local  ", c) == ConsoleCommand::Set);
    GatewayConfig comparison = c;
    ++comparison.frequencyKhz; assert(!sameConfig(c, comparison));
    comparison = c; --comparison.sync; assert(!sameConfig(c, comparison));
    comparison = c; comparison.host[0] = 'd'; assert(!sameConfig(c, comparison));
    const std::string overlong = "set host " + longest + "b";
    const std::string longLabel = "set host " + label63 + "a.local";
    for (const std::string& bad : std::initializer_list<std::string>{
             "set frequency 902124", "set frequency 927876", "set frequency -915000", "set frequency 915.5",
             "set frequency 915000x", "set frequency 999999999999999999999", "set frequency", "set frequency 915000 extra",
             "set sync", "set sync D3C7 extra", "set sync 0x", "set sync D3C", "set sync D3C70", "set sync 0xD3C70",
             "set sync D3G7", "set sync -123", "set sync +123", "set sync 0x-123",
             "set host", "set host den clock.local", "set host den_clock.local", "set host -den.local",
             "set host den-.local", "set host .local", "set host den..local", "set host den.local.", "set host den.local extra",
             "set host 192.168.1", "set host 192.168.1.256", "set host 192.168.01.1", "set host 1.2.3.4.5",
             "set host 1.2.3.", "set host 1..2.3", "set host 1234.1.1.1", "set host den.123", "set host 123",
             "set host http://den.local", "set host den.local:5210", "set host den@local", overlong, longLabel,
             "set port 5210", "set node 1", "set", "set host den.local extra", "show extra", "config now", "frequency 915000"}) {
        GatewayConfig before = c;
        assert(run(bad.c_str(), c) == ConsoleCommand::Invalid);
        assert(sameConfig(c, before));
    }
    assert(!validHost(""));
    for (const char* good : {"?", "HELP", "show", "save", "defaults", "reboot", "exit", "CONFIG"})
        assert(run(good, c) != ConsoleCommand::Invalid);
    assert(run("Config", c) == ConsoleCommand::Config);
    assert(sameConfig(validatedConfig(c, true), c));
    assert(sameConfig(validatedConfig(c, false), GatewayConfig{}));
    GatewayConfig invalid = c; invalid.version = 2;
    assert(sameConfig(validatedConfig(invalid, true), GatewayConfig{}));
    invalid = c; invalid.frequencyKhz = 902124;
    assert(sameConfig(validatedConfig(invalid, true), GatewayConfig{}));
    invalid = c; invalid.host[0] = '_';
    assert(sameConfig(validatedConfig(invalid, true), GatewayConfig{}));
    invalid = c; memset(invalid.host, 'a', sizeof(invalid.host)); // No terminator.
    assert(!validConfig(invalid));
    assert(sameConfig(validatedConfig(invalid, true), GatewayConfig{}));
    CommandLine line;
    for (char ch : {'s','h','o','w'}) assert(!line.feed(ch));
    assert(line.feed('\r')); line.clear(); assert(!line.feed('\n'));
    for (unsigned i = 0; i + 1 < sizeof(line.text); ++i) line.feed('x');
    assert(!line.invalid && line.length == sizeof(line.text) - 1);
    line.feed('x'); assert(line.feed('\n') && line.invalid); line.clear();
    line.feed('s'); line.feed('\0'); assert(line.feed('\n') && line.invalid);
    std::cout << "Gateway configuration tests passed\n";
}
