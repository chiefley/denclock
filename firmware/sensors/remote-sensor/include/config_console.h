#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

inline SensorConfig loadConfig() {
    Preferences prefs;
    SensorConfig stored;
    bool complete = false;
    if (prefs.begin("denclock-sensor", true)) {
        if (prefs.getBytesLength("config") == sizeof(stored))
            complete = prefs.getBytes("config", &stored, sizeof(stored)) == sizeof(stored);
        prefs.end();
    }
    return validatedConfig(stored, complete);
}
inline bool saveConfig(const SensorConfig& config) {
    if (!validConfig(config)) return false;
    Preferences prefs;
    if (!prefs.begin("denclock-sensor", false)) return false;
    bool ok = prefs.putBytes("config", &config, sizeof(config)) == sizeof(config);
    prefs.end();
    return ok;
}
struct ConfigConsole {
    SensorConfig& working;
    SensorConfig saved;
    CommandLine line;
    bool active = false, finished = false;
    uint32_t lastCommand = 0;
    bool previousWasCr = false;
    explicit ConfigConsole(SensorConfig& config) : working(config), saved(config) {}
    void prompt() { if (!finished) Serial.printf("> "); }
    void begin() {
        Serial.println("Enter a configuration command within 10 seconds; ? for help.");
        prompt();
    }
    void show() {
        Serial.printf("Node ID:       %ld\nSample:        %ld seconds\nJitter:        +/- %ld seconds\nHeartbeat:     %ld seconds\nThreshold:     %ld (%.1f C)\nTX Power:      %ld dBm\n\nConfiguration: %s\n",
            long(working.node), long(working.sample), long(working.jitter), long(working.heartbeat),
            long(working.threshold), working.threshold / 10.0, long(working.power),
            sameConfig(working, saved) ? "Saved" : "Modified (unsaved)");
    }
    void exit() { working = saved; active = false; finished = true; }
    void poll() {
        if (active && uint32_t(millis() - lastCommand) >= 300000) {
            Serial.println("Configuration timeout; unsaved changes discarded.");
            exit();
            return;
        }
        while (!finished && Serial.available()) {
            const char ch = char(Serial.read());
            // Echo as characters arrive, including before configuration entry.
            if (ch == '\n' && previousWasCr) { previousWasCr = false; continue; }
            previousWasCr = ch == '\r';
            if (ch == '\b' || ch == 127) {
                if (line.length) {
                    line.text[--line.length] = 0;
                    Serial.printf("\b \b");
                }
                // A poisoned/overlong line must still be rejected in full.
                continue;
            }
            if (ch == '\r' || ch == '\n') Serial.printf("\r\n");
            else if (static_cast<unsigned char>(ch) >= 32 || ch == '\t')
                Serial.printf("%c", ch);
            if (!line.feed(ch)) continue;
            Command command = line.invalid ? Command::Invalid : parseCommand(line.text, working);
            line.clear();
            if (command == Command::Invalid) {
                Serial.println("Invalid command or value; check ranges and sample > jitter. Enter ? for help.");
                prompt();
                continue;
            }
            if (!active) {
                active = true;
                Serial.println("DenClock Sensor Configuration\nEnter ? for help.");
            }
            lastCommand = millis();
            switch (command) {
            case Command::Help:
                Serial.println(
                    "? / help          Command reference\nshow              Current settings and save status\n"
                    "set node N        1..239 (integer node ID)\n"
                    "set sample N      1..86400 seconds; must exceed jitter\n"
                    "set jitter N      0..2147 seconds (+/-); less than sample\n"
                    "set heartbeat N   1..604800 seconds\n"
                    "set threshold N   1..1650 tenths of C (1 = 0.1 C)\n"
                    "set power N       -9..22 dBm\n"
                    "save              Save all settings; stay in console\n"
                    "defaults          Development defaults in RAM only\n"
                    "reboot            Restart without saving\n"
                    "exit              Discard unsaved changes; resume operation\n"
                    "Examples: set node 3; set sample 15; set jitter 2; set heartbeat 300;\n"
                    "set threshold 2; set power 10 (enter each command on its own line).\n"
                    "Five minutes without a valid command exits and discards unsaved changes.");
                break;
            case Command::Show: show(); break;
            case Command::Set: Serial.println("Updated in RAM. Enter show to inspect or save to persist."); break;
            case Command::Defaults: working = SensorConfig{}; show(); break;
            case Command::Save:
                if (saveConfig(working)) { saved = working; Serial.println("Configuration saved."); }
                else Serial.println("Save failed; saved configuration unchanged. Enter ? for help.");
                break;
            case Command::Reboot: finished = true; Serial.flush(); ESP.restart(); break;
            case Command::Exit: exit(); break;
            default: break;
            }
            prompt();
        }
    }
};
