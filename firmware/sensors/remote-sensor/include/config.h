#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// Integer kHz, register bytes, and whole seconds keep parsing/conversions explicit.
struct SensorConfig {
    int32_t version = 2;
    int32_t frequencyKhz = 915000;
    uint16_t sync = 0xD3C7;
    int32_t node = 2, sample = 5, jitter = 1, heartbeat = 30, threshold = 1, power = 10;
};
inline bool validConfig(const SensorConfig& c) {
    return c.version == 2
        && c.frequencyKhz >= 902125 && c.frequencyKhz <= 927875
        && c.node >= 1 && c.node <= 239
        && c.sample >= 1 && c.sample <= 86400 && c.jitter >= 0 && c.jitter <= 2147
        && c.sample > c.jitter && c.heartbeat >= 1 && c.heartbeat <= 604800
        && c.threshold >= 1 && c.threshold <= 1650 && c.power >= -9 && c.power <= 22;
}
inline bool sameConfig(const SensorConfig& a, const SensorConfig& b) {
    return a.version == b.version && a.frequencyKhz == b.frequencyKhz && a.sync == b.sync
        && a.node == b.node && a.sample == b.sample
        && a.jitter == b.jitter && a.heartbeat == b.heartbeat
        && a.threshold == b.threshold && a.power == b.power;
}
inline SensorConfig validatedConfig(const SensorConfig& stored, bool complete) {
    return complete && validConfig(stored) ? stored : SensorConfig{};
}
enum class Command { Invalid, Help, Show, Set, Save, Defaults, Reboot, Exit };
inline Command parseCommand(char* line, SensorConfig& config) {
    for (char* p = line; *p; ++p) *p = char(tolower(static_cast<unsigned char>(*p)));
    char* context = nullptr;
    char* words[5]{};
    unsigned count = 0;
    for (char* p = strtok_r(line, " \t\v\f", &context); p; p = strtok_r(nullptr, " \t\v\f", &context)) {
        if (count == 4) return Command::Invalid;
        words[count++] = p;
    }
    if (count == 1) {
        if (!strcmp(words[0], "?") || !strcmp(words[0], "help")) return Command::Help;
        if (!strcmp(words[0], "show")) return Command::Show;
        if (!strcmp(words[0], "save")) return Command::Save;
        if (!strcmp(words[0], "defaults")) return Command::Defaults;
        if (!strcmp(words[0], "reboot")) return Command::Reboot;
        if (!strcmp(words[0], "exit")) return Command::Exit;
    }
    if (count != 3 || strcmp(words[0], "set")) return Command::Invalid;
    SensorConfig candidate = config;
    if (!strcmp(words[1], "sync")) {
        const char* hex = words[2];
        if (hex[0] == '0' && hex[1] == 'x') hex += 2;
        if (strlen(hex) != 4) return Command::Invalid;
        uint16_t value = 0;
        for (unsigned i = 0; i < 4; ++i) {
            if (!isxdigit(static_cast<unsigned char>(hex[i]))) return Command::Invalid;
            value = uint16_t((value << 4) | (hex[i] <= '9' ? hex[i] - '0' : hex[i] - 'a' + 10));
        }
        candidate.sync = value;
        if (!validConfig(candidate)) return Command::Invalid;
        config = candidate;
        return Command::Set;
    }
    char* end;
    errno = 0;
    long n = strtol(words[2], &end, 10);
    if (errno || *end || end == words[2] || n < INT32_MIN || n > INT32_MAX) return Command::Invalid;
    if (!strcmp(words[1], "frequency")) candidate.frequencyKhz = n;
    else if (!strcmp(words[1], "node")) candidate.node = n;
    else if (!strcmp(words[1], "sample")) candidate.sample = n;
    else if (!strcmp(words[1], "jitter")) candidate.jitter = n;
    else if (!strcmp(words[1], "heartbeat")) candidate.heartbeat = n;
    else if (!strcmp(words[1], "threshold")) candidate.threshold = n;
    else if (!strcmp(words[1], "power")) candidate.power = n;
    else return Command::Invalid;
    if (!validConfig(candidate)) return Command::Invalid;
    config = candidate;
    return Command::Set;
}
// Overflow/control characters poison the entire line, never execute a truncated command.
struct CommandLine {
    char text[96]{};
    unsigned length = 0;
    bool invalid = false;
    bool feed(char c) {
        if (c == '\r' || c == '\n') return length || invalid;
        if (static_cast<unsigned char>(c) < 32 && c != '\t') invalid = true;
        else if (length + 1 < sizeof(text)) { text[length++] = c; text[length] = 0; }
        else invalid = true;
        return false;
    }
    void clear() { length = 0; invalid = false; text[0] = 0; }
};
