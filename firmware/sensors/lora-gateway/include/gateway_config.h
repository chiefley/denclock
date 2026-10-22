#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "config.h"

// RFC 1035 limit for a hostname in text form, without a trailing dot.
constexpr size_t MaxHostLength = 253;
// Integer kHz and register bytes match the sensor configuration representation.
struct GatewayConfig {
    int32_t version = 1;
    int32_t frequencyKhz = 915000;
    uint16_t sync = 0xD3C7;
    char host[MaxHostLength + 1] = LORA_SERVICE_HOST;
};
// Dotted quad only: no shorthand forms, and no leading zeros that could read as octal.
inline bool validIpv4(const char* text) {
    for (unsigned parts = 1;; ++parts) {
        unsigned value = 0, digits = 0;
        const char* start = text;
        for (; isdigit(static_cast<unsigned char>(*text)); ++text) {
            if (++digits > 3) return false;
            value = value * 10 + unsigned(*text - '0');
        }
        if (!digits || value > 255 || (digits > 1 && *start == '0')) return false;
        if (!*text) return parts == 4;
        if (*text++ != '.' || parts == 4) return false;
    }
}
inline bool validHost(const char* host) {
    const size_t length = strnlen(host, MaxHostLength + 1);
    if (length == 0 || length > MaxHostLength) return false;
    // DNS names never end in an all-numeric label, so such text must be IPv4.
    const char* last = strrchr(host, '.');
    last = last ? last + 1 : host;
    bool numeric = *last != 0;
    for (const char* p = last; *p; ++p)
        if (!isdigit(static_cast<unsigned char>(*p))) numeric = false;
    if (numeric) return validIpv4(host);
    // Labels are 1..63 letters, digits or interior hyphens; no empty labels.
    size_t label = 0;
    for (const char* p = host;; ++p) {
        if (*p == '.' || *p == 0) {
            if (label == 0 || p[-1] == '-') return false;
            if (*p == 0) return true;
            label = 0;
        } else if (isalnum(static_cast<unsigned char>(*p)) || *p == '-') {
            if ((label == 0 && *p == '-') || ++label > 63) return false;
        } else {
            return false;
        }
    }
}
inline bool validConfig(const GatewayConfig& c) {
    return c.version == 1
        && c.frequencyKhz >= 902125 && c.frequencyKhz <= 927875
        && memchr(c.host, 0, sizeof(c.host)) && validHost(c.host);
}
inline bool sameConfig(const GatewayConfig& a, const GatewayConfig& b) {
    return a.version == b.version && a.frequencyKhz == b.frequencyKhz && a.sync == b.sync
        && !strcmp(a.host, b.host);
}
inline GatewayConfig validatedConfig(const GatewayConfig& stored, bool complete) {
    return complete && validConfig(stored) ? stored : GatewayConfig{};
}
enum class ConsoleCommand { Invalid, Config, Help, Show, Set, Save, Defaults, Reboot, Exit };
inline ConsoleCommand parseCommand(char* line, GatewayConfig& config) {
    char* context = nullptr;
    char* words[4]{};
    unsigned count = 0;
    for (char* p = strtok_r(line, " \t\v\f", &context); p; p = strtok_r(nullptr, " \t\v\f", &context)) {
        if (count == 3) return ConsoleCommand::Invalid;
        words[count++] = p;
    }
    // Keywords ignore case; a hostname keeps its entered representation.
    for (unsigned i = 0; i < count; ++i) {
        if (i == 2 && !strcmp(words[1], "host")) break;
        for (char* p = words[i]; *p; ++p) *p = char(tolower(static_cast<unsigned char>(*p)));
    }
    if (count == 1) {
        if (!strcmp(words[0], "config")) return ConsoleCommand::Config;
        if (!strcmp(words[0], "?") || !strcmp(words[0], "help")) return ConsoleCommand::Help;
        if (!strcmp(words[0], "show")) return ConsoleCommand::Show;
        if (!strcmp(words[0], "save")) return ConsoleCommand::Save;
        if (!strcmp(words[0], "defaults")) return ConsoleCommand::Defaults;
        if (!strcmp(words[0], "reboot")) return ConsoleCommand::Reboot;
        if (!strcmp(words[0], "exit")) return ConsoleCommand::Exit;
    }
    if (count != 3 || strcmp(words[0], "set")) return ConsoleCommand::Invalid;
    GatewayConfig candidate = config;
    if (!strcmp(words[1], "host")) {
        if (!validHost(words[2])) return ConsoleCommand::Invalid;
        // validHost bounds the length, so the copy always fits with its terminator.
        memset(candidate.host, 0, sizeof(candidate.host));
        memcpy(candidate.host, words[2], strlen(words[2]));
    } else if (!strcmp(words[1], "sync")) {
        const char* hex = words[2];
        if (hex[0] == '0' && hex[1] == 'x') hex += 2;
        if (strlen(hex) != 4) return ConsoleCommand::Invalid;
        uint16_t value = 0;
        for (unsigned i = 0; i < 4; ++i) {
            if (!isxdigit(static_cast<unsigned char>(hex[i]))) return ConsoleCommand::Invalid;
            value = uint16_t((value << 4) | (hex[i] <= '9' ? hex[i] - '0' : hex[i] - 'a' + 10));
        }
        candidate.sync = value;
    } else if (!strcmp(words[1], "frequency")) {
        char* end;
        errno = 0;
        long n = strtol(words[2], &end, 10);
        if (errno || *end || end == words[2] || n < INT32_MIN || n > INT32_MAX) return ConsoleCommand::Invalid;
        candidate.frequencyKhz = n;
    } else {
        return ConsoleCommand::Invalid;
    }
    if (!validConfig(candidate)) return ConsoleCommand::Invalid;
    config = candidate;
    return ConsoleCommand::Set;
}
// Overflow/control characters poison the entire line, never execute a truncated command.
// The buffer holds "set host" with a maximum-length name and surrounding whitespace.
struct CommandLine {
    char text[288]{};
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
