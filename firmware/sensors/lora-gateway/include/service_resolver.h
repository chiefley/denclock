#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#include "gateway_config.h"

// Resolve anew on each reconnect; no application-owned address cache.
inline bool resolveServiceHost(const char* host, IPAddress& address) {
    if (address.fromString(host)) return true;
    char name[GatewayConfiguration::MaxHostLength + 1];
    strcpy(name, host); // Configuration has already been validated.
    size_t length = strlen(name);
    if (length && name[length - 1] == '.') name[--length] = 0;
    if (length > 6 && GatewayConfiguration::equalWord(name + length - 6, ".local")) {
        name[length - 6] = 0; // ESPmDNS appends .local itself.
        static bool mdnsReady = false;
        if (!mdnsReady) mdnsReady = MDNS.begin("denclock-gateway");
        if (!mdnsReady) return false;
        address = MDNS.queryHost(name, 1000);
        return uint32_t(address) != 0;
    }
    return WiFi.hostByName(name, address) == 1;
}
