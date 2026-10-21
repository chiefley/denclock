#include <assert.h>
#include <iostream>
#include "../remote-sensor/include/config_console.h"
void send(ConfigConsole& console, const char* commands) { Serial.input = commands; console.poll(); }
int main() {
    SensorConfig c = loadConfig();
    assert(sameConfig(c, SensorConfig{}) && writes == 0);
    ConfigConsole editing(c);
    Serial.output.clear();
    editing.begin();
    assert(Serial.output.find("10 seconds") != std::string::npos);
    assert(Serial.output.substr(Serial.output.size() - 2) == "> ");
    Serial.output.clear();
    send(editing, "sh");
    assert(Serial.output == "sh" && !editing.active);
    send(editing, "x\bow\r\n");
    assert(editing.active);
    assert(Serial.output.find("shx\b \bow\r\n") == 0);
    assert(Serial.output.substr(Serial.output.size() - 2) == "> ");
    Serial.output.clear();
    send(editing, "bad\r\n");
    assert(Serial.output.substr(Serial.output.size() - 2) == "> ");
    assert(Serial.output.find("\n> ") == Serial.output.rfind("\n> "));
    send(editing, "set node 9\1773\n");
    assert(c.node == 3);
    Serial.output.clear();
    send(editing, "exit\n");
    assert(Serial.output.find("> ") == std::string::npos);
    assert(c.node == 2);
    ConfigConsole console(c);
    send(console, "garbage\r\n"); assert(!console.active);
    send(console, "?\r\nset node 3\nshow\r");
    assert(console.active && c.node == 3 && writes == 0);
    assert(Serial.output.find("Modified (unsaved)") != std::string::npos);
    send(console, "save\nshow\n");
    assert(console.active && writes == 1 && loadConfig().node == 3);
    send(console, "defaults\nexit\n");
    assert(console.finished && c.node == 3 && writes == 1);
    ConfigConsole timeout(c);
    send(timeout, "set node 4\n");
    fakeMillis += 299999; send(timeout, "bad\n"); assert(timeout.active);
    ++fakeMillis; timeout.poll(); assert(timeout.finished && c.node == 3);
    ConfigConsole failure(c);
    send(failure, "set node 5\n"); failSave = true;
    send(failure, "save\nexit\n"); assert(c.node == 3 && loadConfig().node == 3);
    ConfigConsole reboot(c);
    send(reboot, "set node 6\nreboot\n");
    assert(ESP.restarted && loadConfig().node == 3);
    ConfigConsole timer(c);
    send(timer, "show\n"); fakeMillis += 299999;
    send(timer, "help\n"); fakeMillis += 299999; timer.poll(); assert(timer.active);
    ++fakeMillis; timer.poll(); assert(timer.finished);
    flash = "bad"; assert(sameConfig(loadConfig(), SensorConfig{}));
    SensorConfig invalid; invalid.version = 99;
    flash.assign(reinterpret_cast<const char*>(&invalid), sizeof(invalid));
    assert(sameConfig(loadConfig(), SensorConfig{}));
    std::cout << "Configuration console tests passed\n";
}
