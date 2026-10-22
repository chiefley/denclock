#include <assert.h>
#include <iostream>
#include "../lora-gateway/include/gateway_console.h"
using namespace GatewayConfiguration;
void send(Console& console, const std::string& input) {
    Serial.input = input;
    while (Serial.available()) console.poll();
}
int main() {
    const Config active = load();
    assert(same(active, Config{}) && writes == 0);
    Console console(active);
    send(console, "set host forbidden.local\nshow\n");
    assert(!console.active && same(console.working, active));
    send(console, "confiX\bg\r\n");
    assert(console.active);
    assert(Serial.output.find("confiX\b \bg\r\n") != std::string::npos);
    assert(Serial.output.substr(Serial.output.size() - 2) == "> ");
    send(console, "SET HOST DenClock-Test.local\nset frequency 915500\nset sync 0X1234\nshow\n");
    assert(writes == 0 && !strcmp(console.working.host, "DenClock-Test.local"));
    assert(Serial.output.find("915.500 MHz") != std::string::npos);
    assert(Serial.output.find("0x1234") != std::string::npos);
    assert(Serial.output.find("Modified (unsaved)") != std::string::npos);
    assert(same(active, Config{})); // Console never mutates the running snapshot.
    send(console, "save\nshow\n");
    assert(writes == 1 && console.active && same(load(), console.working));
    assert(same(active, Config{}));
    send(console, "defaults\nexit\nconfig\n");
    assert(console.active && same(console.working, load()));
    send(console, "set host Other.local\n");
    fakeMillis += 299999;
    send(console, "bad\n");
    ++fakeMillis; console.poll();
    assert(!console.active && same(console.working, load()));
    send(console, "config\nshow\rhelp\n");
    fakeMillis += 299999; send(console, "show\n");
    fakeMillis += 299999; console.poll(); assert(console.active);
    const Config saved = console.saved;
    send(console, "set host Failed.local\n"); failSave = true;
    send(console, "save\nexit\n");
    assert(same(console.working, saved) && same(load(), saved));
    unsigned previousWrites = writes;
    send(console, "config\nset host Unsaved.local\nreboot\n");
    assert(ESP.restarted && writes == previousWrites && same(load(), saved));
    send(console, "exit\nconfig\nset sync ABX\177CD\n");
    assert(console.working.sync == 0xABCD);
    Config before = console.working;
    send(console, std::string(400, 'x') + "\b\n");
    assert(same(console.working, before));
    send(console, std::string("set host Good") + char(0) + ".local\n");
    assert(same(console.working, before));
    Serial.input = std::string(1000, 'x');
    console.poll(); assert(Serial.input.size() == 936); // Fairness per loop.
    Serial.input.clear();
    Config invalid; invalid.version = 2;
    flash.assign(reinterpret_cast<const char*>(&invalid), sizeof(invalid));
    assert(same(load(), Config{}) && writes == previousWrites);
    invalid = Config{}; memset(invalid.host, 'a', sizeof(invalid.host));
    flash.assign(reinterpret_cast<const char*>(&invalid), sizeof(invalid));
    assert(same(load(), Config{}));
    invalid = Config{}; invalid.frequencyKhz = 1;
    flash.assign(reinterpret_cast<const char*>(&invalid), sizeof(invalid));
    assert(same(load(), Config{}));
    flash = "short"; assert(same(load(), Config{}));
    std::cout << "Gateway console tests passed\n";
}
