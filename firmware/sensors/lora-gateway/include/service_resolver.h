#pragma once
#include <WiFi.h>
#include "gateway_config.h"

// Resolve anew on each reconnect; no application-owned address cache.
// The pinned ESP32 lwIP resolver supports .local queries without a responder.
inline bool resolveServiceHost(const char* host, IPAddress& address) {
    if (address.fromString(host)) return true;
    char name[GatewayConfiguration::MaxHostLength + 1];
    strcpy(name, host); // Configuration has already been validated.
    size_t length = strlen(name);
    if (length && name[length - 1] == '.') name[--length] = 0;
    // lwIP recognizes the .local suffix case-sensitively.
    if (length > 6 && GatewayConfiguration::equalWord(name + length - 6, ".local"))
        strcpy(name + length - 6, ".local");
    return WiFi.hostByName(name, address) == 1;
}
