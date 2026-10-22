#include <assert.h>
#include <string>
#include <iostream>
#include "../lora-gateway/include/gateway_config.h"
using namespace GatewayConfiguration;
Command run(const std::string& input, Config& c) {
    std::string text = input;
    return parse(&text[0], c);
}
int main() {
    Config c;
    assert(valid(c) && c.version == 1 && c.frequencyKhz == 915000 && c.sync == 0xD3C7);
    assert(!strcmp(c.host, LORA_SERVICE_HOST));
    for (const char* command : {"set frequency 902125", "SET frequency 927875", "set frequency 915500", "set sync 0000", "set sync FFFF", "  SeT\tSyNc 0Xd3C7  "})
        assert(run(command, c) == Command::Set);
    assert(c.frequencyKhz == 915500 && c.sync == 0xD3C7);
    for (const char* host : {"DenClock-D3C7.local", "server", "A.b.example.com.", "192.168.50.90", "0.0.0.0", "255.255.255.255"}) {
        assert(run(std::string("set host ") + host, c) == Command::Set);
        assert(!strcmp(c.host, host));
    }
    std::string longest = std::string(63, 'a') + '.' + std::string(63, 'b') + '.' + std::string(63, 'c') + '.' + std::string(61, 'd');
    assert(longest.size() == MaxHostLength);
    assert(run("set host " + longest, c) == Command::Set);
    for (const std::string& host : {std::string("-bad.local"), std::string("bad-.local"), std::string("bad..local"), std::string(".local"), std::string("http://host"), std::string("bad_name"), std::string("host:5210"), std::string("host/name"), std::string("256.1.2.3"), std::string("1.2.3"), std::string("1.2.3.4.5"), std::string("1..2.3"), std::string("1.2.3.4."), std::string("01.2.3.4"), std::string("::1"), std::string(64, 'a'), longest + 'x'}) {
        Config before = c;
        assert(run("set host " + host, c) == Command::Invalid);
        assert(same(c, before));
    }
    for (const char* command : {"set frequency 902124", "set frequency 927876", "set frequency 915500.0", "set frequency -1", "set frequency 99999999999999999999999", "set sync D3C", "set sync 0xD3C70", "set sync -123", "set sync 0xGGGG", "set host", "set host a b", "set sync D3C7 extra", "show extra", "set port 5210"}) {
        Config before = c;
        assert(run(command, c) == Command::Invalid && same(c, before));
    }
    for (const char* command : {"CONFIG", "?", "HELP", "SHOW", "SAVE", "DEFAULTS", "EXIT", "REBOOT"})
        assert(run(command, c) != Command::Invalid);
    Config comparison = c;
    ++comparison.frequencyKhz; assert(!same(c, comparison));
    comparison = c; ++comparison.sync; assert(!same(c, comparison));
    comparison = c; comparison.host[0] = 'z'; assert(!same(c, comparison));
    c.version = 2; assert(!valid(c));
    c = Config{}; memset(c.host, 'a', sizeof(c.host)); assert(!valid(c));
    std::cout << "Gateway configuration tests passed\n";
}
