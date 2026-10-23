#pragma once
#include <WiFi.h>
#include <mdns.h>
#include "gateway_config.h"

// Resolve anew on each reconnect; no application-owned address cache.
// Use the dedicated mDNS client for .local, not lwIP's legacy one-shot queries.
inline bool resolveServiceHost(const char* host, IPAddress& address) {
    if (address.fromString(host)) return true;
    char name[GatewayConfiguration::MaxHostLength + 1];
    strcpy(name, host); // Configuration has already been validated.
    size_t length = strlen(name);
    if (length && name[length - 1] == '.') name[--length] = 0;
    if (length > 6 && GatewayConfiguration::equalWord(name + length - 6, ".local")) {
        // mdns_query_a takes the hostname without the .local domain.
        name[length - 6] = 0;
        static bool initialized = false;
        if (!initialized) {
            const esp_err_t error = mdns_init();
            if (error != ESP_OK) {
                Serial.printf("mDNS client initialization failed (%d); will retry\n", int(error));
                return false;
            }
            // No mdns_hostname_set or service registration: query only.
            // The library handles Wi-Fi/IP events for subsequent reconnects.
            initialized = true;
        }
        esp_ip4_addr_t result = {};
        if (mdns_query_a(name, 2000, &result) != ESP_OK || result.addr == 0) return false;
        address = IPAddress(result.addr);
        return true;
    }
    return WiFi.hostByName(name, address) == 1;
}
