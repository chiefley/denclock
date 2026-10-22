#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "gateway_config.h"

namespace GatewayConfiguration {
inline Config load() {
    Preferences prefs;
    Config stored;
    bool complete = false;
    if (prefs.begin("denclock-gw", true)) {
        if (prefs.getBytesLength("config") == sizeof(stored))
            complete = prefs.getBytes("config", &stored, sizeof(stored)) == sizeof(stored);
        prefs.end();
    }
    return complete && valid(stored) ? stored : Config{};
}
inline bool save(const Config& config) {
    if (!valid(config)) return false;
    Preferences prefs;
    if (!prefs.begin("denclock-gw", false)) return false;
    bool ok = prefs.putBytes("config", &config, sizeof(config)) == sizeof(config);
    prefs.end();
    return ok;
}
struct Console {
    Config working, saved;
    bool active = false;
    uint32_t lastCommand = 0;
    char line[288]{};
    size_t length = 0;
    bool invalid = false, previousWasCr = false;
    explicit Console(const Config& config) : working(config), saved(config) {}
    void prompt() { if (active) Serial.printf("> "); }
    void show() {
        Serial.printf("DenClock Gateway Configuration\n\nLoRa Frequency: %.3f MHz\nLoRa Sync:      0x%04X\nService Host:   %s\nService Port:   %u\n\nConfiguration: %s\n",
            working.frequencyKhz / 1000.0, unsigned(working.sync), working.host,
            unsigned(LORA_SERVICE_PORT), same(working, saved) ? "Saved" : "Modified (unsaved)");
    }
    void exit() { working = saved; active = false; }
    void poll() {
        if (active && uint32_t(millis() - lastCommand) >= 300000) {
            Serial.println("Configuration timeout; unsaved changes discarded. Enter config to return.");
            exit();
            length = 0; invalid = false;
        }
        // Bound work even when a terminal continuously sends data.
        unsigned budget = 64;
        while (budget-- && Serial.available()) {
            char c = char(Serial.read());
            if (c == '\n' && previousWasCr) { previousWasCr = false; continue; }
            previousWasCr = c == '\r';
            if (c == '\b' || c == 127) {
                if (length) { line[--length] = 0; Serial.printf("\b \b"); }
                continue; // Poisoned lines stay poisoned until their delimiter.
            }
            if (c != '\r' && c != '\n') {
                if (static_cast<unsigned char>(c) >= 32 || c == '\t') {
                    Serial.printf("%c", c);
                    if (length + 1 < sizeof(line)) { line[length++] = c; line[length] = 0; }
                    else invalid = true;
                } else invalid = true;
                continue;
            }
            Serial.printf("\r\n");
            if (!length && !invalid) { prompt(); continue; }
            // Parse only after deliberate entry, so set cannot mutate idle RAM.
            Command command = Command::Invalid;
            if (!invalid) {
                if (active) command = parse(line, working);
                else {
                    Config ignored = saved;
                    if (parse(line, ignored) == Command::Enter) command = Command::Enter;
                }
            }
            length = 0; line[0] = 0; invalid = false;
            if (command == Command::Invalid) {
                Serial.println(active ? "Invalid command or value. Enter ? for help." : "Enter config to configure the gateway.");
                prompt();
                continue;
            }
            lastCommand = millis();
            switch (command) {
            case Command::Enter:
                active = true;
                Serial.println("DenClock Gateway Configuration\nEnter ? for help. Changes apply after reboot.");
                break;
            case Command::Help:
                Serial.println("? / help          Command reference\nshow              Working settings and save status\n"
                    "set frequency N   902125..927875 integer kHz\n"
                    "set sync HHHH     Exactly four hex digits, 0000..FFFF; optional 0x\n"
                    "set host NAME     DNS / .local / IPv4; max 253 characters, labels 1..63\n"
                    "                  Letters, digits, interior hyphens and dots; IPv4 octets 0..255\n"
                    "                  No whitespace or leading zeroes in IPv4 octets\n"
                    "save              Persist settings; stay in console\n"
                    "defaults          Restore development defaults in RAM\n"
                    "reboot            Restart without saving pending edits\n"
                    "exit              Discard unsaved edits; return to normal operation\n"
                    "Examples: set frequency 915500; set sync D3C7; set host denclock-d3c7.local\n"
                    "Enter each command on its own line. Settings apply only after reboot.\n"
                    "Five minutes without a valid command discards unsaved edits and exits.");
                break;
            case Command::Show: show(); break;
            case Command::Set: Serial.println("Updated in RAM; save to persist. Reboot to apply."); break;
            case Command::Defaults: working = Config{}; show(); break;
            case Command::Save:
                if (save(working)) { saved = working; Serial.println("Configuration saved. Reboot to apply."); }
                else Serial.println("Save failed; saved configuration unchanged. Enter ? for help.");
                break;
            case Command::Exit: exit(); Serial.println("Unsaved changes discarded. Enter config to return."); break;
            case Command::Reboot: Serial.flush(); ESP.restart(); return;
            default: break;
            }
            prompt();
        }
    }
};
} // namespace GatewayConfiguration
