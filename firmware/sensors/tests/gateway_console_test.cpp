#include <assert.h>
#include <iostream>
#include "../lora-gateway/include/gateway_console.h"
void send(GatewayConsole& console, const char* commands) { Serial.input = commands; console.poll(); }
bool printed(const char* text) { return Serial.output.find(text) != std::string::npos; }
int main() {
    GatewayConfig running = loadConfig();
    assert(sameConfig(running, GatewayConfig{}) && writes == 0);
    assert(preferencesNamespace == "denclock-gateway");
    const GatewayConfig boot = running;
    GatewayConsole console(running);
    console.begin();
    assert(printed("Enter config") && !console.active);
    // Normal operation: lines other than config are echoed but change nothing.
    Serial.output.clear();
    send(console, "set frequency 916000\r\n");
    assert(!console.active && sameConfig(console.working, boot) && sameConfig(console.saved, boot));
    assert(Serial.output.find("set frequency 916000\r\nEnter config") == 0 && !printed("> "));
    // A partial line is buffered without waiting; poll returns to the receive loop.
    Serial.output.clear();
    send(console, "con");
    assert(Serial.output == "con" && !console.active && Serial.input.empty());
    send(console, "x\bfig\r\n");
    assert(console.active);
    assert(Serial.output.find("conx\b \bfig\r\n") == 0);
    assert(Serial.output.substr(Serial.output.size() - 2) == "> ");
    Serial.output.clear();
    send(console, "bad\r\n");
    assert(printed("Invalid") && Serial.output.substr(Serial.output.size() - 2) == "> ");
    assert(Serial.output.find("\n> ") == Serial.output.rfind("\n> "));
    send(console, "config\n"); // Already active: not a configuration command.
    assert(printed("Invalid") && console.active);
    // Backspace and Delete edit the line; a lone CR then LF counts once.
    send(console, "set frequency 915509\1770\r\n");
    assert(console.working.frequencyKhz == 915500);
    send(console, "set sync 0xab12\rset host DenClock-D3C7.Local\n");
    assert(console.working.sync == 0xAB12 && !strcmp(console.working.host, "DenClock-D3C7.Local"));
    // Staged edits never touch the running configuration.
    assert(sameConfig(running, boot) && writes == 0);
    Serial.output.clear();
    send(console, "show\n");
    assert(printed("LoRa Frequency: 915.500 MHz") && printed("LoRa Sync:      0xAB12"));
    assert(printed("Service Host:   DenClock-D3C7.Local") && printed("Service Port:   5210"));
    assert(printed("Configuration: Modified (unsaved)\n"));
    // An oversized line is rejected in full, even after backspacing.
    GatewayConfig before = console.working;
    std::string overlong = "set host " + std::string(400, 'a') + "\b\b\b\n";
    send(console, overlong.c_str());
    assert(sameConfig(console.working, before));
    // Help lists ranges, units and examples.
    Serial.output.clear();
    send(console, "HELP\n");
    assert(printed("902125..927875 kHz") && printed("0x prefix") && printed("253") && printed("set host denclock-d3c7.local"));
    // Exit discards unsaved edits.
    Serial.output.clear();
    send(console, "exit\n");
    assert(!console.active && sameConfig(console.working, boot) && writes == 0 && !printed("> "));
    // Save persists without leaving and without changing the running configuration.
    send(console, "config\nset frequency 915500\nset sync D3C7\nset host denclock-d3c7.local\nsave\n");
    assert(console.active && writes == 1 && sameConfig(loadConfig(), console.working));
    assert(loadConfig().frequencyKhz == 915500 && !strcmp(loadConfig().host, "denclock-d3c7.local"));
    assert(preferencesNamespace == "denclock-gateway" && sameConfig(running, boot));
    Serial.output.clear();
    send(console, "show\n");
    assert(printed("Configuration: Saved (reboot to apply)"));
    // Defaults only change RAM.
    send(console, "defaults\n");
    assert(sameConfig(console.working, GatewayConfig{}) && writes == 1 && printed("Modified (unsaved)"));
    send(console, "exit\n");
    assert(console.working.frequencyKhz == 915500 && writes == 1);
    // Inactivity timeout discards edits; invalid input does not extend it.
    send(console, "config\nset frequency 916000\n");
    fakeMillis += 299999; send(console, "bad\n"); assert(console.active);
    ++fakeMillis; Serial.output.clear(); console.poll();
    assert(!console.active && console.working.frequencyKhz == 915500 && printed("timeout") && !printed("> "));
    // Valid commands do refresh the timer.
    send(console, "config\n"); fakeMillis += 299999;
    send(console, "show\n"); fakeMillis += 299999; console.poll(); assert(console.active);
    ++fakeMillis; console.poll(); assert(!console.active);
    // Save failure keeps the previous saved snapshot.
    failSave = true;
    send(console, "config\nset frequency 917000\nsave\nexit\n");
    assert(console.working.frequencyKhz == 915500 && loadConfig().frequencyKhz == 915500);
    failSave = false;
    // Reboot never saves implicitly.
    unsigned previousWrites = writes;
    send(console, "config\nset frequency 918000\nreboot\n");
    assert(ESP.restarted && writes == previousWrites && loadConfig().frequencyKhz == 915500);
    // A restarted gateway uses the saved configuration.
    GatewayConfig restarted = loadConfig();
    GatewayConsole after(restarted);
    after.begin();
    Serial.output.clear();
    send(after, "config\nshow\n");
    assert(printed("Configuration: Saved\n") && printed("915.500 MHz"));
    // Invalid or incompatible records fall back to defaults without writing.
    previousWrites = writes;
    flash = "bad"; assert(sameConfig(loadConfig(), GatewayConfig{}));
    GatewayConfig invalid = restarted; invalid.version = 99;
    flash.assign(reinterpret_cast<const char*>(&invalid), sizeof(invalid));
    assert(sameConfig(loadConfig(), GatewayConfig{}));
    invalid = restarted; invalid.host[0] = ' ';
    flash.assign(reinterpret_cast<const char*>(&invalid), sizeof(invalid));
    assert(sameConfig(loadConfig(), GatewayConfig{}) && writes == previousWrites);
    std::cout << "Gateway console tests passed\n";
}
