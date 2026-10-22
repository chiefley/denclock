#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "config.h"

namespace GatewayConfiguration {
constexpr size_t MaxHostLength = 253;
struct Config {
    int32_t version = 1;
    int32_t frequencyKhz = 915000;
    uint16_t sync = 0xD3C7;
    char host[MaxHostLength + 1] = LORA_SERVICE_HOST;
};
inline bool validHost(const char* host) {
    size_t length = strnlen(host, MaxHostLength + 1);
    if (!length || length > MaxHostLength) return false;
    bool numeric = true;
    for (size_t i = 0; i < length; ++i)
        if ((host[i] < '0' || host[i] > '9') && host[i] != '.') numeric = false;
    if (numeric) {
        // Numeric dotted input is an IPv4 literal, never a fallback DNS name.
        unsigned parts = 0;
        const char* p = host;
        while (*p) {
            const char* start = p;
            unsigned value = 0;
            while (*p >= '0' && *p <= '9') {
                value = value * 10 + unsigned(*p++ - '0');
                if (p - start > 3 || value > 255) return false;
            }
            if (p == start || (p - start > 1 && *start == '0')) return false;
            ++parts;
            if (!*p) break;
            if (*p++ != '.' || !*p) return false;
        }
        return parts == 4;
    }
    // Permit an explicit DNS root dot, but not empty interior labels.
    if (host[length - 1] == '.') --length;
    size_t labelStart = 0;
    for (size_t i = 0; i <= length; ++i) {
        if (i == length || host[i] == '.') {
            size_t n = i - labelStart;
            if (!n || n > 63 || host[labelStart] == '-' || host[i - 1] == '-') return false;
            labelStart = i + 1;
        } else {
            const char c = host[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9') || c == '-')) return false;
        }
    }
    return true;
}
inline bool valid(const Config& c) {
    return c.version == 1 && c.frequencyKhz >= 902125 && c.frequencyKhz <= 927875 && validHost(c.host);
}
inline bool same(const Config& a, const Config& b) {
    return a.version == b.version && a.frequencyKhz == b.frequencyKhz
        && a.sync == b.sync && !strcmp(a.host, b.host);
}
inline bool equalWord(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower(static_cast<unsigned char>(*a++)) != *b++) return false;
    }
    return !*a && !*b;
}
enum class Command { Invalid, Enter, Help, Show, Set, Save, Defaults, Reboot, Exit };
inline Command parse(char* line, Config& config) {
    char* context = nullptr;
    char* words[4]{};
    unsigned count = 0;
    for (char* p = strtok_r(line, " \t\v\f", &context); p; p = strtok_r(nullptr, " \t\v\f", &context)) {
        if (count == 4) return Command::Invalid;
        words[count++] = p;
    }
    if (count == 1) {
        if (equalWord(words[0], "config")) return Command::Enter;
        if (equalWord(words[0], "?") || equalWord(words[0], "help")) return Command::Help;
        if (equalWord(words[0], "show")) return Command::Show;
        if (equalWord(words[0], "save")) return Command::Save;
        if (equalWord(words[0], "defaults")) return Command::Defaults;
        if (equalWord(words[0], "reboot")) return Command::Reboot;
        if (equalWord(words[0], "exit")) return Command::Exit;
    }
    if (count != 3 || !equalWord(words[0], "set")) return Command::Invalid;
    Config candidate = config;
    if (equalWord(words[1], "host")) {
        if (strlen(words[2]) > MaxHostLength) return Command::Invalid;
        strcpy(candidate.host, words[2]);
    } else if (equalWord(words[1], "sync")) {
        const char* hex = words[2];
        if (hex[0] == '0' && tolower(static_cast<unsigned char>(hex[1])) == 'x') hex += 2;
        if (strlen(hex) != 4) return Command::Invalid;
        uint16_t value = 0;
        for (unsigned i = 0; i < 4; ++i) {
            const char c = char(tolower(static_cast<unsigned char>(hex[i])));
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return Command::Invalid;
            value = uint16_t((value << 4) | (c <= '9' ? c - '0' : c - 'a' + 10));
        }
        candidate.sync = value;
    } else if (equalWord(words[1], "frequency")) {
        char* end;
        errno = 0;
        long n = strtol(words[2], &end, 10);
        if (errno || *end || end == words[2] || n < 902125 || n > 927875) return Command::Invalid;
        candidate.frequencyKhz = int32_t(n);
    } else return Command::Invalid;
    if (!valid(candidate)) return Command::Invalid;
    config = candidate;
    return Command::Set;
}
} // namespace GatewayConfiguration
