#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "gateway_config.h"

inline GatewayConfig loadConfig() {
    Preferences prefs;
    GatewayConfig stored;
    bool complete = false;
    if (prefs.begin("denclock-gateway", true)) {
        if (prefs.getBytesLength("config") == sizeof(stored))
            complete = prefs.getBytes("config", &stored, sizeof(stored)) == sizeof(stored);
        prefs.end();
    }
    return validatedConfig(stored, complete);
}
inline bool saveConfig(const GatewayConfig& config) {
    if (!validConfig(config)) return false;
    Preferences prefs;
    if (!prefs.begin("denclock-gateway", false)) return false;
    bool ok = prefs.putBytes("config", &config, sizeof(config)) == sizeof(config);
    prefs.end();
    return ok;
}
// Edits are staged here; the running configuration changes only after a reboot.
// poll() handles only bytes already received, so the receive loop never waits.
struct GatewayConsole {
    const GatewayConfig& running;
    GatewayConfig saved, working;
    CommandLine line;
    bool active = false;
    uint32_t lastCommand = 0;
    bool previousWasCr = false;
    explicit GatewayConsole(const GatewayConfig& config) : running(config), saved(config), working(config) {}
    void prompt() { if (active) Serial.printf("> "); }
    void begin() {
        saved = working = running;
        Serial.println("Enter config to edit gateway configuration.");
    }
    void show() {
        Serial.printf("DenClock Gateway Configuration\n\nLoRa Frequency: %.3f MHz\nLoRa Sync:      0x%04X\nService Host:   %s\nService Port:   %u\n\nConfiguration: %s%s\n",
            working.frequencyKhz / 1000.0, unsigned(working.sync), working.host, unsigned(LORA_SERVICE_PORT),
            sameConfig(working, saved) ? "Saved" : "Modified (unsaved)",
            sameConfig(saved, running) ? "" : " (reboot to apply)");
    }
    void exit() { working = saved; active = false; }
    void poll() {
        if (active && uint32_t(millis() - lastCommand) >= 300000) {
            Serial.println("Configuration timeout; unsaved changes discarded.");
            exit();
        }
        while (Serial.available()) {
            const char ch = char(Serial.read());
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
            ConsoleCommand command = ConsoleCommand::Invalid;
            GatewayConfig candidate = working;
            if (!line.invalid) command = parseCommand(line.text, candidate);
            line.clear();
            if (!active) {
                // Outside configuration mode only the entry command is recognized.
                if (command != ConsoleCommand::Config) {
                    Serial.println("Enter config to edit gateway configuration.");
                    continue;
                }
                active = true;
                lastCommand = millis();
                Serial.println("DenClock Gateway Configuration\nEnter ? for help.");
                prompt();
                continue;
            }
            if (command == ConsoleCommand::Invalid || command == ConsoleCommand::Config) {
                Serial.println("Invalid command or value; check ranges. Enter ? for help.");
                prompt();
                continue;
            }
            working = candidate;
            lastCommand = millis();
            switch (command) {
            case ConsoleCommand::Help:
                Serial.println(
                    "? / help          Command reference\nshow              Current settings and save status\n"
                    "set frequency N   902125..927875 kHz (integer)\n"
                    "set sync HHHH     0000..FFFF hex register bytes; optional 0x prefix\n"
                    "set host NAME     DNS name, .local mDNS name or IPv4 address; 1..253 characters\n"
                    "save              Save all settings; stay in console\n"
                    "defaults          Development defaults in RAM only\n"
                    "reboot            Restart without saving; saved settings apply\n"
                    "exit              Discard unsaved changes; leave configuration\n"
                    "Examples: set frequency 915500; set sync D3C7; set sync 0xD3C7;\n"
                    "set host denclock-d3c7.local; set host 192.168.50.90\n"
                    "(enter each command on its own line).\n"
                    "Service port is fixed. Sync words are radio parameters, not credentials.\n"
                    "Gateway reception continues; saved changes apply after reboot.\n"
                    "Five minutes without a valid command exits and discards unsaved changes.");
                break;
            case ConsoleCommand::Show: show(); break;
            case ConsoleCommand::Set: Serial.println("Updated in RAM. Enter show to inspect or save to persist."); break;
            case ConsoleCommand::Defaults: working = GatewayConfig{}; show(); break;
            case ConsoleCommand::Save:
                if (saveConfig(working)) { saved = working; Serial.println("Configuration saved; reboot to apply."); }
                else Serial.println("Save failed; saved configuration unchanged. Enter ? for help.");
                break;
            case ConsoleCommand::Reboot: Serial.flush(); ESP.restart(); return;
            case ConsoleCommand::Exit: exit(); Serial.println("Configuration closed."); break;
            default: break;
            }
            prompt();
        }
    }
};
