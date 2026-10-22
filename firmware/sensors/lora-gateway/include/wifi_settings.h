#pragma once
#include <Preferences.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace GatewayWiFi {
struct Credentials {
    uint8_t version = 1;
    char ssid[33]{};
    char password[65]{};
};
inline bool valid(const Credentials& c) {
    const size_t s = strnlen(c.ssid, sizeof(c.ssid));
    const size_t p = strnlen(c.password, sizeof(c.password));
    if (c.version != 1 || s == 0 || s > 32 || p > 64) return false;
    // Open networks, WPA passphrases, or a 64-digit raw PSK.
    if (p == 0) return true;
    if (p < 8) return false;
    for (size_t i = 0; i < p; ++i) {
        const unsigned char b = c.password[i];
        if (p == 64) {
            if (!((b >= '0' && b <= '9') || (b >= 'a' && b <= 'f') || (b >= 'A' && b <= 'F'))) return false;
        } else if (b < 32 || b > 126) return false;
    }
    return true;
}
inline bool load(Credentials& c) {
    Preferences p;
    Credentials stored;
    if (!p.begin("denclock-wifi", true)) return false;
    bool ok = p.getBytesLength("credentials") == sizeof(stored) &&
        p.getBytes("credentials", &stored, sizeof(stored)) == sizeof(stored);
    p.end();
    if (ok && valid(stored)) { c = stored; return true; }
    return false;
}
inline bool save(const Credentials& c) {
    if (!valid(c)) return false;
    Preferences p;
    if (!p.begin("denclock-wifi", false)) return false;
    const bool ok = p.putBytes("credentials", &c, sizeof(c)) == sizeof(c);
    p.end();
    return ok;
}
inline bool clear() {
    Preferences p;
    if (!p.begin("denclock-wifi", false)) return false;
    // A valid empty tombstone also makes clearing idempotent.
    Credentials empty;
    const bool ok = p.putBytes("credentials", &empty, sizeof(empty)) == sizeof(empty);
    p.end();
    return ok;
}
inline bool setupPassword(char (&password)[9], const uint8_t (&random)[16]) {
    Preferences p;
    if (!p.begin("denclock-wifi", false)) return false;
    char stored[9]{};
    bool ok = p.getBytesLength("setup") == sizeof(stored) &&
        p.getBytes("setup", stored, sizeof(stored)) == sizeof(stored);
    const char alphabet[] = "abcdefghjklmnpqrstuvwxyz23456789";
    if (stored[8] != 0) ok = false;
    for (unsigned i = 0; i < 8; ++i)
        if (!stored[i] || !strchr(alphabet, stored[i])) ok = false;
    if (!ok) {
        for (unsigned i = 0; i < 8; ++i) stored[i] = alphabet[random[i] & 31];
        stored[8] = 0;
        ok = p.putBytes("setup", stored, sizeof(stored)) == sizeof(stored);
    }
    p.end();
    if (ok) memcpy(password, stored, sizeof(stored));
    return ok;
}
inline void setupSsid(char (&ssid)[33], const uint8_t (&mac)[6]) {
    snprintf(ssid, sizeof(ssid), "DenClock-Setup-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
} // namespace GatewayWiFi
